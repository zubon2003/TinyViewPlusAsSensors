const state = {
    tvpasReady: false,
    raceState: { status: 'stopped' },
    pilotData: {},
    flickerLength: 100, // Default value, will be updated by heartbeat
};

module.exports = state;
