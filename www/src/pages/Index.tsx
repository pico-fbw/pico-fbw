/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { useEffect, useState } from "preact/hooks";
import { Redirect } from "wouter-preact";

import Alert from "elements/Alert";

import { api } from "helpers/api";

export default function Index() {
    const [status, setStatus] = useState("loading");
    const [hasAPIConnection, setHasAPIConnection] = useState<boolean | null>(null);

    /**
     * Check if the API is reachable, and update the state accordingly.
     */
    const checkAPIConnection = async () => {
        // The API will respond with an empty JSON object at the PING endpoint,
        // so if no errors are thrown, the connection is working
        try {
            await api("ping");
        } catch (e) {
            setHasAPIConnection(false);
            setStatus(`Oops! API error (${(e as Error).message})`);
            return;
        }
        setHasAPIConnection(true);
    };

    // Clear the loading status after the connection checks are done
    useEffect(() => {
        if (status === "loading" && hasAPIConnection !== null) {
            const delay = Math.floor(Math.random() * (1000 - 500 + 1)) + 500;
            setTimeout(() => {
                setStatus(null);
            }, delay);
        }
    }, [status, hasAPIConnection]);

    // Check connection on page load
    useEffect(() => {
        void checkAPIConnection();
    }, []);

    return (
        <>
            {status ? (
                // Show a loading icon while connection is being checked
                <div className="flex flex-col items-center justify-center h-screen">
                    {status === "loading" ? (
                        <div className="animate-pulse fixed top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2">
                            <img src="/icon.svg" alt="loading" className="h-64 w-64" />
                        </div>
                    ) : (
                        // Some checks failed
                        <Alert type="danger" className="mx-4 sm:mx-8 lg:mx-0">
                            {status}
                        </Alert>
                    )}
                </div>
            ) : (
                // Preliminary checks passed, redirect to dashboard
                <Redirect to="/dashboard" />
            )}
        </>
    );
}
