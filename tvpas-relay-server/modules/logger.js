const winston = require('winston');

const logger = winston.createLogger({
    level: 'info',
    format: winston.format.combine(
        winston.format.colorize(), // Adds color to the output
        winston.format.timestamp({ format: 'YYYY-MM-DD HH:mm:ss' }),
        winston.format.splat(),    // Enables string interpolation
        winston.format.printf(info => {
            const { timestamp, level, message, ...args } = info;
            const ts = timestamp.slice(0, 19).replace('T', ' ');

            // Convert message and args to a single-line string if they are objects
            let formattedMessage = typeof message === 'object' ? JSON.stringify(message) : message;
            let formattedArgs = Object.keys(args).length ? JSON.stringify(args) : '';

            // Combine message and args, ensuring it's a single line
            let fullMessage = `${formattedMessage} ${formattedArgs}`.trim();

            return `${ts} [${level}]: ${fullMessage}`;
        })
    ),
    transports: [
        new winston.transports.Console()
    ]
});

module.exports = logger;