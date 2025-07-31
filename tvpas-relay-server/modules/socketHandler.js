const osc = require('node-osc');
const state = require('./state.js');
const logger = require('./logger.js');

function createSocketHandler(io, oscServer, config, comPort) {
    return (socket) => {
        logger.info(`FPVTrackside client trying to connect: ${socket.id}`);

        if (!state.tvpasReady) {
            logger.warn(`Denied connection from ${socket.id} because TVPAS is not ready. FPVTrackside should retry.`);
            socket.disconnect(true);
            return;
        }

        logger.info(`FPVTrackside client accepted: ${socket.id}`);

        socket.on("ts_server_info", (callback) => {
            logger.info("Received 'ts_server_info' request.");
            if (callback) callback({ release_version: "1.0.0", name: "TinyViewPlus As A Sensor" });
        });

        socket.on("ts_server_time", (callback) => {
            logger.info("Forwarding 'ts_server_time' request to TVPAS via OSC.");
            const tvpasOscClient = new osc.Client(config.tvpas_host, config.tvpas_osc_listen_port);
            
            const responseAddress = '/server_time_response';

            const responseListener = (msgArray) => {
                const serverTime = msgArray[1];
                logger.info(`Received /server_time_response from TVPAS with time: ${serverTime}`);
                if (callback) {
                    callback(serverTime);
                    logger.info(`Sent time (${serverTime}) back to FPVTrackside.`);
                }
                oscServer.removeListener(responseAddress, responseListener); // Clean up listener
                clearTimeout(timeout);
                tvpasOscClient.close();
            };

            oscServer.once(responseAddress, responseListener);

            const timeout = setTimeout(() => {
                logger.error("Timeout: Did not receive /server_time_response from TVPAS.");
                oscServer.removeListener(responseAddress, responseListener);
                if (callback) callback(null);
                tvpasOscClient.close();
            }, 2000);

            const requestMsg = new osc.Message('/get_server_time');
            tvpasOscClient.send(requestMsg, (err) => {
                if (err) {
                    logger.error(`FATAL ERROR sending /get_server_time to TVPAS: ${err}`);
                    clearTimeout(timeout);
                    io.of("/").server.removeListener(responseAddress, responseListener);
                    if (callback) callback(null);
                    tvpasOscClient.close();
                } else {
                    logger.info(`Sent /get_server_time request to TVPAS (${config.tvpas_host}:${config.tvpas_osc_listen_port}).`);
                }
            });
        });

        socket.on("ts_frequency_setup", (data, callback) => {
            logger.info("Received 'ts_frequency_setup'", data);
            state.pilotData = {};
            if (data && data.f) {
                for (let i = 0; i < data.f.length; i++) {
                    const freq = data.f[i];
                    state.pilotData[freq] = { seat: i, band: data.b ? data.b[i] : null, channel: data.c ? data.c[i] : null };
                }
            }
            logger.info("Updated pilot data map:", state.pilotData);
            if (callback) callback(true);
        });

        socket.on("ts_race_stage", (data, callback) => {
            logger.info("Received 'ts_race_stage'", data);
            state.raceState.status = 'racing';
            logger.info("Race state is now ACTIVE.");

            socket.emit("stage_ready", { pi_starts_at_s: data.start_time_s });

            const tvpasOscClient = new osc.Client(config.tvpas_host, config.tvpas_osc_listen_port);
            const tvpasStageReadyMsg = new osc.Message('/stage_ready');
            tvpasStageReadyMsg.append(data.start_time_s);
            tvpasOscClient.send(tvpasStageReadyMsg, (err) => {
                if (err) logger.error(`FATAL ERROR sending /stage_ready to TVPAS: ${err}`);
                else logger.info(`Sent /stage_ready to TVPAS with data: ${data.start_time_s}`);
                tvpasOscClient.close();
            });

            if (callback) callback();

            if (comPort) {
                const startTime = data.start_time_s * 1000;
                const now = Date.now();
                const delay = startTime - now;

                if (delay > 0) {
                    setTimeout(() => {
                        comPort.write('S', (err) => {
                            if (err) {
                                return logger.error('Error writing to COM port: ', err.message);
                            }
                            logger.info('Sent "S" to COM port.');
                        });
                    }, delay);
                } else {
                    comPort.write('S', (err) => {
                        if (err) {
                            return logger.error('Error writing to COM port: ', err.message);
                        }
                        logger.info('Sent "S\\n" to COM port.');
                    });
                }
            }
        });

        socket.on("ts_race_stop", (cb) => {
            setTimeout(() => {
                state.raceState.status = 'stopped';
                logger.info("Race stopped after delay.");
                if (comPort) {
                    comPort.write('E', (err) => {
                        if (err) {
                            return logger.error('Error writing to COM port: ', err.message);
                        }
                        logger.info('Sent "E" to COM port.');
                    });
                }
                if (cb) cb();
            }, state.flickerLength);
        });
        socket.on("ts_race_abort", (cb) => {
            setTimeout(() => {
                state.raceState.status = 'stopped';
                logger.info("Race aborted after delay.");
                if (cb) cb();
            }, state.flickerLength);
        });
        socket.on("disconnect", () => logger.info(`Client disconnected: ${socket.id}`));
    };
}

module.exports = createSocketHandler;
