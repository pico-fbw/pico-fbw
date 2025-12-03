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

let config = {
    sections: [
        {
            name: "General",
            values: [2, 1, 20, 50, 50, 1, 0, 0, 1, 0],
        },
        {
            name: "Control",
            values: [25, 15, 1.5, 2, 10, 30, 0.015, 180, 0, 33, 67, -15, 30, 25, 15, 20, 20, 0.5, 1, 1],
        },
        {
            name: "Pins",
            values: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        },
        {
            name: "Sensors",
            values: [400, 1, 9600],
        },
        {
            name: "System",
            values: [1, 0, 0, 0, 0],
        },
        {
            name: "WiFi",
            values: ["pico-fbw", "picodashfbw"],
        },
    ],
};
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
                const body = JSON.parse(bodyString) as { section: string; key: number };
                const section = config.sections.find(s => s.name === body.section);
                if (section) {
                    const key = section.values[body.key];
                    if (key !== undefined) {
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
            req.on("data", (bodyString: string) => {
                const body = JSON.parse(bodyString) as { name: string };
                activeFlightplan = body.name;
                send_data(res, { error: "" });
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
            req.on("data", (bodyString: string) => {
                const body = JSON.parse(bodyString) as {
                    changes: { section: string; key: string; value: string }[];
                    save: boolean;
                };
                body.changes.forEach(change => {
                    const section = config.sections.find(s => s.name === change.section);
                    if (section) {
                        // FIXME: I'm aware that this handling is improper and doesn't work,
                        // but I'm waiting for the config system rewrite to bother fixing it
                        const currentValue = section.values[change.key];
                        if (typeof currentValue === "number") {
                            section.values[change.key] = parseFloat(change.value);
                        } else {
                            section.values[change.key] = change.value;
                        }
                    }
                });
                send_data(res, { error: "" });
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
        pattern: "/api/v1/ping",
        handle: (req, res) => {
            send_data(res, {});
        },
    },
];
