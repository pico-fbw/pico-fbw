/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import * as http from "http";
import { MockHandler } from "vite-plugin-mock-server";

function send_data(res: http.ServerResponse<http.IncomingMessage>, data: object) {
    res.setHeader("Content-Type", "application/json");
    res.end(JSON.stringify(data));
}

const configData = {
    sections: [
        {
            name: "General",
            keys: [0, 1, 20, 50, 50, 1, 0, 1],
        },
        {
            name: "Control",
            keys: [25, 15, 1.5, 2, 10, 30, 0.015, 180, 0, 33, 67, -15, 30, 25, 15, 20, 20, 0.5, 1, 1],
        },
        {
            name: "Pins",
            keys: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        },
        {
            name: "Sensors",
            keys: [1, 0, 400, 1, 9600],
        },
        {
            name: "System",
            keys: [1, 1, 0, 0, 0, 0],
        },
        {
            name: "WiFi",
            keys: ["pico-fbw", "picodashfbw"],
        },
    ],
};

export default (): MockHandler[] => [
    {
        pattern: "/api/v1/get/config",
        handle: (req, res) => {
            let dataReceived = false;
            req.on("data", (bodyString: string) => {
                dataReceived = true;
                const body = JSON.parse(bodyString) as { section: string; key: number };
                const section = configData.sections.find(s => s.name === body.section);
                if (section) {
                    const key = section.keys[body.key];
                    if (key !== undefined) {
                        send_data(res, {
                            sections: [
                                {
                                    name: body.section,
                                    keys: [key],
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
                    send_data(res, configData);
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
            });
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
                    const section = configData.sections.find(s => s.name === change.section);
                    if (section) {
                        const currentValue = section.keys[change.key];
                        if (typeof currentValue === "number") {
                            section.keys[change.key] = parseFloat(change.value);
                        } else {
                            section.keys[change.key] = change.value;
                        }
                    }
                });
                send_data(res, {});
            });
        },
    },
    {
        pattern: "/api/v1/set/flightplan",
        handle: (req, res) => {
            send_data(res, {
                message: "",
            });
        },
    },
    {
        pattern: "/api/v1/ping",
        handle: (req, res) => {
            send_data(res, {});
        },
    },
];
