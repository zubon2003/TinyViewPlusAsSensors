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

// --- Create OSC Client for TVPAS ---
const tvpasOscClient = new osc.Client(config.tvpas_target.host, config.tvpas_target.port);
logger.info(`OSC client for TVPAS created for target ${config.tvpas_target.host}:${config.tvpas_target.port}`);

// --- Create OSC Client for Camera Switcher (if enabled) ---
let cameraSwitcherOscClient = null;
if (config.osc_forwarding) {
    cameraSwitcherOscClient = new osc.Client(config.camera_switcher_osc_target.host, config.camera_switcher_osc_target.port);
    logger.info(`OSC client for Camera Switcher created for target ${config.camera_switcher_osc_target.host}:${config.camera_switcher_osc_target.port}`);
}


// --- Helper Functions ---
function startServers(comPort) {
    const servers = {};
    const serverConfigs = [
        { role: 'server1', port: config.server1_port, ids: config.server1_id },
        { role: 'server2', port: config.server2_port, ids: config.server2_id },
        { role: 'server3', port: config.server3_port, ids: config.server3_id },
        { role: 'server4', port: config.server4_port, ids: config.server4_id }
    ];

    // Create Socket.IO servers
    serverConfigs.forEach(serverConfig => {
        if (serverConfig.port > 0) {
            const io = new Server(serverConfig.port, { cors: { origin: "*" } });
            servers[serverConfig.role] = io;
            logger.info(`Socket.IO server for '${serverConfig.role}' listening on port ${serverConfig.port}`);
            
            // Pass the role, OSC client, config, COM port, and state to the handler
            io.on("connection", createSocketHandler(serverConfig.role, tvpasOscClient, config, comPort, state, cameraSwitcherOscClient));
        }
    });

    // Create OSC Server to listen to TVPAS
    const oscServer = new osc.Server(config.relay_osc_listen_port, '0.0.0.0');
    oscServer.on('message', createOscHandler(servers, config, comPort, cameraSwitcherOscClient, state)); // Pass all server instances
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


// --- Graceful Shutdown ---
process.on('SIGINT', () => {
    logger.info("Shutting down servers...");
    tvpasOscClient.close();
    if (cameraSwitcherOscClient) { 
        cameraSwitcherOscClient.close();
    }
    // Add any other cleanup logic here
    process.exit(0);
});

