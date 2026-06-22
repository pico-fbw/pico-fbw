/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import L, { LatLng } from "leaflet";
import { useEffect, useRef, useState } from "preact/hooks";
import { GlobeAmericasOutline, MapOutline, MapPinOutline, Square3Stack3dOutline } from "preact-heroicons";

import Alert from "elements/Alert";
import EditDock from "./EditDock";
import { MapContext } from "./MapContext";
import MapControls from "./MapControls";
import WaypointTable from "./WaypointTable";

import { flightplanToMarkers, markersToFlightplan } from "helpers/flightplan";
import settings from "helpers/settings";

import "leaflet/dist/leaflet.css";

// [ ] ETA calculation
// [ ] Better UI for changing speed, etc (move out of settings)

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
        attribution:
            'Map data &copy; <a href="http://www.openstreetmapRef.org/copyright">OpenStreetMap</a> and contributors',
        link: "https://tile.openstreetmapRef.org/{z}/{x}/{y}.png",
        icon: MapPinOutline,
    },
];
export type MapLayers = typeof layers;

interface MapProps {
    json: string;
    setJson: (json: string) => void;
    setIsFocused: (isFocused: boolean) => void;
}

export default function Map({ json, setJson, setIsFocused }: MapProps) {
    const [currentAlt, setCurrentAlt] = useState(100);
    const altRef = useRef(currentAlt);
    const speed = useRef(Number(settings.get("defaultSpeed")));

    const [markers, setMarkers] = useState<Marker[]>([]);
    const [editing, setEditing] = useState(-1); // Marker currently being edited
    const polylineRef = useRef<L.Polyline>(null);
    const polylineColor = "#a21caf"; // Color of the line connecting all markers

    const [mapAttribution, setMapAttribution] = useState(layers[0].attribution);
    const [mapLink, setMapLink] = useState(layers[0].link);
    const mapContainerRef = useRef<HTMLDivElement>(null); // div that will contain the map
    const mapRef = useRef<L.Map>(null); // The map itself

    // Configuration of the icon used to visually display markers
    const markerIcon = L.icon({
        iconUrl: "/marker-icon.png",
        shadowUrl: "/marker-shadow.png",
        iconSize: [25, 41],
        shadowSize: [41, 41],
        iconAnchor: [12.5, 38],
        shadowAnchor: [12.5, 38],
    });

    /**
     * Enters marker edit mode for the marker with the given ID.
     * @param id the ID of the marker to edit
     */
    const markerEditMode = (id: number) => {
        if (!mapRef.current) {
            return;
        }
        const marker = markers.find(marker => marker.id === id);
        mapRef.current.setView(marker ? marker.position : { lat: 0, lng: 0 });
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
                alt: altRef.current,
                speed: speed.current,
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
     * Calculates the center and zoom level of the map based on the given markers.
     * @param markers markers to calculate from
     * @returns a tuple containing the center and zoom level of the map
     */
    const calcCenterZoom = (markers: Marker[]): [LatLng, number] => {
        const latLngs = markers.map(marker => marker.position);
        const averageLat = markers.reduce((acc, marker) => acc + marker.position.lat, 0) / markers.length;
        const averageLng = markers.reduce((acc, marker) => acc + marker.position.lng, 0) / markers.length;
        const zoom = mapRef.current?.getBoundsZoom(L.latLngBounds(latLngs), false) ?? 2;
        return [L.latLng(averageLat, averageLng), zoom];
    };

    // Sync altitude ref with state
    // The ref is needed so that the Leaflet event handlers can access the current altitude value
    useEffect(() => {
        altRef.current = currentAlt;
    }, [currentAlt]);

    // Rerenders markers and polyline when markers are updated
    useEffect(() => {
        if (!mapRef.current) {
            return;
        }
        const m = mapRef.current;

        // Clear existing markers and polyline
        m.eachLayer(layer => {
            if (!(layer instanceof L.TileLayer)) {
                m.removeLayer(layer);
            }
        });

        // Add all current markers
        markers.forEach(marker => {
            const { position, id } = marker;
            L.marker(position, { icon: markerIcon, draggable: true })
                .addTo(m)
                .on("click", () => markerEditMode(id))
                .on("dragend", e => handleMarkerDragEnd(e, id));
        });

        // Add polyline connecting all markers
        const latLngs = markers.map(marker => marker.position);
        polylineRef.current = L.polyline(latLngs, { color: polylineColor }).addTo(m);

        // Update externally managed JSON
        setJson(markersToFlightplan(markers));

        // Save position for later, so that when the user returns to the map, they are back where they left off
        const [center, zoom] = calcCenterZoom(markers);
        const { lat: averageLat, lng: averageLng } = center;
        settings.set("lastMapPosition", `${averageLat},${averageLng}`);
        settings.set("lastMapZoom", zoom.toString());

        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [markers]);

    // Rerenders the map when a new tileset is selected
    useEffect(() => {
        if (!mapRef.current) {
            return;
        }
        mapRef.current.eachLayer(layer => {
            if (layer instanceof L.TileLayer) {
                layer.setUrl(mapLink);
            }
        });
        mapRef.current.attributionControl.setPrefix(mapAttribution);
    }, [mapLink, mapAttribution]);

    // Registers event listeners and initializes the map when the component is mounted
    useEffect(() => {
        if (!mapContainerRef.current || mapRef.current) {
            return;
        }
        mapRef.current = L.map(mapContainerRef.current, {
            center: [20, 0],
            zoom: 2,
            scrollWheelZoom: true,
            zoomAnimation: true,
        });

        L.tileLayer(mapLink, {
            attribution: mapAttribution,
        }).addTo(mapRef.current);

        mapRef.current.on("click", handleMapClick);
        mapRef.current.on("dragstart", () => setIsFocused(true));
        mapRef.current.on("mousedown", () => setIsFocused(true));
        mapRef.current.on("dragend", () => {
            setTimeout(() => setIsFocused(false), 100);
        });
        mapRef.current.on("mouseup", () => {
            setTimeout(() => setIsFocused(false), 100);
        });
        const index = Number(settings.get("defaultMap"));
        setMapLink(layers[index].link);
        setMapAttribution(layers[index].attribution);

        if (json && json !== "{}") {
            // We were given initial flightplan data, load it and center the map on the plan
            const newMarkers = flightplanToMarkers(json);
            setMarkers(newMarkers);
            const [center, zoom] = calcCenterZoom(newMarkers);
            mapRef.current.setView(center, zoom);
        } else if (settings.get("lastMapPosition") !== "") {
            // No JSON given, start at the last known position (if available)
            const [lat, lng] = settings.get("lastMapPosition").split(",").map(Number);
            mapRef.current.setView([lat, lng], Number(settings.get("lastMapZoom")));
        }
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    return (
        <MapContext.Provider
            // Provide the map context to child elements (like controls, edit dock, etc)
            value={{
                currentAlt,
                setCurrentAlt,
                markers,
                setMarkers,
                mapLink,
                setMapLink,
                mapRef,
                layers,
                speed,
                editing,
                setEditing,
                setIsFocused,
                setMapAttribution,
                markerEditMode,
            }}
        >
            <div className="w-full h-full bg-gray-900 relative">
                <div
                    id="map"
                    className="w-full h-full relative"
                    ref={mapContainerRef}
                    style={{ height: "500px", position: "relative" }}
                />
                <MapControls />
                {/* If currently editing a waypoint, display the edit dock between the controls and table/buttons */}
                {editing >= 0 && <EditDock />}
                <div className="mx-auto max-w-7xl px-4 py-10 sm:px-6 lg:px-8">
                    <div className="sm:flex sm:items-center flex-row">
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
                                    </Alert>
                                ) : (
                                    <div className="space-y-6">
                                        <WaypointTable />
                                    </div>
                                )}
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </MapContext.Provider>
    );
}
