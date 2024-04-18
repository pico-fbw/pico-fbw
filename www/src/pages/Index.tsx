/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import { useEffect, useState } from 'preact/hooks';

import Alert from '../elements/Alert';
import Map from '../elements/Map';
import Uploader from '../elements/Uploader';

import { api, ERR } from '../helpers/api';
import hasInternet from '../helpers/hasInternet';

export default function Index() {
    const [status, setStatus] = useState('loading');
    const [apiConnection, setAPIConnection] = useState<boolean | null>(null);
    const [hasConnection, setHasConnection] = useState<boolean | null>(null);

    const checkAPIConnection = async () => {
        const res = await api('ping');
        const isConnected = res.err === ERR.OK;
        setAPIConnection(isConnected);
        if (!isConnected) {
            setStatus(`Server error (${res.err})`);
        }
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
        if (status === 'loading' && apiConnection !== null && hasConnection !== null) {
            const delay = Math.floor(Math.random() * (1000 - 500 + 1)) + 500;
            setTimeout(() => {
                setStatus(null);
            }, delay);
        }
    }, [status, apiConnection, hasConnection]);

    return (
        <div className="flex flex-col items-center justify-center h-screen">
            {status ? (
                status === 'loading' ? (
                    <div className="animate-pulse fixed top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2">
                        <img src="icon.svg" alt="loading" className="h-64 w-64" />
                    </div>
                ) : (
                    <Alert type="danger" className="mx-4 sm:mx-8 lg:mx-0">
                        {status}
                    </Alert>
                )
            ) : (
                <>{hasConnection ? <Map /> : <Uploader />}</>
            )}
        </div>
    );
}
