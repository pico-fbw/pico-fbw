/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

/* eslint-disable prefer-const */

import * as http from "http";
import { MockHandler } from "vite-plugin-mock-server";

import { Flightplan } from "helpers/flightplan";

function send_data(res: http.ServerResponse<http.IncomingMessage>, data: object) {
    res.setHeader("Content-Type", "application/json");
    res.end(JSON.stringify(data));
}

const configKeys = {
    General: ["controlMode", "switchType", "maxCalibrationOffset", "servoHz", "escHz", "apiEnabled", "wifiEnabled", "launchAssistEnabled", "autoTuneEnabled", "skipCalibration"],
    Control: ["maxRollRate", "maxPitchRate", "expo", "rudderSensitivity", "controlDeadband", "throttleMaxTime", "throttleCooldownTime", "throttleSensitivity", "dropDetentClosed", "dropDetentOpen", "rollLimit", "rollLimitHold", "pitchLowerLimit", "pitchUpperLimit", "maxAilDeflection", "maxEleDeflection", "maxRudDeflection", "maxElevonDeflection", "elevonMixingGain", "ailMixingBias", "elevMixingBias"],
    Pins: ["inputAil", "servoAil", "inputEle", "servoEle", "inputRud", "servoRud", "inputThrottle", "escThrottle", "inputSwitch", "servoBay", "i2cSda", "i2cScl", "spiClk", "spiMosi", "spiMiso", "spiCs0", "spiCs1", "spiCs2", "gpsTx", "gpsRx", "reverseRoll", "reversePitch", "reverseYaw"],
    Sensors: ["busType", "i2cBusFreq", "spiBusFreq", "gpsCommandType", "gpsBaudrate"],
    System: ["ssid", "pass", "printsys", "printIMU", "printAircraft", "printGPS", "printNetwork"],
} as const;

const configSectionDefaults: { [key: string]: (number | string)[] } = {
    General: [2, 1, 20, 50, 50, 1, 2, 0, 1, 1],
    Control: [50, 20, 0.4, 0.3, 2, 10, 30, 0.3, 180, 0, 33, 67, -15, 30, 60, 60, 40, 20, 0.5, 1, 1],
    Pins: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, -1, -1, 17, 18, 0, 0, 0],
    Sensors: [0, 400, 1, 1, 9600],
    System: ["pico-fbw", "picodashfbw", 1, 0, 0, 0, 0],
};

let config: { sections: { name: string; values: (number | string)[] }[] } = {
    sections: [
        { name: "General", values: [...configSectionDefaults.General] },
        { name: "Control", values: [...configSectionDefaults.Control] },
        { name: "Pins", values: [...configSectionDefaults.Pins] },
        { name: "Sensors", values: [...configSectionDefaults.Sensors] },
        { name: "System", values: [...configSectionDefaults.System] },
    ],
};

function findSection(sectionName: string) {
    return config.sections.find(section => section.name === sectionName);
}

function findValueIndex(sectionName: keyof typeof configKeys | string, key: string) {
    const keys = configKeys[sectionName as keyof typeof configKeys] as readonly string[] | undefined;
    return keys ? keys.indexOf(key) : -1;
}

function applyConfigChange(sectionName: string, key: string, value: string) {
    const section = findSection(sectionName);
    const valueIndex = findValueIndex(sectionName, key);
    if (!section || valueIndex < 0) {
        return false;
    }
    const currentValue = section.values[valueIndex];
    if (typeof currentValue === "number") {
        section.values[valueIndex] = parseFloat(value);
    } else {
        section.values[valueIndex] = value;
    }
    return true;
}

function applyFullConfigUpdate(nextConfig: { sections?: { name: string; values: (number | string)[] }[] }) {
    if (!nextConfig.sections) {
        return;
    }
    nextConfig.sections.forEach(section => {
        const target = findSection(section.name);
        if (!target) {
            return;
        }
        const keys = configKeys[section.name as keyof typeof configKeys] as readonly string[] | undefined;
        if (!keys) {
            return;
        }
        keys.forEach((_, index) => {
            if (index < section.values.length) {
                target.values[index] = section.values[index];
            }
        });
    });
}
let flightplans: { [name: string]: Flightplan } = {
    default: {
        version: "1.0",
        version_fw: "1.0.0",
        alt_samples: 0,
        waypoints: [
            { lat: 35, lng: -140, alt: 100, speed: 20, drop: 0 },
            { lat: 35, lng: 140, alt: 100, speed: 20, drop: 0 },
        ],
    },
};
let activeFlightplan: string | null = null; // Name of currently active flightplan
let mode = "direct";

export default (): MockHandler[] => [
    {
        pattern: "/api/v1/get/config",
        handle: (req, res) => {
            let dataReceived = false;
            req.on("data", (bodyString: string) => {
                dataReceived = true;
                const body = JSON.parse(bodyString) as { section: string; key: string };
                const section = findSection(body.section);
                const keyIndex = findValueIndex(body.section, body.key);
                if (section && keyIndex >= 0) {
                    const key = section.values[keyIndex];
                    send_data(res, {
                        sections: [
                            {
                                name: body.section,
                                values: [key],
                            },
                        ],
                    });
                    return;
                }
                res.statusCode = 400;
                send_data(res, {});
            });
            req.on("end", () => {
                if (!dataReceived) {
                    send_data(res, config);
                }
            });
        },
    },
    {
        pattern: "/api/v1/get/flightplan",
        handle: (req, res) => {
            let dataReceived = false;
            req.on("data", (bodyString: string) => {
                dataReceived = true;
                const body = JSON.parse(bodyString) as { name: string };
                const flightplan = flightplans[body.name];
                if (!flightplan) {
                    res.statusCode = 404;
                    res.end();
                    return;
                }
                send_data(res, flightplan);
            });
            req.on("end", () => {
                if (dataReceived) {
                    return;
                }
                // No input, return list of all flightplan names
                send_data(res, {
                    flightplans: flightplans
                        ? Object.keys(flightplans).map(name => ({
                              name,
                              size: Buffer.byteLength(JSON.stringify(flightplans[name])),
                          }))
                        : [],
                    active: activeFlightplan,
                });
            });
        },
    },
    {
        pattern: "/api/v1/get/info",
        handle: (req, res) => {
            const fs_total = 256000;
            const fs_used = Buffer.byteLength(JSON.stringify(flightplans));
            send_data(res, {
                version: "1.0.0",
                version_api: "1.0",
                version_flightplan: "1.0",
                platform: "Simulated Devlopment Platform",
                platform_version: "1.0.0",
                fs_used,
                fs_total,
            });
        },
    },
    {
        pattern: "/api/v1/get/input",
        handle: (req, res) => {
            send_data(res, {
                ail: 90,
                ele: 90,
                rud: 90,
                thr: 50,
                switch: 90,
            });
        },
    },
    {
        pattern: "/api/v1/get/logs",
        handle: (req, res) => {
            send_data(res, {
                logs: [
                    { type: 1, msg: "This is an info message", code: -1, timestamp: 0 },
                    { type: 2, msg: "This is a warning message", code: 500, timestamp: 30000 },
                    { type: 3, msg: "This is an error message", code: 1000, timestamp: 100000 },
                ],
            });
        },
    },
    {
        pattern: "/api/v1/get/mode",
        handle: (req, res) => {
            send_data(res, {
                mode,
            });
        },
    },
    {
        pattern: "/api/v1/get/sensor",
        handle: (req, res) => {
            send_data(res, {
                gps: {
                    lat: 0,
                    lng: 0,
                    alt: 0,
                    speed: 0,
                    track: 0,
                    pdop: 0,
                    hdop: 0,
                    vdop: 0,
                    sats: 0,
                },
                imu: {
                    roll: 0,
                    pitch: 0,
                    yaw: 0,
                    roll_rate: 0,
                    pitch_rate: 0,
                    yaw_rate: 0,
                    accel_x: 0,
                    accel_y: 0,
                    accel_z: 0,
                },
                batt: [3.1, 3.1, 3.1, 3.1],
            });
        },
    },
    {
        pattern: "/api/v1/set/active",
        handle: (req, res) => {
            let dataReceived = false;
            req.on("data", (bodyString: string) => {
                dataReceived = true;
                const body = JSON.parse(bodyString) as { name: string };
                activeFlightplan = body.name;
                send_data(res, { error: "" });
            });
            req.on("end", () => {
                if (!dataReceived) {
                    res.statusCode = 400;
                    send_data(res, { error: "" });
                }
            });
        },
    },
    {
        pattern: "/api/v1/set/bay",
        handle: (req, res) => {
            send_data(res, {});
        },
    },
    {
        pattern: "/api/v1/set/config",
        handle: (req, res) => {
            let dataReceived = false;
            req.on("data", (bodyString: string) => {
                dataReceived = true;
                const body = JSON.parse(bodyString) as {
                    changes: { section: string; key: string; value: string }[];
                    save: boolean;
                };
                body.changes.forEach(change => {
                    applyConfigChange(change.section, change.key, change.value);
                });
                send_data(res, { error: "" });
            });
            req.on("end", () => {
                if (!dataReceived) {
                    res.statusCode = 400;
                    send_data(res, { error: "" });
                }
            });
        },
    },
    {
        pattern: "/api/v1/set/config_full",
        handle: (req, res) => {
            let dataReceived = false;
            req.on("data", (bodyString: string) => {
                dataReceived = true;
                const body = JSON.parse(bodyString) as { sections?: { name: string; values: (number | string)[] }[]; save: boolean };
                applyFullConfigUpdate(body);
                send_data(res, { error: "" });
            });
            req.on("end", () => {
                if (!dataReceived) {
                    res.statusCode = 400;
                    send_data(res, { error: "" });
                }
            });
        },
    },
    {
        pattern: "/api/v1/set/flightplan",
        handle: (req, res) => {
            req.on("data", (bodyString: string) => {
                const body = JSON.parse(bodyString) as { flightplan?: Flightplan; name: string };
                if (body.flightplan) {
                    flightplans[body.name] = body.flightplan;
                } else {
                    delete flightplans[body.name];
                }
                send_data(res, {});
            });
        },
    },
    {
        pattern: "/api/v1/set/mode",
        handle: (req, res) => {
            req.on("data", (bodyString: string) => {
                const body = JSON.parse(bodyString) as { mode: string };
                mode = body.mode;
                send_data(res, {});
            });
        },
    },
    {
        pattern: "/api/v1/set/target",
        handle: (req, res) => {
            send_data(res, {});
        },
    },
    {
        pattern: "/api/v1/set/waypoint",
        handle: (req, res) => {
            send_data(res, {});
        },
    },
    {
        pattern: "/api/v1/get/calibration",
        handle: (req, res) => {
            let dataReceived = false;
            req.on("data", (bodyString: string) => {
                dataReceived = true;
                const body = JSON.parse(bodyString) as { system: string };
                if (!["receiver", "esc", "imu", "pid"].includes(body.system)) {
                    res.statusCode = 400;
                    send_data(res, {});
                    return;
                }
                send_data(res, { calibrated: false });
            });
            req.on("end", () => {
                if (!dataReceived) {
                    res.statusCode = 400;
                    send_data(res, {});
                }
            });
        },
    },
    {
        pattern: "/api/v1/set/calibration",
        handle: (req, res) => {
            send_data(res, {});
        },
    },
    {
        pattern: "/api/v1/ping",
        handle: (req, res) => {
            send_data(res, {});
        },
    },
    {
        pattern: "/api/v1/reboot",
        handle: (req, res) => {
            send_data(res, {});
        },
    },
];
