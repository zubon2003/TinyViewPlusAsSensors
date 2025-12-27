const logger = require('./logger.js');
const osc = require('node-osc'); // osc.Message を使うために追加

/**
 * Creates a handler for OSC messages from TVPAS.
 * @param {Object.<string, import('socket.io').Server>} servers - An object containing all active Socket.IO server instances, keyed by role (e.g., 'primary', 'slave1').
 * @param {object} config - The application configuration.
 * @param {import('serialport').SerialPort} comPort - The serial port instance, if available.
 * @param {import('node-osc').Client} cameraSwitcherOscClient - The OSC client for camera-switcher, if osc_forwarding is enabled.
 * @param {object} state - The global state object.
 * @returns {function(Array): void} The OSC message handler function.
 */
function createOscHandler(servers, config, comPort, cameraSwitcherOscClient, state) { // state を引数に追加
    const serverRoles = Object.keys(servers); // e.g., ['primary', 'slave1', 'slave2']

    // This function is returned and executed for each incoming OSC message
    return (msgArray) => {
        const address = msgArray[0];
        const args = msgArray.slice(1);

        logger.debug(`OSC message received: ${address} ${JSON.stringify(args)}`); // Added debug log for all OSC messages

        // OSCフォワーディングが有効な場合、camera-switcherにメッセージを転送
        if (config.osc_forwarding && cameraSwitcherOscClient) {
            // /heartbeatはノイズが多い可能性があるので転送しない
            if (address !== '/heartbeat' && address !== '/ts_lap_data' && address !== '/start_race' && address !== '/end_race') {
                const message = new osc.Message(address, ...args);
                cameraSwitcherOscClient.send(message, (err) => {
                    if (err) {
                        logger.error(`Error forwarding OSC message to camera-switcher (${address}): ${err.message}`);
                    } else {
                        logger.debug(`Forwarded OSC message to camera-switcher: ${address} ${JSON.stringify(args)}`);
                    }
                });
            }
        }

        switch (address) {
            case '/ts_lap_data': {
                // レースがACTIVEでない場合はラップデータを無視する
                if (state.raceState.status !== 'ACTIVE') {
                    logger.warn(`Ignoring lap data as race is not active. Current state: ${state.raceState.status}`);
                    break;
                }
                // args が単一のJSON文字列として来ているので、それをパースし直す
                let parsedArgs = args;
                if (args.length === 1 && typeof args[0] === 'string' && args[0].startsWith('[')) {
                    try {
                        parsedArgs = JSON.parse(args[0]);
                    } catch (e) {
                        logger.error(`Error parsing lap data args: ${e.message}, raw args: ${args[0]}`);
                        break;
                    }
                }

                // これで parsedArgs が [lapTime, frequency, peakRssi, markerId] の形式になる
                const lapTime = parsedArgs[0];
                const frequency = parsedArgs[1];
                const peakRssi = parsedArgs[2];
                const markerId = parseInt(parsedArgs[3], 10); // markerId はそのまま使用

                // Check if markerId is a valid number before proceeding
                if (isNaN(markerId)) {
                    logger.warn(`Received lap data with invalid marker ID: ${parsedArgs[3]}`);
                    break;
                }

                // Perform pilot lookup here to include seat for camera-switcher
                const pilot = state.pilotData[frequency];
                if (!pilot) {
                    logger.warn(`Ignoring lap data for frequency ${frequency} as no pilot data found.`);
                    break; // Exit if no pilot data
                }
                const seat = pilot.seat; // Get seat for lapData

                const lapData = {
                    seat: seat,
                    frequency: frequency,
                    lap_time: lapTime,
                    peak_rssi: peakRssi
                };
                
                let actualServerIdForCameraSwitcher = null; // camera-switcher に送る serverID
                let serverFoundForSocketIO = false; // Socket.IOへの転送が行われたか

                for (const role of serverRoles) {
                    const assignedIds = config[`${role}_id`];
                    logger.debug(`DEBUG: Checking role '${role}' with assigned IDs: [${assignedIds}]`);

                    if (assignedIds && assignedIds.includes(markerId)) {
                        servers[role].emit('ts_lap_data', lapData);
                        logger.info(`Emitted 'ts_lap_data' to FPVTrackside (from '${role}' server):`, lapData);
                        serverFoundForSocketIO = true;
                        
                        // ここで actualServerIdForCameraSwitcher を決定する
                        if (role === 'server1') actualServerIdForCameraSwitcher = 1;
                        else if (role === 'server2') actualServerIdForCameraSwitcher = 2;
                        else if (role === 'server3') actualServerIdForCameraSwitcher = 3;
                        else if (role === 'server4') actualServerIdForCameraSwitcher = 4;

                        // Handle serial port for server1 server passes
                        if (role === 'server1' && comPort && comPort.isOpen) {
                            const seatChar = seat.toString();
                            comPort.write(seatChar, (err) => {
                                if (err) logger.error('Error writing to COM port: ', err.message);
                                else logger.debug(`Sent seat "${seatChar}" to COM port for server1 pass.`);
                            });
                        }
                        // ここで break すると、camera-switcher へのフォワードの前にループが終わるため、
                        // Socket.IO への転送ロジックとは別に camera-switcher へのフォワードを行う
                    }
                }
                
                if (!serverFoundForSocketIO) {
                    logger.warn(`Received lap data for unassigned marker ID: ${markerId}`);
                }

                // --- Forward to Camera Switcher (一度だけ行う) ---
                if (config.osc_forwarding && cameraSwitcherOscClient && actualServerIdForCameraSwitcher !== null) {
                    const message = new osc.Message('/ts_lap_data', seat, actualServerIdForCameraSwitcher); // [seat, actualServerIdForCameraSwitcher]形式で送信
                    cameraSwitcherOscClient.send(message, (err) => {
                        if (err) {
                            logger.error(`Error forwarding OSC message to camera-switcher (/ts_lap_data): ${err.message}`);
                        } else {
                            logger.debug(`Forwarded OSC message to camera-switcher: /ts_lap_data ${seat}, ${actualServerIdForCameraSwitcher}`);
                        }
                    });
                }
                break;
            }

            case '/start_race': {
                logger.info(`Received '${address}' from TVPAS.`);
                // --- Forward to Camera Switcher ---
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
                // TODO: ここでSocket.IOクライアントへのブロードキャストが必要であれば追加
                break;
            }

            case '/end_race': {
                logger.info(`Received '${address}' from TVPAS.`);
                // --- Forward to Camera Switcher ---
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
                // TODO: ここでSocket.IOクライアントへのブロードキャストが必要であれば追加
                break;
            }
                
            case '/heartbeat': {                
                logger.debug(`Received '/heartbeat' from TVPAS with args: ${JSON.stringify(args)}`); // Enhanced debug log
                try {
                    const jsonString = args[0];
                    const heartbeat_data = JSON.parse(jsonString);
                    
                    if (heartbeat_data.flicker_length) {
                        state.flickerLength = heartbeat_data.flicker_length;
                        delete heartbeat_data.flicker_length;
                    }
                    if (heartbeat_data.loop_time && Array.isArray(heartbeat_data.loop_time)) {
                        heartbeat_data.loop_time = heartbeat_data.loop_time.map(Math.round);
                    }

                    // Broadcast heartbeat to all connected clients on all servers
                    for (const role of serverRoles) {
                        servers[role].emit('heartbeat', heartbeat_data);
                        logger.debug(`Emitted 'heartbeat' to '${role}' server: ${JSON.stringify(heartbeat_data)}`);                    }
                } catch (e) {
                    logger.error(`Error parsing heartbeat JSON from TVPAS: ${e.message}. Raw args: ${JSON.stringify(args)}`); // Enhanced error log
                }
                break;
            }

            default:
                // To reduce noise, we don't log unknown messages.
                // logger.debug(`Received unknown OSC message: ${address}`);
                break;
        }
    };
}

module.exports = createOscHandler;
