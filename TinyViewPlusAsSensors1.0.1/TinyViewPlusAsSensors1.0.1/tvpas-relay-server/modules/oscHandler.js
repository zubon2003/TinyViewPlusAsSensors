const state = require('./state.js');
const logger = require('./logger.js');

function handleHeartbeat(io, msgArray) {
    if (!state.tvpasReady) {
        state.tvpasReady = true;
        logger.info("First heartbeat received! tinyviewplusAsSensors is now connected.");
        logger.info("The relay is now active and will accept connections from FPVTrackside.");
    }

    // Forward the heartbeat data to FPVTrackside
    const jsonString = msgArray[1]; // Get the JSON payload from the OSC message
    if (typeof jsonString === 'string') {
        try {
            const heartbeat_data = JSON.parse(jsonString);
            if (heartbeat_data.flicker_length) {
                state.flickerLength = heartbeat_data.flicker_length;
                delete heartbeat_data.flicker_length; // Remove flicker_length before emitting
            }
            if (heartbeat_data.loop_time && Array.isArray(heartbeat_data.loop_time)) {
                heartbeat_data.loop_time = heartbeat_data.loop_time.map(Math.round);
            }
            io.emit('heartbeat', heartbeat_data);
        } catch (e) {
            logger.error(`Error parsing heartbeat JSON: ${e.message}`);
        }
    }
}

function handleLapData(io, msgArray, comPort) {
    if (state.raceState.status !== 'racing') {
        logger.warn(`Ignoring lap data because race status is '${state.raceState.status}', not 'racing'.`);
        return;
    }

    const [lapTime, frequency, peakRssi, markerIds] = msgArray.slice(1);
    const pilot = state.pilotData[frequency];

    if (!pilot) {
        logger.warn(`Ignoring lap for unassigned frequency: ${frequency}`);
        return;
    }

    const lapData = {
        seat: pilot.seat,
        frequency: frequency,
        peak_rssi: peakRssi,
        lap_time: lapTime
    };

    io.emit('ts_lap_data', lapData);
    logger.info("Emitted 'ts_lap_data' to FPVTrackside:", lapData);

    if (comPort && comPort.isOpen) {
        const seat = pilot.seat.toString();
        comPort.write(seat, (err) => {
            if (err) {
                // Log the error but don't crash. The port might have closed just now.
                return logger.error('Error writing to COM port: ', err.message);
            }
            logger.info(`Sent "${seat}" to COM port.`);
        });
    } else if (comPort) {
        // Optional: Log a warning if the port exists but is closed.
        logger.warn(`COM port ${comPort.path} is not open. Cannot send lap data.`);
    }

    if (typeof markerIds === 'string') {
        logger.info(`Lap detected for frequency ${frequency} with ArUco Marker ID: ${markerIds}`);
    }
}

function handleServerTimeResponse(msgArray, oscServer, tempCallback) {
    const serverTime = msgArray[1];
    logger.info(`Received /server_time_response from TVPAS with time: ${serverTime}`);
    if (tempCallback) {
        tempCallback(serverTime);
    }
    // This is a one-time listener, it will be removed in socketHandler
}

function createOscHandler(io, oscServer, comPort) {
    return (msgArray) => {
        const address = msgArray[0];

        // Check if there are temporary, specific listeners for this address (e.g., for time sync)
        if (oscServer.listenerCount(address) > 0) {
            oscServer.emit(address, msgArray);
            return;
        }

        // Handle general, persistent messages
        switch (address) {
            case '/heartbeat':
                handleHeartbeat(io, msgArray);
                break;
            case '/ts_lap_data':
                handleLapData(io, msgArray, comPort);
                break;
            default:
                // Reduce noise by not logging unknown messages
                break;
        }
    };
}

module.exports = createOscHandler;
