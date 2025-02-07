/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

import { Flightplan } from "./flightplan";

type EmptyResponse = Record<string, never>;

// GET endpoints
export type GET_CONFIG = {
    sections: {
        name: string;
        values: (number | string)[];
    }[];
};
export type GET_FLIGHTPLAN = Flightplan;
export type GET_INFO = {
    version: string;
    version_api: string;
    version_flightplan: string;
    platform: string;
    platform_version: string;
};
export type GET_INPUT = {
    ail: number;
    ele: number;
    rud?: number;
    thr?: number;
    switch?: number;
};
export type GET_LOGS = {
    logs: {
        type: number;
        msg: string;
        code: number;
        timestamp: number;
    }[];
};
export type GET_MODE = {
    mode: "launch" | "direct" | "normal" | "auto" | "tune" | "hold";
};
export type GET_SENSOR = {
    aahrs: {
        roll: number | null;
        pitch: number | null;
        yaw: number | null;
        roll_rate: number | null;
        pitch_rate: number | null;
        yaw_rate: number | null;
        accel_x: number | null;
        accel_y: number | null;
        accel_z: number | null;
    };
    gps: {
        lat: number | null;
        lng: number | null;
        alt: number | null;
        speed: number | null;
        track: number | null;
        pdop: number | null;
        hdop: number | null;
        vdop: number | null;
        sats: number | null;
    };
    batt?: number[];
};

// SET endpoints
export type SET_BAY = EmptyResponse;
export type SET_CONFIG = {
    error?: string;
};
export type SET_FLIGHTPLAN = {
    error?: string;
};
export type SET_MODE = EmptyResponse;
export type SET_TARGET = EmptyResponse;
export type SET_WAYPOINT = EmptyResponse;

// MISC endpoints
export type PING = EmptyResponse;

// Maps API URIs to their respective response types
export type EndpointMap = {
    "get/config": GET_CONFIG;
    "get/flightplan": GET_FLIGHTPLAN;
    "get/info": GET_INFO;
    "get/logs": GET_LOGS;
    "set/config": SET_CONFIG;
    "set/flightplan": SET_FLIGHTPLAN;
    ping: PING;
};
