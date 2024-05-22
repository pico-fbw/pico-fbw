/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import { useEffect, useState } from "preact/hooks";
import { Redirect } from "wouter-preact";

import Alert from "../elements/Alert";

import { api } from "../helpers/api";
import hasInternet from "../helpers/hasInternet";

export default function Index() {
    const [status, setStatus] = useState("loading");
    const [apiConnection, setAPIConnection] = useState<boolean | null>(null);
    const [hasConnection, setHasConnection] = useState<boolean | null>(null);

    const checkAPIConnection = async () => {
        // The API will respond with an empty JSON object at the PING endpoint,
        // so if no errors are thrown, the connection is working
        try {
            await api("ping");
        } catch (e) {
            setAPIConnection(false);
            setStatus(`Oops! API error (${(e as Error).message})`);
            return;
        }
        setAPIConnection(true);
    };

    const checkInternetConnection = async () => {
        const isConnected = await hasInternet();
        setHasConnection(isConnected);
    };

    useEffect(() => {
        checkAPIConnection()
            .then(() => {
                checkInternetConnection().catch(console.error);
            })
            .catch(console.error);
    }, []);

    useEffect(() => {
        if (status === "loading" && apiConnection !== null && hasConnection !== null) {
            const delay = Math.floor(Math.random() * (1000 - 500 + 1)) + 500;
            setTimeout(() => {
                setStatus(null);
            }, delay);
        }
    }, [status, apiConnection, hasConnection]);

    return (
        <>
            {status ? (
                <div className="flex flex-col items-center justify-center h-screen">
                    {status === "loading" ? (
                        <div className="animate-pulse fixed top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2">
                            <img src="icon.svg" alt="loading" className="h-64 w-64" />
                        </div>
                    ) : (
                        <Alert type="danger" className="mx-4 sm:mx-8 lg:mx-0">
                            {status}
                        </Alert>
                    )}
                </div>
            ) : hasConnection ? (
                <Redirect to="/planner" />
            ) : (
                <Redirect to="/upload" />
            )}
        </>
    );
}
