import { useEffect, useState } from 'preact/hooks';
import Alert from '../elements/Alert';
import hasInternet from '../helpers/hasInternet';
import api from '../helpers/api';

export default function Index() {
    const [hasConnection, setHasConnection] = useState(false);
    const [loading, setLoading] = useState(true);

    const checkInternetConnection = async () => {
        const isConnected = await hasInternet();
        setHasConnection(isConnected);
        setLoading(false);
    };

    useEffect(() => {
        checkInternetConnection();
        // testing
        api('ping').then((res) => {
            if (res.err === 0) {
                console.log('API is working');
            } else {
                console.error('API is not working');
            }
        });
    }, []);

    return (
        <div className="flex flex-col items-center justify-center h-screen">
            {loading ? (
                <div className="fixed top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2">
                    <img src="load.gif" alt="loading" className="h-32 w-32" />
                </div>
            ) : (
                <>
                    <h1 className="text-4xl font-bold mb-4 text-gray-300">Welcome to pico-fbw</h1>
                    {hasConnection ? (
                        <Alert type="info" className="mx-4 sm:mx-8 lg:mx-0">
                            Connected to the internet
                        </Alert>
                    ) : (
                        <Alert type="warning" className="mx-4 sm:mx-8 lg:mx-0">
                            Not connected to the internet
                        </Alert>
                    )}
                </>
            )}
        </div>
    );
}
