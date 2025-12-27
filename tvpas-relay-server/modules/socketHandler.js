const { randomUUID } = require('crypto');
const { performance } = require('perf_hooks');
const osc = require('node-osc');
const logger = require('./logger.js');

// The createSocketHandler function is now a factory that takes the server's role
function createSocketHandler(role, tvpasOscClient, config, comPort, state, cameraSwitcherOscClient) { // cameraSwitcherOscClient を引数に追加
    // Determine the port for this specific server instance
    let serverPort;
    switch (role) {
        case 'server1': serverPort = config.server1_port; break;
        case 'server2': serverPort = config.server2_port; break;
        case 'server3': serverPort = config.server3_port; break;
        case 'server4': serverPort = config.server4_port; break;
        default: serverPort = -1; // Should not happen with valid roles
    }

    // This function is returned and executed for each new socket connection
    return (socket) => {
        logger.info(`FPVTrackside client connected to '${role}' server: ${socket.id}`);

        // Heartbeat to FPVTrackside
        const heartbeatInterval = setInterval(() => {
            const serverIds = config[`${role}_id`] || [];
            const heartbeatData = { frequency: serverIds };
            socket.emit('heartbeat', heartbeatData);
            logger.debug(`[${role}] Emitted 'heartbeat' to FPVTrackside: ${JSON.stringify(heartbeatData)}`); // Added debug log
        }, 5000);


        socket.on("ts_server_info", (callback) => {
            logger.info(`[${role}] Received 'ts_server_info' request.`);
            if (callback) callback({ release_version: "1.3.0", name: "TinyViewPlus AsSensors" });
        });

        socket.on("ts_server_time", (callback) => {
            const server_time_monotonic = performance.now() / 1000; // Get monotonic time in seconds

            logger.info(`[${role}] Received 'ts_server_time' from client. server_time_monotonic: ${server_time_monotonic}`);

            if (callback) {
                callback(server_time_monotonic); // Return monotonic time in seconds
                logger.info(`[${role}] Emitted 'ts_server_time_ack' to client with server_time: ${server_time_monotonic}`);
            } else {
                logger.warn(`[${role}] Received 'ts_server_time' request without a callback. No 'ts_server_time_ack' sent.`);
            }
        });

        socket.on("ts_frequency_setup", (data, callback) => {
            logger.info(`[${role}] Received 'ts_frequency_setup'`);

            // Only the server1 server updates the shared pilot data map.
            if (role === 'server1') {
                logger.info("[server1] Updating pilot data map with received setup.");
                state.pilotData = {};
                if (data && data.f) {
                    for (let i = 0; i < data.f.length; i++) {
                        const freq = data.f[i];
                        state.pilotData[freq] = { seat: i, band: data.b ? data.b[i] : null, channel: data.c ? data.c[i] : null };
                    }
                }
                logger.info("Pilot data map has been updated.", state.pilotData);
                logger.info("DEBUG: state.pilotData AFTER server1 ts_frequency_setup:", state.pilotData);
            } else {
                logger.info(`[${role}] Ignoring frequency setup data update as this is not the server1 server.`);
            }

            // As requested, do not send any OSC message to TVPAS.

            // Immediately acknowledge the request for all servers.
            if (callback) callback(true);
        });

        socket.on("ts_race_stage", (data, callback) => {
            logger.info(`[${role}] Received 'ts_race_stage' with data: ${JSON.stringify(data)}`);
            
            // 全てのサーバーでクライアントに 'stage_ready' をemitする
            socket.emit("stage_ready", { pi_starts_at_s: data.start_time_s });
            logger.debug(`[${role}] Emitted 'stage_ready' back to client ${socket.id}`);

            // server1以外のロールではstateの更新やTFPASへのOSC送信、COMポートへの書き込みは行わない
            if (role !== 'server1') {
                logger.info(`[${role}] Ignoring 'ts_race_stage' state update, OSC send, and COM port write as this is not the server1 server.`);
                if (callback) callback();
                return;
            }

            state.raceState.status = 'ACTIVE'; // Set global race state
            logger.info("Global race state is now ACTIVE.");

            logger.info("[server1] Processing 'ts_race_stage' for TVPAS and COM port.");

            // --- Forward /start_race to Camera Switcher ---
            if (config.osc_forwarding && cameraSwitcherOscClient) {
                const activeSeats = Object.values(state.pilotData).map(p => p.seat);
                const message = new osc.Message('/start_race', ...activeSeats); // シート番号の配列を送信
                cameraSwitcherOscClient.send(message, (err) => {
                    if (err) {
                        logger.error(`Error forwarding OSC message to camera-switcher (/start_race): ${err.message}`);
                    } else {
                        logger.debug(`Forwarded OSC message to camera-switcher: /start_race ${activeSeats}`);
                    }
                });
            }

            // Forward to TVPAS in the old format
            const oscMsg = new osc.Message('/stage_ready');
            oscMsg.append(data.start_time_s); // Send only the start time float
            tvpasOscClient.send(oscMsg, (err) => {
                if (err) logger.error(`[server1] FATAL ERROR sending /stage_ready to TVPAS: ${err}`);
                else logger.debug(`[server1] Sent /stage_ready to TVPAS with start time: ${data.start_time_s}`);
            });

            // Handle COM port for server1
            if (comPort) {
                comPort.write('S', (err) => {
                    if (err) return logger.error('Error writing to COM port: ', err.message);
                    logger.debug('Sent "S" to COM port for race start.');
                });
            }

            if (callback) callback();
        });

        const handleRaceStop = (eventName) => {
            logger.info(`[${role}] Received '${eventName}'`);
            if (role !== 'server1') {
                logger.info(`[${role}] Ignoring '${eventName}' as this is not the server1 server.`);
                return;
            }

            state.raceState.status = 'stopped';
            logger.info("Global race state is now STOPPED.");

            // --- Forward /end_race to Camera Switcher ---
            if (config.osc_forwarding && cameraSwitcherOscClient) {
                const message = new osc.Message('/end_race'); // 引数なしで送信
                cameraSwitcherOscClient.send(message, (err) => {
                    if (err) {
                        logger.error(`Error forwarding OSC message to camera-switcher (/end_race): ${err.message}`);
                    } else {
                        logger.debug(`Forwarded OSC message to camera-switcher: /end_race`);
                    }
                });
            }

            if (comPort) {
                comPort.write('E', (err) => {
                    if (err) return logger.error('Error writing to COM port: ', err.message);
                    logger.debug('Sent "E" to COM port for race end.'); // Added debug log
                });
            }
        };

        socket.on("ts_race_stop", (cb) => {
            handleRaceStop("ts_race_stop");
            if (cb) cb();
        });

        socket.on("ts_race_abort", (cb) => {
            handleRaceStop("ts_race_abort");
            if (cb) cb();
        });

        socket.on("disconnect", () => {
            logger.info(`Client disconnected from '${role}': ${socket.id}`);
            clearInterval(heartbeatInterval);
        });
    };
}

module.exports = createSocketHandler;
