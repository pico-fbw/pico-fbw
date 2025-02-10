/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

import * as http from "http";
import { MockHandler } from "vite-plugin-mock-server";

import { Flightplan } from "../src/helpers/flightplan";

function send_data(res: http.ServerResponse<http.IncomingMessage>, data: object) {
    res.setHeader("Content-Type", "application/json");
    res.end(JSON.stringify(data));
}

// eslint-disable-next-line prefer-const
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
            values: [1, 0, 400, 1, 9600],
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
let flightplans: { [name: string]: string };
let activeFlightplan = "";
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
            req.on("data", () => {
                dataReceived = true;
                res.statusCode = 200;
                res.setHeader("Content-Type", "application/json");
                res.end(activeFlightplan.toString());
            });
            req.on("end", () => {
                if (!dataReceived) {
                    // No input, return list of all flightplan names
                    send_data(res, {
                        flightplans: Object.keys(flightplans),
                    });
                }
            });
        },
    },
    {
        pattern: "/api/v1/get/info",
        handle: (req, res) => {
            send_data(res, {
                version: "1.0.0",
                version_api: "1.0",
                version_flightplan: "1.0",
                platform: "Simulated Devlopment Platform",
                platform_version: "1.0.0",
                fs_free: 256000,
            });
        },
    },
    {
        pattern: "/api/v1/get/input",
        handle: (req, res) => {
            send_data(res, {
                ail: 0,
                ele: 0,
                rud: 0,
                thr: 0,
                switch: 0,
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
                aahrs: {
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
                batt: [3.1, 3.1, 3.1, 3.1],
            });
        },
    },
    {
        pattern: "/api/v1/set/active",
        handle: (req, res) => {
            req.on("data", (bodyString: string) => {
                const body = JSON.parse(bodyString) as { name: string };
                activeFlightplan = flightplans[body.name];
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
                    changes: { section: string; key: number; value: string }[];
                    save: boolean;
                };
                body.changes.forEach(change => {
                    const section = config.sections.find(s => s.name === change.section);
                    if (section) {
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
                const body = JSON.parse(bodyString) as { flightplan: Flightplan; name: string };
                flightplans[body.name] = JSON.stringify(body.flightplan);
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
