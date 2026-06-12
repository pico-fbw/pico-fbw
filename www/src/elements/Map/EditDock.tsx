/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { LatLng } from "leaflet";

import { useMapContext } from "./MapContext";

import classNames from "helpers/classNames";
import validateAndClamp from "helpers/validateAndClamp";

// The dock that appears when a waypoint is being edited
export default function EditDock() {
    const { editing, markers, setMarkers, setEditing } = useMapContext();

    /**
     * Gets the marker with the given ID.
     * @param id the ID of the marker to get
     * @returns the marker with the given ID, or a default marker if the ID is invalid
     */
    const getMarker = (id: number): { position: LatLng; alt: number; speed: number; drop: boolean } => {
        const marker = markers.find(marker => marker.id === id);

        let latitude = 0,
            longitude = 0,
            altitude = 0;
        if (marker) {
            altitude = Math.min(Math.max(marker.alt, 0), 400);
            latitude = Math.min(Math.max(marker.position.lat, -90), 90);
            longitude = Math.min(Math.max(marker.position.lng, -180), 180);
        }

        return marker
            ? {
                  position: { lat: latitude, lng: longitude } as LatLng,
                  alt: altitude,
                  speed: marker.speed,
                  drop: marker.drop,
              }
            : { position: { lat: 0, lng: 0 } as LatLng, alt: 0, speed: 0, drop: false };
    };

    /**
     * Sets the position, altitude, speed, and drop status of the marker with the given ID.
     * @param id the ID of the marker to set
     * @param lat the new latitude of the marker
     * @param lng the new longitude of the marker
     * @param alt the new altitude of the marker
     * @param speed the new speed at the marker
     * @param drop the new drop status at the marker
     */
    const setMarker = (id: number, lat: number, lng: number, alt: number, speed: number, drop: boolean): void => {
        const updatedMarkers = markers.map(marker => {
            if (marker.id === id) {
                return {
                    ...marker,
                    position: { lat, lng } as LatLng,
                    alt,
                    speed,
                    drop,
                };
            }
            return marker;
        });
        setMarkers(updatedMarkers);
    };

    /**
     * Removes the marker with the given ID.
     * @param id the ID of the marker to remove
     */
    const removeMarker = (id: number) => {
        const updatedMarkers = markers.filter(marker => marker.id !== id);
        const shiftedMarkers = updatedMarkers.map((marker, index) => ({
            ...marker,
            id: index + 1,
        }));
        setMarkers(shiftedMarkers);
        setEditing(-1);
    };

    return (
        <div
            className={classNames(
                editing >= 0 ? "opacity-100" : "opacity-0",
                "transition-opacity duration-250 border-white/5 border-b",
            )}
        >
            <div className="flex transition-all duration-150 border-white/5 border-b">
                <div className="mx-auto my-4">
                    <div className="md:col-span-2">
                        <div className="grid grid-cols-1 gap-x-6 gap-y-8 sm:max-w-3xl sm:grid-cols-12">
                            <h2 className="text-2xl font-bold leading-7 text-white sm:text-3xl sm:tracking-tight sm:col-span-1 my-auto">
                                #{editing}
                            </h2>
                            <div className="sm:col-span-2">
                                <label htmlFor="latitude" className="block text-sm font-medium leading-6 text-white">
                                    Latitude
                                </label>
                                <div className="mt-2">
                                    <input
                                        type="text"
                                        name="latitude"
                                        id="latitude"
                                        className="block w-full rounded-md border-0 bg-white/5 px-2 py-1.5 text-white shadow-sm ring-1 ring-inset ring-white/10 focus:ring-2 focus:ring-inset focus:ring-indigo-500 sm:text-sm sm:leading-6"
                                        value={getMarker(editing ?? -1).position.lat}
                                        onChange={e => {
                                            const newLat = validateAndClamp(
                                                Number((e.target as HTMLInputElement).value),
                                                -90,
                                                90,
                                            );
                                            setMarker(
                                                editing ?? -1,
                                                newLat,
                                                getMarker(editing ?? -1).position.lng,
                                                getMarker(editing ?? -1).alt,
                                                getMarker(editing ?? -1).speed,
                                                getMarker(editing ?? -1).drop,
                                            );
                                        }}
                                    />
                                </div>
                            </div>
                            <div className="sm:col-span-2">
                                <label htmlFor="longitude" className="block text-sm font-medium leading-6 text-white">
                                    Longitude
                                </label>
                                <div className="mt-2">
                                    <input
                                        type="text"
                                        name="longitude"
                                        id="longitude"
                                        className="block w-full rounded-md border-0 bg-white/5 px-2 py-1.5 text-white shadow-sm ring-1 ring-inset ring-white/10 focus:ring-2 focus:ring-inset focus:ring-indigo-500 sm:text-sm sm:leading-6"
                                        value={getMarker(editing ?? -1).position.lng}
                                        onChange={e => {
                                            const newLng = validateAndClamp(
                                                Number((e.target as HTMLInputElement).value),
                                                -180,
                                                180,
                                            );
                                            setMarker(
                                                editing ?? -1,
                                                getMarker(editing ?? -1).position.lat,
                                                newLng,
                                                getMarker(editing ?? -1).alt,
                                                getMarker(editing ?? -1).speed,
                                                getMarker(editing ?? -1).drop,
                                            );
                                        }}
                                    />
                                </div>
                            </div>
                            <div className="sm:col-span-2">
                                <label htmlFor="altitude" className="block text-sm font-medium leading-6 text-white">
                                    Altitude
                                </label>
                                <div className="mt-2">
                                    <input
                                        type="number"
                                        min={0}
                                        max={400}
                                        name="altitude"
                                        id="altitude"
                                        className="block w-full rounded-md border-0 bg-white/5 px-2 py-1.5 text-white shadow-sm ring-1 ring-inset ring-white/10 focus:ring-2 focus:ring-inset focus:ring-indigo-500 sm:text-sm sm:leading-6"
                                        value={getMarker(editing ?? -1).alt}
                                        onChange={e => {
                                            const newAlt = validateAndClamp(
                                                Number((e.target as HTMLInputElement).value),
                                                0,
                                                400,
                                            );
                                            setMarker(
                                                editing ?? -1,
                                                getMarker(editing ?? -1).position.lat,
                                                getMarker(editing ?? -1).position.lng,
                                                newAlt,
                                                getMarker(editing ?? -1).speed,
                                                getMarker(editing ?? -1).drop,
                                            );
                                        }}
                                    />
                                </div>
                            </div>
                            <div className="sm:col-span-2">
                                <label htmlFor="speed" className="block text-sm font-medium leading-6 text-white">
                                    Speed
                                </label>
                                <div className="mt-2">
                                    <input
                                        type="number"
                                        min={1}
                                        max={100}
                                        name="speed"
                                        id="speed"
                                        className="block w-full rounded-md border-0 bg-white/5 px-2 py-1.5 text-white shadow-sm ring-1 ring-inset ring-white/10 focus:ring-2 focus:ring-inset focus:ring-indigo-500 sm:text-sm sm:leading-6"
                                        value={getMarker(editing ?? -1).speed}
                                        onChange={e => {
                                            const newSpeed = validateAndClamp(
                                                Number((e.target as HTMLInputElement).value),
                                                1,
                                                100,
                                            );
                                            setMarker(
                                                editing ?? -1,
                                                getMarker(editing ?? -1).position.lat,
                                                getMarker(editing ?? -1).position.lng,
                                                getMarker(editing ?? -1).alt,
                                                newSpeed,
                                                getMarker(editing ?? -1).drop,
                                            );
                                        }}
                                    />
                                </div>
                            </div>
                            <div className="sm:col-span-1">
                                <label htmlFor="drop" className="block text-sm font-medium leading-6 text-white">
                                    Drop
                                </label>
                                <div className="ml-2 mt-3">
                                    <input
                                        type="checkbox"
                                        name="drop"
                                        id="drop"
                                        className="h-5 w-5 cursor-pointer appearance-none rounded-md border transition-all checked:border-indigo-500 checked:bg-indigo-500 checked:before:bg-indigo-500"
                                        checked={getMarker(editing ?? -1).drop}
                                        onChange={e => {
                                            setMarker(
                                                editing ?? -1,
                                                getMarker(editing ?? -1).position.lat,
                                                getMarker(editing ?? -1).position.lng,
                                                getMarker(editing ?? -1).alt,
                                                getMarker(editing ?? -1).speed,
                                                (e.target as HTMLInputElement).checked,
                                            );
                                        }}
                                    />
                                </div>
                            </div>
                            <button
                                type="button"
                                onClick={() => removeMarker(editing ?? -1)}
                                className="sm:col-span-2 mt-auto rounded-md bg-red-500/60 px-3 py-2 text-sm font-semibold text-white shadow-sm hover:bg-red-500/90 focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-red-600"
                            >
                                Delete
                            </button>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
}
