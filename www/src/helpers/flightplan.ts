/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import { LatLng } from "leaflet";
import { Marker } from "../elements/Map";
import settings from "./settings";

const generatorVersion = "1.0";
const firmwareVersion = "1.0.0"; // This should match PICO_FBW_VERSION in the root CMakelists.txt

// Copy of Waypoint struct in src/modes/auto.h
export interface Waypoint {
    lat: number;
    lng: number;
    alt: number;
    speed: number;
    drop: number;
}

// Copy of Flightplan struct in src/sys/flightplan.h
export interface Flightplan {
    version: string;
    version_fw: string;
    alt_samples: number;
    waypoints: Waypoint[];
    // waypoint_count is not included in JSON; it is set when parsed
}

export function markersToFlightplan(markers: Marker[]): Flightplan {
    const altSamples = Number(settings.get("altSamples"));
    const dropSecs = Number(settings.get("dropSecs"));

    const waypoints: Waypoint[] = markers.map(marker => {
        if (marker.position.lat <= -90 || marker.position.lat >= 90) {
            throw new Error("Invalid latitude");
        }
        if (marker.position.lng <= -180 || marker.position.lng > 180) {
            throw new Error("Invalid longitude");
        }
        if (marker.alt < 0 || marker.alt > 400) {
            throw new Error("Invalid altitude");
        }

        return {
            lat: marker.position.lat,
            lng: marker.position.lng,
            alt: marker.alt,
            speed: marker.speed,
            drop: marker.drop ? dropSecs : 0,
        };
    });

    return {
        version: generatorVersion,
        version_fw: firmwareVersion,
        alt_samples: altSamples,
        waypoints,
    };
}

export function flightplanToMarkers(json: string): Marker[] {
    const parsed: Flightplan = JSON.parse(json) as Flightplan;
    if (parsed.version !== generatorVersion) {
        throw new Error("Invalid JSON version");
    }

    return parsed.waypoints.map((waypoint: Waypoint, index: number) => ({
        id: index + 1,
        position: {
            lat: waypoint.lat,
            lng: waypoint.lng,
        } as LatLng,
        alt: waypoint.alt,
        speed: waypoint.speed,
        drop: waypoint.drop > 0,
    }));
}
