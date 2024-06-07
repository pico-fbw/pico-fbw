/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import { useEffect, useState } from "preact/hooks";
import { useFileUpload } from "../helpers/hooks";

import Alert from "../elements/Alert";
import ContentBlock from "../elements/ContentBlock";

import { api } from "../helpers/api";
import { Flightplan } from "../helpers/flightplan";
import hasInternet from "../helpers/hasInternet";
import settings from "../helpers/settings";

export default function Upload() {
    const [flightplan, setFlightplan] = useState("");
    const [error, setError] = useState("");
    const [uploaded, setUploaded] = useState(false);
    const [hasConnection, setHasConnection] = useState<boolean | null>(null);

    const showOfflineNotice = settings.get("showOfflineNotice") === "1";

    /**
     * Send the flightplan to the server.
     * @param input the flightplan as a JSON string
     */
    const sendFlightplan = async (input: string) => {
        let flightplan: Flightplan;
        try {
            flightplan = JSON.parse(input) as Flightplan;
        } catch (e) {
            console.error("Error parsing JSON:", (e as Error).message);
            setError("Invalid flightplan!");
            return;
        }
        try {
            await api("set/flightplan", flightplan).then(() => setUploaded(true));
        } catch (e) {
            const error = (e as Error).message;
            if (error === "400") {
                setError("Invalid flightplan!");
            } else {
                setError(`Server error whilst uploading: ${error}`);
            }
        }
    };

    // Use the file upload hook to handle file selection
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
        setHasConnection(isConnected);
    };

    useEffect(() => {
        setError("");
    }, [uploaded]);

    useEffect(() => {
        checkInternetConnection().catch(console.error);
    }, []);

    return (
        <ContentBlock title="Upload" loading={hasConnection === null}>
            <div className="flex flex-col items-center justify-center h-screen space-y-4 p-8">
                {!hasConnection && showOfflineNotice && (
                    <Alert type="info" onClose={() => settings.set("showOfflineNotice", "0")} className="mx-4 sm:mx-8 lg:mx-0">
                        It looks like you're offline. You can still upload flightplans that have been pre-generated at&nbsp;
                        <a
                            href="https://pico-fbw.org/tools/planner"
                            target="_blank"
                            rel="noreferrer"
                            className="hover:text-sky-500"
                        >
                            pico-fbw.org
                        </a>
                        .
                    </Alert>
                )}
                <textarea
                    className="p-4 bg-white/5 placeholder:text-gray-400 text-gray-200 ring-1 ring-inset ring-white/10 shadow-sm rounded resize-none w-full h-1/3"
                    placeholder="Paste flightplan here, or click Upload to select a file..."
                    value={flightplan}
                    onInput={e => setFlightplan(e.currentTarget.value)}
                    // Disable native spellcheck and grammarly for user convenience
                    spellCheck={false}
                    data-gramm="false"
                    data-gramm_editor="false"
                    data-enable-grammarly="false"
                />
                {error && (
                    <Alert type="danger" className="mx-4 sm:mx-8 lg:mx-0">
                        {error}
                    </Alert>
                )}
                {uploaded && (
                    <Alert type="success" onClose={() => setUploaded(false)} className="mx-4 sm:mx-8 lg:mx-0">
                        Flightplan uploaded successfully!
                    </Alert>
                )}
                <button
                    className="bg-blue-600 hover:bg-sky-500 text-white font-bold py-2 px-4 rounded mt-4"
                    onClick={() => {
                        if (flightplan) {
                            sendFlightplan(flightplan).catch(console.error);
                        } else {
                            openFilePicker();
                        }
                    }}
                >
                    Upload
                </button>
            </div>
        </ContentBlock>
    );
}
