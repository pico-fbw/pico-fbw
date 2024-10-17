/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import L, { LatLng } from "leaflet";
import { useEffect, useRef, useState } from "preact/hooks";
import { useFileDownload, useFileUpload } from "../helpers/hooks";
import {
    Cog6ToothOutline,
    GlobeAmericasOutline,
    MapOutline,
    MapPinOutline,
    PlusOutline,
    Square3Stack3dOutline,
} from "preact-heroicons";
import { Link } from "wouter-preact";

import Alert from "./Alert";
import calculateDistance from "../helpers/calculateDistance";
import classNames from "../helpers/classNames";

import { api } from "../helpers/api";
import { Flightplan, flightplanToMarkers, markersToFlightplan } from "../helpers/flightplan";
import settings from "../helpers/settings";

import "leaflet/dist/leaflet.css";

export interface Marker {
    id: number;
    position: LatLng;
    alt: number;
    speed: number;
    drop: boolean;
}

// Different tilesets (layers) that can be used for the map
const layers = [
    {
        id: 0,
        name: "Google Satellite",
        attribution: "Imagery &copy; Google Satellite Imagery Sources",
        link: "https://mt1.google.com/vt/lyrs=s&x={x}&y={y}&z={z}",
        icon: GlobeAmericasOutline,
    },
    {
        id: 1,
        name: "Google Hybrid",
        attribution: "Imagery &copy; Google Satellite Imagery Sources, Map data &copy; 2023 Google",
        link: "https://mt1.google.com/vt/lyrs=y&x={x}&y={y}&z={z}",
        icon: Square3Stack3dOutline,
    },
    {
        id: 2,
        name: "Google Map",
        attribution: "Map data &copy; 2023 Google",
        link: "https://mt1.google.com/vt/lyrs=m&x={x}&y={y}&z={z}",
        icon: MapOutline,
    },
    {
        id: 3,
        name: "OpenStreetMap",
        attribution: 'Map data &copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a> and contributors',
        link: "https://tile.openstreetmap.org/{z}/{x}/{y}.png",
        icon: MapPinOutline,
    },
];

interface MapProps {
    setIsFocused?: (isFocused: boolean) => void;
}

const Map: preact.FunctionComponent<MapProps> = ({ setIsFocused }) => {
    const [currentAlt, setCurrentAlt] = useState(50);
    const [currentSpeed] = useState(Number(settings.get("defaultSpeed")));

    const [markers, setMarkers] = useState<Marker[]>([]);
    const [editing, setEditing] = useState(-1); // Marker currently being edited
    const polyline = useRef<L.Polyline | null>(null);
    const polylineColor = "#a21caf"; // Color of the line connecting all markers

    const [mapAttribution, setMapAttribution] = useState(layers[0].attribution);
    const [mapLink, setMapLink] = useState(layers[0].link);
    const mapContainer = useRef<HTMLDivElement>(null); // div that will contain the map
    const map = useRef<L.Map | null>(null); // The map itself

    const [error, setError] = useState(""); // Any current errors, to be displayed in an alert
    const [uploaded, setUploaded] = useState(false); // Whether the flightplan has been uploaded

    // Configuration of the icon used to visually display markers
    const markerIcon = L.icon({
        iconUrl: "marker-icon.png",
        shadowUrl: "marker-shadow.png",
        iconSize: [25, 41],
        shadowSize: [41, 41],
        iconAnchor: [12.5, 38],
        shadowAnchor: [12.5, 38],
    });

    /**
     * Uploads a flightplan to the server.
     * @param input the flightplan to upload
     */
    const uploadFlightplan = async (input: string) => {
        let flightplan: Flightplan;
        try {
            flightplan = JSON.parse(input) as Flightplan;
        } catch (e) {
            console.error("Error parsing JSON:", (e as Error).message);
            setError(`Error generating flightplan: ${(e as Error).message}`);
            return;
        }
        try {
            await api("set/flightplan", flightplan).then(() => setUploaded(true));
        } catch (e) {
            setError(`Server error whilst uploading: ${(e as Error).message}`);
        }
    };

    // Configure file upload and download hooks

    const { downloadFile } = useFileDownload({
        filename: "flightplan.json",
        filetype: "application/json",
    });

    const { openFilePicker } = useFileUpload({
        accept: ".json",
        onFileChange: selectedFile => {
            const reader = new FileReader();
            reader.onload = async () => {
                const uploaded = reader.result as string;
                setMarkers(flightplanToMarkers(uploaded));
                await uploadFlightplan(uploaded);
            };
            reader.readAsText(selectedFile);
        },
    });

    /**
     * Enters marker edit mode for the marker with the given ID.
     * @param id the ID of the marker to edit
     */
    const markerEditMode = (id: number) => {
        if (!map.current) {
            return;
        }
        const marker = markers.find(marker => marker.id === id);
        map.current.setView(marker ? marker.position : ({ lat: 0, lng: 0 } as LatLng));
        setEditing(id);
    };

    /**
     * Event handler for when the map is clicked.
     * Adds a new marker at the clicked location.
     * @param e the Leaflet mouse event
     */
    const handleMapClick = (e: L.LeafletMouseEvent) => {
        const { latlng } = e;
        setMarkers(prevMarkers => {
            const newMarker = {
                id: prevMarkers.length + 1,
                position: latlng,
                alt: currentAlt,
                speed: currentSpeed,
                drop: false,
            };
            return [...prevMarkers, newMarker];
        });
    };

    /**
     * Event handler for when a marker is dropped after being dragged.
     * Updates the position of the marker in the state.
     * @param e the Leaflet drag end event
     * @param id the ID of the marker that was dragged
     */
    const handleMarkerDragEnd = (e: L.DragEndEvent, id: number) => {
        const updatedMarkers = markers.map(marker => {
            if (marker.id === id) {
                return {
                    ...marker,
                    position: (e.target as L.Marker).getLatLng(),
                };
            }
            return marker;
        });
        setMarkers(updatedMarkers);
    };

    /**
     * Validates a number and clamps it to a given range.
     * @param value the value to validate
     * @param min the minimum value
     * @param max the maximum value
     * @returns the validated and clamped value
     */
    const validateAndClamp = (value: number, min: number, max: number): number => {
        if (isNaN(value)) {
            return min;
        }
        return Math.min(Math.max(value, min), max);
    };

    // Clear any errors if a successful upload has occurred
    useEffect(() => {
        setError("");
    }, [uploaded]);

    // Rerenders markers and polyline when markers are updated
    useEffect(() => {
        if (!map.current) {
            return;
        }
        // Clear existing markers and polyline
        map.current.eachLayer(layer => {
            if (!(layer instanceof L.TileLayer)) {
                map.current.removeLayer(layer);
            }
        });

        // Add all current markers
        markers.forEach(marker => {
            const { position, id } = marker;
            L.marker(position, { icon: markerIcon, draggable: true })
                .addTo(map.current)
                .on("click", () => markerEditMode(id))
                .on("dragend", e => handleMarkerDragEnd(e, id));
        });

        // Add polyline connecting all markers
        const latLngs = markers.map(marker => marker.position);
        polyline.current = L.polyline(latLngs, { color: polylineColor }).addTo(map.current);

        // Calculate the average position of all markers and lowest possible zoom level that still shows all markers
        const averageLat = markers.reduce((acc, marker) => acc + marker.position.lat, 0) / markers.length;
        const averageLng = markers.reduce((acc, marker) => acc + marker.position.lng, 0) / markers.length;
        const zoom = map.current.getBoundsZoom(L.latLngBounds(latLngs), false);
        // Save these for later, so that when the user returns to the map, they are pretty much right back where they left off
        settings.set("lastMapPosition", `${averageLat},${averageLng}`);
        settings.set("lastMapZoom", zoom.toString());
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [markers]);

    // Rerenders the map when a new tileset is selected
    useEffect(() => {
        if (!map.current) {
            return;
        }
        map.current.eachLayer(layer => {
            if (layer instanceof L.TileLayer) {
                layer.setUrl(mapLink);
            }
        });
        map.current.attributionControl.setPrefix(mapAttribution);
    }, [mapLink, mapAttribution]);

    // Registers event listeners and initializes the map when the component is mounted
    useEffect(() => {
        if (!mapContainer.current) {
            return;
        }
        map.current = L.map(mapContainer.current as HTMLElement, {
            center: [20, 0],
            zoom: 2,
            scrollWheelZoom: true,
            zoomAnimation: true,
        });

        L.tileLayer(mapLink, {
            attribution: mapAttribution,
        }).addTo(map.current);

        map.current.on("click", handleMapClick);
        map.current.on("dragstart", () => setIsFocused(true));
        map.current.on("mousedown", () => setIsFocused(true));
        map.current.on("dragend", () => {
            setTimeout(() => setIsFocused(false), 100);
        });
        map.current.on("mouseup", () => {
            setTimeout(() => setIsFocused(false), 100);
        });
        const index = Number(settings.get("defaultMap"));
        setMapLink(layers[index].link);
        setMapAttribution(layers[index].attribution);

        if (settings.get("lastMapPosition") !== "") {
            const [lat, lng] = settings.get("lastMapPosition").split(",").map(Number);
            map.current.setView([lat, lng], Number(settings.get("lastMapZoom")));
        }
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    // The controls located directly under the map (altitude slider, marker add button, settings button)
    const MapControls = () => {
        const [showSettings, setShowSettings] = useState(false); // Whether the settings dropdown should be visible to the user
        const [renderDropdown, setRenderDropdown] = useState(false); // Whether the settings dropdown should be rendered
        // Visible and rendered are seperated here to allow for a transition to occur before content is unrendered.

        // Show/hide the settings dropdown
        useEffect(() => {
            if (showSettings) {
                setRenderDropdown(true);
            } else {
                // Add a delay before unrendering the dropdown content to allow the transition to finish
                setTimeout(() => setRenderDropdown(showSettings), 100);
            }
        }, [showSettings]);

        return (
            <div className="border-white/5 border-b py-2">
                <div className="flex items-center my-auto h-6">
                    <div className="flex-auto my-auto flex ml-3">
                        <label htmlFor="alt" className="text-gray-300 mr-3 md:hidden">
                            Alt:
                        </label>
                        <label htmlFor="alt" className="text-gray-300 mr-3 hidden md:block">
                            Altitude:
                        </label>
                        {/* FIXME: this slider is very...laggy and I have no clue why. It isn't laggy on the React version..? */}
                        <input
                            type="range"
                            name="alt"
                            min={0}
                            max={400}
                            step={5}
                            className="h-6 rounded-full appearance-none bg-white/5 focus:outline-none focus:ring-2 focus:ring-indigo-500 transition-all px-1"
                            value={currentAlt}
                            onChange={e => setCurrentAlt(Number((e.target as HTMLInputElement).value))}
                            onFocus={() => setIsFocused(true)}
                            onBlur={() => setTimeout(() => setIsFocused(false), 100)}
                        />
                        <input
                            type="text"
                            className="my-auto text-gray-300 ml-3 pr-0 mr-0 transition-all bg-transparent border-0 outline-none focus:outline-none focus:border-b-2 focus:border-indigo-500"
                            value={currentAlt}
                            style={{
                                width: `${currentAlt.toString().length}ch`,
                                minWidth: "2ch",
                                maxWidth: "6ch",
                            }}
                            onFocus={() => setIsFocused(true)}
                            onBlur={e => {
                                const newAlt = validateAndClamp(Number((e.target as HTMLInputElement).value), 0, 400);
                                setCurrentAlt(newAlt);
                                setTimeout(() => setIsFocused(false), 100);
                            }}
                        />
                        <span className="text-gray-300 my-auto">ft</span>
                    </div>
                    <div className="ml-5 sm:mt-0 flex-none mr-3 my-auto flex">
                        <PlusOutline
                            className="text-gray-500 w-6 h-6 mr-2 cursor-pointer hover:text-gray-400 duration-150 transition-all"
                            onClick={() => {
                                if (!map.current) {
                                    return;
                                }
                                const newMarker = {
                                    id: markers.length + 1,
                                    position: map.current.getCenter(),
                                    alt: currentAlt,
                                    speed: currentSpeed,
                                    drop: false,
                                };
                                setMarkers([...markers, newMarker]);
                                setEditing(newMarker.id);
                            }}
                        />

                        <div className="relative inline-block text-left my-auto">
                            <div className="flex">
                                <button onClick={() => setShowSettings(!showSettings)}>
                                    <Cog6ToothOutline className="text-gray-500 w-6 h-6 cursor-pointer hover:text-gray-400 duration-150 transition-all" />
                                </button>
                            </div>
                            <div
                                className={classNames(
                                    showSettings
                                        ? "ease-out duration-100 opacity-100 scale-100"
                                        : "ease-in duration-75 opacity-0 scale-95",
                                    "transition transform",
                                )}
                            >
                                <>
                                    {renderDropdown && (
                                        <div className="absolute py-1 right-0 z-10 divide-y divide-gray-700 mt-2 w-56 origin-top-right rounded-md bg-gray-800 shadow-lg ring-1 ring-black ring-opacity-5 focus:outline-none">
                                            <div className="py-1">
                                                {layers.map(layer => (
                                                    <div key={layer.id}>
                                                        <a
                                                            onClick={() => {
                                                                setMapLink(layer.link);
                                                                setMapAttribution(layer.attribution);
                                                                settings.set("defaultMap", layer.id.toString());
                                                            }}
                                                            className={classNames(
                                                                mapLink === layer.link
                                                                    ? "bg-gray-700 text-gray-100"
                                                                    : "text-gray-400",
                                                                "group flex items-center px-4 py-2 text-sm hover:bg-gray-700 hover:text-gray-100",
                                                            )}
                                                        >
                                                            <layer.icon
                                                                className="mr-3 h-5 w-5 text-gray-400 group-hover:text-gray-300"
                                                                aria-hidden="true"
                                                            />
                                                            {layer.name}
                                                        </a>
                                                    </div>
                                                ))}
                                            </div>
                                            <div className="py-1">
                                                <div>
                                                    <Link
                                                        to={"/settings"}
                                                        className={
                                                            "group flex items-center px-4 py-2 text-sm text-gray-400 hover:bg-gray-700 hover:text-gray-100"
                                                        }
                                                    >
                                                        <Cog6ToothOutline
                                                            className="mr-3 h-5 w-5 text-gray-400 group-hover:text-gray-300"
                                                            aria-hidden="true"
                                                        />
                                                        More Settings
                                                    </Link>
                                                </div>
                                            </div>
                                        </div>
                                    )}
                                </>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        );
    };

    // The dock that appears when a waypoint is being edited
    const EditDock = () => {
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
                                    className="sm:col-span-2 mt-auto rounded-md bg-red-600 px-3 py-2 text-sm font-semibold text-white shadow-sm hover:bg-red-500 focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-red-600"
                                >
                                    Delete
                                </button>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        );
    };

    // The table containing all waypoints
    const WaypointTable = () => {
        return (
            <table className="min-w-full divide-y divide-gray-700">
                <thead>
                    <tr>
                        <th scope="col" className="py-3.5 pl-4 pr-3 text-left text-sm font-semibold text-white sm:pl-0">
                            ID
                        </th>
                        <th scope="col" className="px-3 py-3.5 text-left text-sm font-semibold text-white">
                            <div className="hidden md:block">Latitude</div>
                            <div className="md:hidden">Lat.</div>
                        </th>
                        <th scope="col" className="px-3 py-3.5 text-left text-sm font-semibold text-white">
                            <div className="hidden md:block">Longitude</div>
                            <div className="md:hidden">Lng.</div>
                        </th>
                        <th scope="col" className="px-3 py-3.5 text-left text-sm font-semibold text-white">
                            <div className="hidden md:block">Distance</div>
                            <div className="md:hidden">Dist.</div>
                        </th>
                        <th scope="col" className="px-3 py-3.5 text-left text-sm font-semibold text-white">
                            <div className="hidden md:block">Altitude</div>
                            <div className="md:hidden">Alt.</div>
                        </th>
                        <th scope="col" className="px-3 py-3.5 text-left text-sm font-semibold text-white">
                            Speed
                        </th>
                        <th scope="col" className="relative py-3.5 pl-3 pr-4 sm:pr-0">
                            <span className="sr-only">Edit</span>
                        </th>
                    </tr>
                </thead>
                <tbody className="divide-y divide-gray-800">
                    {markers.map((marker, index) => {
                        let distanceToPrevious = "";
                        if (index > 0) {
                            const previousMarker = markers[index - 1];
                            distanceToPrevious = calculateDistance(
                                marker.position.lat,
                                marker.position.lng,
                                previousMarker.position.lat,
                                previousMarker.position.lng,
                            );
                        }

                        return (
                            <tr key={marker.id}>
                                <td className="whitespace-nowrap py-4 pl-4 pr-3 text-sm font-medium text-white sm:pl-0">
                                    {marker.id}
                                    {marker.drop ? <i> (drop)</i> : ""}
                                </td>
                                <td className="whitespace-nowrap px-3 py-4 text-sm text-gray-300">
                                    <div className="hidden md:block">{marker.position.lat}</div>
                                    <div className="md:hidden">{`${marker.position.lat.toFixed(4)}...`}</div>
                                </td>
                                <td className="whitespace-nowrap px-3 py-4 text-sm text-gray-300">
                                    <div className="hidden md:block">{marker.position.lng}</div>
                                    <div className="md:hidden">{`${marker.position.lng.toFixed(4)}...`}</div>
                                </td>
                                <td className="whitespace-nowrap px-3 py-4 text-sm text-gray-300">{distanceToPrevious}</td>
                                <td className="whitespace-nowrap px-3 py-4 text-sm text-gray-300">
                                    <>{marker.alt}ft</>
                                </td>
                                <td className="whitespace-nowrap px-3 py-4 text-sm text-gray-300">
                                    <>{marker.speed}kts</>
                                </td>
                                <td className="relative whitespace-nowrap py-4 pl-3 pr-4 text-right text-sm font-medium sm:pr-0">
                                    <a
                                        onClick={() => markerEditMode(marker.id)}
                                        className="text-indigo-400 hover:text-indigo-300 cursor-pointer"
                                    >
                                        Edit
                                        <span className="sr-only">, {marker.id}</span>
                                    </a>
                                </td>
                            </tr>
                        );
                    })}
                </tbody>
            </table>
        );
    };

    return (
        <div className="w-full h-full bg-gray-900 relative">
            <div
                id="map"
                className="w-full h-full relative"
                ref={mapContainer}
                style={{ height: "500px", position: "relative" }}
            />
            <MapControls />
            {/* If currently editing a waypoint, display the edit dock between the controls and table/buttons */}
            {editing >= 0 && <EditDock />}
            <div className="mx-auto max-w-7xl px-4 py-10 sm:px-6 lg:px-8">
                <div className="sm:flex sm:items-center">
                    <div className="sm:flex-auto">
                        <h1 className="text-base font-semibold leading-6 text-white">Waypoints</h1>
                        <p className="mt-2 text-sm text-gray-300">A list of all waypoints in your flightplan</p>
                    </div>
                </div>
                <div className="mt-8 flow-root">
                    <div className="-mx-4 -my-2 overflow-x-auto sm:-mx-6 lg:-mx-8">
                        <div className="inline-block w-full min-w-full py-2 align-middle sm:px-6 lg:px-8">
                            {markers.length < 2 ? (
                                <Alert type="info" className="flex mx-4 sm:mx-6 lg:mx-0">
                                    Please create at least 2 waypoints
                                    <span className="hidden md:block">, or&nbsp;</span>
                                    <a className="hidden md:block cursor-pointer hover:text-sky-500" onClick={openFilePicker}>
                                        click to upload a flightplan
                                    </a>
                                </Alert>
                            ) : (
                                <div className="space-y-6">
                                    {error && <Alert type="danger">{error}</Alert>}
                                    {uploaded && (
                                        <Alert
                                            type="success"
                                            onClose={() => setUploaded(false)}
                                            className="mx-4 sm:mx-8 lg:mx-0"
                                        >
                                            Flightplan uploaded successfully!
                                        </Alert>
                                    )}
                                    <WaypointTable />
                                    <div className="relative w-full">
                                        <button
                                            type="button"
                                            // eslint-disable-next-line @typescript-eslint/no-misused-promises
                                            onClick={() => uploadFlightplan(JSON.stringify(markersToFlightplan(markers)))}
                                            className="mt-3 w-full rounded-md bg-white/10 px-2.5 py-1.5 text-sm font-semibold text-white shadow-sm hover:bg-white/20 cursor-pointer"
                                        >
                                            Upload
                                        </button>
                                        <button
                                            type="button"
                                            onClick={() => downloadFile(JSON.stringify(markersToFlightplan(markers)))}
                                            className="mt-3 w-full rounded-md bg-white/10 px-2.5 py-1.5 text-sm font-semibold text-white shadow-sm hover:bg-white/20 cursor-pointer"
                                        >
                                            Download
                                        </button>
                                    </div>
                                </div>
                            )}
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default Map;
