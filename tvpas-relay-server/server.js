const { Server } = require("socket.io");
const osc = require('node-osc');

// --- Settings ---
const SOCKET_IO_PORT = 5000; //Socket.IO port that FPVTrackside references. (TCP) 
const OSC_LISTEN_PORT = 8000; //
const OSC_LISTEN_HOST = '127.0.0.1';
const TVPAS_OSC_RECEIVE_PORT = 8001; // TVPASのOSC受信ポート

console.log("--- RotorHazard Relay for tinyviewplusAsSensors ---");

// --- In-memory state ---
let tvpasReady = false; // NEW: Flag to indicate if TVPAS is connected
let raceState = { status: 'stopped' };
let pilotData = {}; // Store full pilot data { frequency: { seat: i, ... } }

// 1. Create Socket.IO Server
const io = new Server(SOCKET_IO_PORT, { cors: { origin: "*" } });
console.log(`[Socket.IO] Server listening on port ${SOCKET_IO_PORT}`);
console.log("[System] Waiting for the first heartbeat from tinyviewplusAsSensors...");

// 2. Create OSC Server
const oscServer = new osc.Server(OSC_LISTEN_PORT, OSC_LISTEN_HOST, () => {
    console.log(`[OSC] Server listening on ${OSC_LISTEN_HOST}:${OSC_LISTEN_PORT}`);
});

io.on("connection", (socket) => {
    console.log(`[Socket.IO] FPVTrackside client trying to connect: ${socket.id}`);

    // --- HANDSHAKE GATE ---
    // If TVPAS is not ready, do not proceed. FPVTrackside will timeout and retry.
    if (!tvpasReady) {
        console.log(`[Socket.IO] Denied connection from ${socket.id} because TVPAS is not ready. FPVTrackside should retry.`);
        socket.disconnect(true);
        return;
    }
    
    console.log(`[Socket.IO] FPVTrackside client accepted: ${socket.id}`);

    // --- Normal Event Handlers ---
    socket.on("ts_server_info", (callback) => {
        console.log("[Socket.IO] Received 'ts_server_info' request.");
        if (callback) callback({ release_version: "1.0.0", name: "TinyViewPlus As A Sensor" });
    });

    socket.on("ts_server_time", (callback) => {
        console.log("[Socket.IO] Received 'ts_server_time' request. Forwarding to TVPAS via OSC.");
        const tvpasOscClient = new osc.Client(OSC_LISTEN_HOST, TVPAS_OSC_RECEIVE_PORT);

        const responseListener = (msgArray) => {
            const address = msgArray[0];
            if (address === '/server_time_response') {
                const serverTime = msgArray[1];
                console.log(`[OSC] Received /server_time_response from TVPAS with time: ${serverTime}`);
                if (callback) {
                    callback(serverTime);
                    console.log(`[Socket.IO] Sent time (${serverTime}) back to FPVTrackside.`);
                }
                oscServer.removeListener('message', responseListener);
                clearTimeout(timeout);
                tvpasOscClient.close();
            }
        };
        
        oscServer.on('message', responseListener);

        const timeout = setTimeout(() => {
            console.error("[OSC] Timeout: Did not receive /server_time_response from TVPAS within 2 seconds.");
            oscServer.removeListener('message', responseListener);
            if (callback) {
                callback(null);
                console.log("[Socket.IO] Responded with null to FPVTrackside due to timeout.");
            }
            tvpasOscClient.close();
        }, 2000);

        const requestMsg = new osc.Message('/get_server_time');
        tvpasOscClient.send(requestMsg, (err) => {
            if (err) {
                console.error('[OSC] FATAL ERROR sending /get_server_time to TVPAS:', err);
                clearTimeout(timeout);
                oscServer.removeListener('message', responseListener);
                if (callback) {
                    callback(null);
                    console.log("[Socket.IO] Responded with null to FPVTrackside due to OSC send error.");
                }
                tvpasOscClient.close();
            } else {
                console.log(`[OSC] Sent /get_server_time request to TVPAS (${OSC_LISTEN_HOST}:${TVPAS_OSC_RECEIVE_PORT}).`);
            }
        });
    });

    socket.on("ts_frequency_setup", (data, callback) => {
        console.log("[Socket.IO] Received 'ts_frequency_setup':", data);
        pilotData = {};
        if (data && data.f) {
            for (let i = 0; i < data.f.length; i++) {
                const freq = data.f[i];
                pilotData[freq] = { 
                    seat: i, 
                    band: data.b ? data.b[i] : null,
                    channel: data.c ? data.c[i] : null
                };
            }
        }
        console.log("[State] Updated pilot data map:", pilotData);
        if (callback) callback(true);
    });

    socket.on("ts_race_stage", (data, callback) => {
        console.log("[Socket.IO] Received 'ts_race_stage':", data);
        raceState.status = 'racing';
        console.log("[State] Race is now ACTIVE.");

        socket.emit("stage_ready", { pi_starts_at_s: data.start_time_s });
        console.log("[Socket.IO] Emitted 'stage_ready'.");

        const tvpasOscClient = new osc.Client(OSC_LISTEN_HOST, TVPAS_OSC_RECEIVE_PORT);
        const tvpasStageReadyMsg = new osc.Message('/stage_ready');
        tvpasStageReadyMsg.append(data.start_time_s);
        tvpasOscClient.send(tvpasStageReadyMsg, (err) => {
            if (err) {
                console.error('[OSC] FATAL ERROR sending /stage_ready to TVPAS:', err);
            } else {
                console.log(`[OSC] Sent /stage_ready to TVPAS with data: ${data.start_time_s}`);
            }
            tvpasOscClient.close();
        });

        if (callback) {
            callback();
            console.log("[Socket.IO] Acknowledged ts_race_stage.");
        }
    });

    socket.on("ts_race_stop", (cb) => { raceState.status = 'stopped'; console.log("[State] Race stopped."); if (cb) cb(); });
    socket.on("ts_race_abort", (cb) => { raceState.status = 'stopped'; console.log("[State] Race aborted."); if (cb) cb(); });
    socket.on("disconnect", () => console.log(`[Socket.IO] Client disconnected: ${socket.id}`));
});


oscServer.on('message', (msgArray) => {
    const address = msgArray[0];
    const args = msgArray.slice(1);

    if (address === '/heartbeat') {
        if (!tvpasReady) {
            tvpasReady = true;
            console.log("[System] First heartbeat received! tinyviewplusAsSensors is now connected.");
            console.log("[System] The relay is now active and will accept connections from FPVTrackside.");
        }
        
        const jsonString = args[0];
        if (typeof jsonString === 'string') {
            try {
                const heartbeat_data = JSON.parse(jsonString);
                if (heartbeat_data.loop_time && Array.isArray(heartbeat_data.loop_time)) {
                    heartbeat_data.loop_time = heartbeat_data.loop_time.map(Math.round);
                }
                io.emit('heartbeat', heartbeat_data);
            } catch (e) {
                console.error(`[OSC] Error parsing heartbeat JSON: ${e.message}`);
            }
        }
    } else if (address === '/ts_lap_data') {
        console.log(`[OSC] Handling /ts_lap_data. Current race status: ${raceState.status}`);

        if (raceState.status !== 'racing') {
            console.log(`[Warning] Ignoring lap data because race status is '${raceState.status}', not 'racing'.`);
            return;
        }

        const lapTime = args[0];
        const frequency = args[1];
        const peakRssi = args[2];

        const pilot = pilotData[frequency];

        if (!pilot) {
            console.log(`[Warning] Ignoring lap for unassigned frequency: ${frequency}`);
            return;
        }

        const lapData = {
            seat: pilot.seat,
            frequency: frequency,
            peak_rssi: peakRssi,
            lap_time: lapTime
        };

        io.emit('ts_lap_data', lapData);
        console.log(`[Socket.IO] Emitted 'ts_lap_data':`, lapData);
    }
    // We don't log every single message here anymore to reduce noise, only known ones.
});

