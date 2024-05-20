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

export default (): MockHandler[] => [
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
        pattern: "/api/v1/set/flightplan",
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
