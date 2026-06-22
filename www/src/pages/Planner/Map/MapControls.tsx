/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { useEffect, useState } from "preact/hooks";
import { Cog6ToothOutline, PlusOutline } from "preact-heroicons";
import { Link } from "wouter-preact";

import { useMapContext } from "./MapContext";

import classNames from "helpers/classNames";
import settings from "helpers/settings";
import validateAndClamp from "helpers/validateAndClamp";

import { Marker } from ".";

// The controls located directly under the map (altitude slider, marker add button, settings button)
export default function MapControls() {
    const {
        currentAlt,
        setCurrentAlt,
        markers,
        setMarkers,
        mapLink,
        setMapLink,
        mapRef,
        layers,
        speed,
        setEditing,
        setIsFocused,
        setMapAttribution,
    } = useMapContext();
    const [showSettings, setShowSettings] = useState(false); // Whether the settings dropdown should be visible to the user
    const [renderDropdown, setRenderDropdown] = useState(false); // Whether the settings dropdown should be rendered
    // Visible and rendered are seperated here to allow for a transition to occur before content is unrendered

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
                            if (!mapRef.current) {
                                return;
                            }
                            const newMarker: Marker = {
                                id: markers.length + 1,
                                position: mapRef.current.getCenter(),
                                alt: currentAlt,
                                speed: speed.current ?? 0,
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
}
