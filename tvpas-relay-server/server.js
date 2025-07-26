const { Server } = require("socket.io");
const osc = require('node-osc');
const fs = require('fs');
const { SerialPort } = require('serialport');

const logger = require('./modules/logger.js');
const state = require('./modules/state.js');
const createSocketHandler = require('./modules/socketHandler.js');
const createOscHandler = require('./modules/oscHandler.js');

// --- Load Configuration ---
let config;
try {
    const configRaw = fs.readFileSync('config.json');
    config = JSON.parse(configRaw);
    logger.info("Configuration loaded successfully.");
} catch (error) {
    logger.error("FATAL: Could not read or parse config.json. Please ensure it exists and is valid JSON.");
    logger.error(error);
    process.exit(1); // Exit if config is missing or invalid
}

// --- Helper Functions ---
function startServers(comPort) {
    const io = new Server(config.socket_io_port, { cors: { origin: "*" } });
    const oscServer = new osc.Server(config.relay_osc_listen_port, '0.0.0.0');

    io.on("connection", createSocketHandler(io, oscServer, config, comPort));
    oscServer.on('message', createOscHandler(io, oscServer, comPort));

    logger.info(`Socket.IO server listening on port ${config.socket_io_port}`);
    logger.info(`OSC server listening on 0.0.0.0:${config.relay_osc_listen_port}`);
    logger.info("Waiting for the first heartbeat from tinyviewplusAsSensors...");
}

function waitForExit() {
    logger.info("Press any key to exit.");
    if (process.stdin.isTTY) {
        process.stdin.setRawMode(true);
        process.stdin.resume();
        process.stdin.on('data', process.exit.bind(process, 0));
    }
}

// --- Main Application Logic ---
if (config.useComPort) {
    try {
        const comPort = new SerialPort({ path: config.comPort, baudRate: 115200 });

        comPort.on('open', () => {
            logger.info(`Successfully opened COM port: ${config.comPort}`);
            startServers(comPort);
        });

        comPort.on('error', (err) => {
            logger.error(`FATAL: Could not open COM port ${config.comPort}. Please check the port name and permissions.`);
            logger.error(err.message);
            logger.error("Server startup aborted.");
            waitForExit();
        });

    } catch (error) {
        logger.error(`FATAL: Error creating COM port instance for ${config.comPort}.`);
        logger.error(error);
        logger.error("Server startup aborted.");
        waitForExit();
    }
} else {
    // Start servers without a COM port if useComPort is false
    startServers(null);
}
