/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import L, { LatLng } from "leaflet";
import { useEffect, useRef, useState } from "preact/hooks";
import { GlobeAmericasOutline, MapOutline, MapPinOutline, Square3Stack3dOutline } from "preact-heroicons";

import Alert from "./Alert";

import { api } from "../helpers/api";
import { Flightplan, markersToFlightplan } from "../helpers/flightplan";
import Settings from "../helpers/settings";

import "leaflet/dist/leaflet.css";

export interface Marker {
    id: number;
    position: LatLng;
    alt: number;
    speed: number;
    drop: boolean;
}

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

export default function Map() {
    const [defaultAlt, setDefaultAlt] = useState(20);
    const [defaultSpeed] = useState(Number(Settings.get("defaultSpeed")));

    const [markers, setMarkers] = useState<Marker[]>([]);
    const polyline = useRef<L.Polyline | null>(null);
    const polylineColor = "#a21caf";

    const [mapAttribution, setMapAttribution] = useState(layers[0].attribution);
    const [mapLink, setMapLink] = useState(layers[0].link);
    const mapContainer = useRef(null); // div that will contain the map
    const map = useRef<L.Map | null>(null);

    const [error, setError] = useState("");

    const markerIcon = L.icon({
        iconUrl: "marker-icon.png",
        shadowUrl: "marker-shadow.png",
        iconSize: [25, 41],
        shadowSize: [41, 41],
        iconAnchor: [12.5, 38],
        shadowAnchor: [12.5, 38],
    });

    const uploadFlightplan = async () => {
        let flightplan: Flightplan;
        try {
            flightplan = markersToFlightplan(markers);
        } catch (e) {
            console.error("Error parsing markers:", (e as Error).message);
            setError(`Error generating flightplan: ${(e as Error).message}`);
            return;
        }
        try {
            await api("set/flightplan", flightplan);
        } catch (e) {
            setError(`Server error whilst uploading: ${(e as Error).message}`);
        }
    };

    const handleMapClick = (e: L.LeafletMouseEvent) => {
        const { latlng } = e;
        setMarkers(prevMarkers => {
            const newMarker = {
                id: prevMarkers.length + 1,
                position: latlng,
                alt: defaultAlt,
                speed: defaultSpeed,
                drop: false,
            };
            return [...prevMarkers, newMarker];
        });
    };

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

    useEffect(() => {
        if (mapContainer.current) {
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
        }
    }, []);

    // Rerenders markers and polyline when markers are updated
    useEffect(() => {
        if (map.current) {
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
                    .on("dragend", e => handleMarkerDragEnd(e, id));
            });

            // Add polyline connecting all markers
            const latLngs = markers.map(marker => marker.position);
            polyline.current = L.polyline(latLngs, { color: polylineColor }).addTo(map.current);
        }
    }, [markers]);

    return (
        <>
            <div
                id="map"
                className="w-full h-full relative"
                ref={mapContainer}
                style={{ height: "500px", position: "relative" }}
            />
            {error && (
                <Alert type="danger" className="mt-4 mx-4 sm:mx-8 lg:mx-0">
                    {error}
                </Alert>
            )}
            <button
                className="bg-blue-600 hover:bg-sky-500 text-white font-bold py-2 px-4 rounded mt-4"
                onClick={() => {
                    uploadFlightplan().catch(console.error);
                }}
            >
                Upload
            </button>
        </>
    );
}
