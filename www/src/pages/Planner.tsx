/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

import { useEffect, useState } from "preact/hooks";
import { useFileDownload, useFileUpload } from "helpers/hooks";
import { useRoute } from "wouter-preact";

import ContentBlock from "elements/ContentBlock";
import Explorer from "elements/Explorer";
import Map from "elements/Map";

import { api } from "helpers/api";
import { FlightplanList } from "helpers/apiTypes";
import hasInternet from "helpers/hasInternet";
import { Flightplan } from "helpers/flightplan";

// [ ] Flightplan manager page, user has to select/create/upload a flightplan to edit
// [ ] Allow: upload from local file > planner, download from planner > local file,
//     'send to' plane (both to fs and active), download from plane?

// [ ] ETA calculation
// [ ] Better UI for changing speed, etc (move out of settings)

// TODO: before commit features
// - this file should manage the active json and passed into map as props when needed
// - should be able to handle uploading/downloading (or at least add funcs for it)
// - remove upload component

export default function Planner() {
    const [error, setError] = useState("");

    const [match, params] = useRoute<{ plan: string }>("/planner/:plan");
    const flightplanName = match ? params.plan : null;
    const [flightplan, setFlightplan] = useState<string | null>(null);

    const [flightplans, setFlightplans] = useState<string[] | null>(null);
    const [hasInternetConnection, setHasInternetConnection] = useState<boolean | null>(null);
    // Keep track of whether the map is focused,
    // so that we don't trigger swipe events when the user is interacting with the map
    const [isMapFocused, setIsMapFocused] = useState(false);

    /**
     * Send a Flightplan to the server.
     * @param input the Flightplan as a JSON string
     */
    const sendFlightplan = async (input: string) => {
        // First attempt to parse the input as a Flightplan to ensure its validity
        try {
            JSON.parse(input) as Flightplan;
        } catch {
            setError("Invalid flightplan!");
            return;
        }
        // Now try to send the Flightplan to the server
        try {
            await api("set/flightplan", { flightplan, name: flightplanName });
        } catch (e) {
            setError(`Server error whilst uploading: ${(e as Error).message}`);
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
            reader.onload = async () => {
                const uploaded = reader.result as string;
                setFlightplan(uploaded);
                await sendFlightplan(uploaded);
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
            return;
        }
        api("get/flightplan", { name: flightplanName })
            .then(response => {
                const fplan = response as Flightplan;
                setFlightplan(JSON.stringify(fplan));
            })
            .catch(console.error);
    }, [flightplanName]);

    // Check internet connection and fetch saved flightplans on page load
    useEffect(() => {
        checkInternetConnection().catch(console.error);
        api("get/flightplan")
            .then(response => {
                setFlightplans((response as FlightplanList).flightplans);
            })
            .catch(console.error);
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
                <>
                    <div className="flex justify-end mb-4">
                        <button
                            type="button"
                            onClick={() => setFlightplan("{}")}
                            className="inline-flex items-center px-3 py-1.5 border border-transparent text-sm leading-4 font-medium rounded-md shadow-sm text-white bg-sky-600 hover:bg-sky-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-sky-600"
                        >
                            New
                        </button>
                    </div>
                    <Explorer flightplans={flightplans} />
                </>
            ) : (
                <>
                    <Map json={flightplan} setJson={setFlightplan} setIsFocused={setIsMapFocused} />
                    <div className="fixed bottom-0 left-0 right-0 xl:left-72 p-2 md:p-4 bg-gray-900">
                        <div className="flex flex-col md:flex-row gap-2">
                            <button
                                type="button"
                                // eslint-disable-next-line @typescript-eslint/no-misused-promises
                                onClick={() => sendFlightplan(flightplan)}
                                className="flex-1 rounded-md bg-white/10 px-2.5 py-1.5 text-sm font-semibold text-white shadow-sm hover:bg-white/20 cursor-pointer"
                            >
                                Send to Plane
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
                                onClick={openFilePicker}
                                className="flex-1 rounded-md bg-white/10 px-2.5 py-1.5 text-sm font-semibold text-white shadow-sm hover:bg-white/20 cursor-pointer"
                            >
                                Load from File
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
