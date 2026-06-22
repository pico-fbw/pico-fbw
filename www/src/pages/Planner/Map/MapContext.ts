/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { createContext, type RefObject } from "preact";
import { useContext } from "preact/hooks";

import type { Marker, MapLayers } from ".";

// Shared context for the map component, to be passed down to child components
interface MapContextValue {
    currentAlt: number;
    setCurrentAlt: (alt: number) => void;

    markers: Marker[];
    setMarkers: (markers: Marker[]) => void;
    editing: number;
    markerEditMode: (id: number) => void;

    mapLink: string;
    setMapLink: (link: string) => void;

    mapRef: RefObject<L.Map>;
    layers: MapLayers;
    speed: RefObject<number>;

    setEditing: (id: number) => void;
    setIsFocused: (isFocused: boolean) => void;
    setMapAttribution: (attribution: string) => void;
}

export const MapContext = createContext<MapContextValue | null>(null);

export function useMapContext() {
    const ctx = useContext(MapContext);
    if (!ctx) {
        throw new Error("useMapContext must be used within a MapContext provider");
    }
    return ctx;
}
