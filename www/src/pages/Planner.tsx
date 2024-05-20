import { useEffect, useState } from "preact/hooks";

import Alert from "../elements/Alert";
import ContentBlock from "../elements/ContentBlock";
import Map from "../elements/Map";

import hasInternet from "../helpers/hasInternet";

export default function Planner() {
    const [hasConnection, setHasConnection] = useState<boolean | null>(null);

    const checkInternetConnection = async () => {
        const isConnected = await hasInternet();
        setHasConnection(isConnected);
    };

    useEffect(() => {
        checkInternetConnection().catch(console.error);
    }, []);

    return (
        <ContentBlock title="Planner">
            {hasConnection ? (
                <Map />
            ) : (
                <div className="flex flex-col items-center justify-center h-screen">
                    <Alert type="danger" className="mx-4 sm:mx-8 lg:mx-0">
                        An internet connection is required to use the flight planner.
                    </Alert>
                </div>
            )}
        </ContentBlock>
    );
}
