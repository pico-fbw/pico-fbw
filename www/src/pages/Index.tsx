import axios from "axios";
import { useEffect, useState } from "preact/hooks";
import hasInternet from "../../helpers/hasInternet";

// Proof of concept.

export default function Index() {
    const [hasConnection, setHasConnection] = useState(false);
    const [heapInfo, setHeapInfo] = useState({});
    const [loading, setLoading] = useState(true);

    const checkInternetConnection = async () => {
        const isConnected = await hasInternet();
        setHasConnection(isConnected);
        setLoading(false);
    };

    const getHeapInfo = async () => {
        const { data } = await axios.post('/api/v1/heap');
        setHeapInfo(data);
    }

    useEffect(() => {
        checkInternetConnection();
        getHeapInfo();
    }, []);
    
    return (
        <div className="flex flex-col items-center justify-center h-screen">
            {loading ? (
                <div className="fixed top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2">
                    <div className="animate-spin rounded-full h-32 w-32 border-t-2 border-b-2 border-white/20" />
                </div>
            ) : (
                <>
                    <h1 className="text-4xl font-bold mb-4 text-gray-300">Welcome to pico-fbw</h1>
                    <p className="text-gray-500 font-bold text-center">
                        {hasConnection ? 'Connected to the internet' : 'Not connected to the internet'}
                    </p>
                    <div className="mt-4">
                        <p className="text-gray-300">Heap Info:</p>
                        <pre className="text-gray-500">{JSON.stringify(heapInfo, null, 2)}</pre>
                    </div>
                </>
            )}
        </div>
    );
}