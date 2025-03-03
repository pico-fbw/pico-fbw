/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { useEffect, useState } from "preact/hooks";
import { useFileDownload, useFileUpload } from "helpers/hooks";
import { useLocation, useRoute } from "wouter-preact";

import ContentBlock from "elements/ContentBlock";
import Explorer from "elements/Explorer";
import Map from "elements/Map";

import { api } from "helpers/api";
import { FlightplanList } from "helpers/apiTypes";
import hasInternet from "helpers/hasInternet";
import { Flightplan } from "helpers/flightplan";

interface NewFlightpanModalProps {
    name: string;
    setName: (name: string) => void;
    setOpen: (open: boolean) => void;
    setLocation: (location: string) => void;
}

function NewFlightpanModal({ name, setName, setOpen, setLocation }: NewFlightpanModalProps) {
    return (
        <div className="fixed inset-0 bg-black/80 bg-opacity-50 flex items-center justify-center z-50">
            <div className="bg-gray-900 p-6 rounded-md shadow-md w-11/12 max-w-md">
                <h2 className="text-xl font-bold mb-4 text-white">New Flightplan</h2>
                <input
                    type="text"
                    value={name}
                    onChange={e => setName(e.currentTarget.value)}
                    placeholder="Enter a name for your flightplan..."
                    className="w-full p-2 mb-4 border-2 border-gray-700 text-gray-300 rounded"
                />
                <div className="flex justify-end">
                    <button
                        onClick={() => {
                            setOpen(false);
                            setName("");
                        }}
                        className="mr-2 px-4 py-2 text-md font-semibold rounded-md shadow-sm text-white bg-gray-500 hover:bg-gray-500/50"
                    >
                        Cancel
                    </button>
                    <button
                        onClick={() => {
                            if (name.trim() !== "") {
                                setLocation(`/planner/${name}`);
                            }
                            setOpen(false);
                            setName("");
                        }}
                        className="px-4 py-2 text-md font-semibold rounded-md shadow-sm text-white bg-sky-500 hover:bg-sky-600"
                    >
                        Create
                    </button>
                </div>
            </div>
        </div>
    );
}

export default function Planner() {
    const [error, setError] = useState("");

    const [, setLocation] = useLocation();
    const [match, params] = useRoute<{ plan: string }>("/planner/:plan");
    const flightplanName = match ? params.plan : null;
    const [flightplan, setFlightplan] = useState<string | null>(null);

    const [fsUsed, setFsUsed] = useState<number | null>(null);
    const [flightplans, setFlightplans] = useState<FlightplanList | null>(null);
    const [hasInternetConnection, setHasInternetConnection] = useState<boolean | null>(null);
    // Keep track of whether the map is focused,
    // so that we don't trigger swipe events when the user is interacting with the map
    const [isMapFocused, setIsMapFocused] = useState(false);

    const [newFlightplanModalOpen, setNewFlightplanModalOpen] = useState(false);
    const [newFlightplanName, setNewFlightplanName] = useState("");

    /**
     * Fetch the list of saved Flightplans from the server.
     */
    const getFlightplanList = async () => {
        try {
            const response = await api("get/flightplan");
            setFlightplans(response as FlightplanList);
        } catch (e) {
            setError(`Couldn't fetch flightplans: ${(e as Error).message}`);
        }
    };

    /**
     * Fetch the filesystem usage from the server.
     */
    const getFsUsed = async () => {
        try {
            const response = await api("get/info");
            setFsUsed(response.fs_used / response.fs_total);
        } catch (e) {
            setError(`Couldn't get filesystem info: ${(e as Error).message}`);
        }
    };

    /**
     * Save a Flightplan to the server.
     * @param input the Flightplan as a JSON string
     */
    const saveFlightplan = async (input: string) => {
        let fplan: Flightplan;
        // First attempt to parse the input as a Flightplan to ensure its validity
        try {
            fplan = JSON.parse(input) as Flightplan;
        } catch (e) {
            setError(`Invalid flightplan! (${(e as Error).message})`);
            return;
        }
        // Now try to send the Flightplan to the server
        try {
            await api("set/flightplan", { flightplan: fplan, name: flightplanName });
        } catch (e) {
            setError(`Server error whilst sending flightplan: ${(e as Error).message}`);
        }
    };

    /**
     * Set a Flightplan as the active Flightplan on the server.
     * @param name the name of the Flightplan to set as active
     */
    const setActiveFlightplan = async (name: string) => {
        try {
            await api("set/active", { name }).then(response => {
                // TODO: make these warnings instead, they aren't really errors
                if (response.error) {
                    setError(response.error);
                }
            });
        } catch (e) {
            setError(`Couldn't set as active: ${(e as Error).message}`);
        }
    };

    // File upload/download hooks
    const { downloadFile } = useFileDownload({
        filename: `${flightplanName}.json`,
        filetype: "application/json",
    });
    const { openFilePicker } = useFileUpload({
        accept: ".json",
        onFileChange: selectedFile => {
            const reader = new FileReader();
            reader.onload = () => {
                // Load file contents into the flightplan state and name into URL
                // Because of our useEffect hooks, this will also result in saving the flightplan to the server
                const uploaded = reader.result as string;
                const name = selectedFile.name.replace(".json", "");
                setLocation(`/planner/${name}`);
                setFlightplan(uploaded);
            };
            reader.readAsText(selectedFile);
        },
    });

    /**
     * Check if the user has an internet connection, and update the state accordingly.
     */
    const checkInternetConnection = async () => {
        const isConnected = await hasInternet();
        setHasInternetConnection(isConnected);
    };

    // Fetch the flightplan from the server when the flightplan name (in URL) changes
    useEffect(() => {
        if (!flightplanName) {
            setFlightplan(null);
            void getFlightplanList();
            return;
        }
        api("get/flightplan", { name: flightplanName })
            .then(response => {
                const fplan = response as Flightplan;
                setFlightplan(JSON.stringify(fplan));
            })
            .catch(e => {
                const err = e as Error;
                if (err.message === "404") {
                    setFlightplan("{}"); // Create a new flightplan
                } else {
                    setError(`Server error whilst fetching flightplan: ${err.message}`);
                }
            });
    }, [flightplanName]);

    // Update the flightplan server-side (auto-save) when the flightplan changes
    useEffect(() => {
        if (!flightplan && flightplan !== "{}") {
            return;
        }
        void saveFlightplan(flightplan);
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [flightplan]);

    // Check internet connection and fetch saved flightplans on page load
    useEffect(() => {
        void checkInternetConnection();
        void getFlightplanList();
        void getFsUsed();
    }, []);

    return (
        <ContentBlock
            title="Planner"
            loading={flightplans === null || hasInternetConnection === null}
            error={error}
            setError={setError}
            ignoreSwipe={isMapFocused}
        >
            {flightplan === null ? (
                // No flightplan selected/being edited, show list of available flightplans
                <div className="flex flex-col h-full">
                    <div className="flex bg-black/10 ring-white/5 ring-1">
                        <h1 className="px-6 place-self-center rounded-md md:text-3xl text-2xl font-bold text-white">
                            Flightplans
                            <span className="md:text-lg text-sm text-gray-400 ml-3">
                                {(fsUsed * 100).toFixed(0)}% used
                            </span>
                        </h1>
                        <div className="flex grow p-4 gap-3 justify-end">
                            <button
                                type="button"
                                onClick={() => setNewFlightplanModalOpen(true)}
                                disabled={hasInternetConnection === false}
                                className="inline-flex items-center px-4 py-2 border border-transparent text-md leading-4 font-semibold rounded-md shadow-sm text-white bg-gray-500 hover:bg-gray-500/50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-sky-600 disabled:opacity-50 disabled:cursor-not-allowed disabled:hover:bg-gray-500"
                            >
                                New
                            </button>
                            <button
                                type="button"
                                onClick={openFilePicker}
                                className="inline-flex items-center px-4 py-2 border border-transparent text-md leading-4 font-semibold rounded-md shadow-sm text-white bg-gray-500 hover:bg-gray-500/50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-sky-600"
                            >
                                Load from File
                            </button>
                        </div>
                    </div>
                    {/* Progress bar-like indicator of fsUsed as divider */}
                    <div
                        className={`h-1 ${fsUsed < 0.8 ? "bg-blue-500" : fsUsed < 0.9 ? "bg-amber-500" : "bg-red-400"}`}
                        style={{ width: `${fsUsed * 100}%` }}
                    />
                    <div className="grow">
                        <Explorer flightplans={flightplans} setFlightplans={setFlightplans} />
                    </div>
                    {newFlightplanModalOpen && (
                        <NewFlightpanModal
                            name={newFlightplanName}
                            setName={setNewFlightplanName}
                            setOpen={setNewFlightplanModalOpen}
                            setLocation={setLocation}
                        />
                    )}
                </div>
            ) : (
                <>
                    <Map json={flightplan} setJson={setFlightplan} setIsFocused={setIsMapFocused} />
                    <div className="fixed bottom-0 left-0 right-0 xl:left-72 p-2 md:p-4 bg-gray-900">
                        <div className="flex flex-col md:flex-row gap-2">
                            <button
                                type="button"
                                // eslint-disable-next-line @typescript-eslint/no-misused-promises
                                onClick={() => setActiveFlightplan(flightplanName)}
                                className="flex-1 rounded-md bg-white/10 px-2.5 py-1.5 text-sm font-semibold text-white shadow-sm hover:bg-white/20 cursor-pointer"
                            >
                                Set as Active
                            </button>
                            <button
                                type="button"
                                onClick={() => downloadFile(flightplan)}
                                className="flex-1 rounded-md bg-white/10 px-2.5 py-1.5 text-sm font-semibold text-white shadow-sm hover:bg-white/20 cursor-pointer"
                            >
                                Save to File
                            </button>
                            <button
                                type="button"
                                onClick={() => setFlightplan(null)}
                                className="flex-1 rounded-md bg-red-500/60 px-2.5 py-1.5 text-sm font-semibold text-white shadow-sm hover:bg-red-500/90 cursor-pointer"
                            >
                                Clear
                            </button>
                        </div>
                    </div>
                </>
            )}
        </ContentBlock>
    );
}
