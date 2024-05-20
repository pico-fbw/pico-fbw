import { useEffect, useState } from "preact/hooks";

import ContentBlock from "../elements/ContentBlock";

import { api, GET_INFO } from "../helpers/api";

export default function Info() {
    const [info, setInfo] = useState<GET_INFO | null>(null);

    useEffect(() => {
        api("get/info").then(setInfo).catch(console.error);
    }, []);

    return (
        <ContentBlock title="Info">
            <div className="flex flex-col items-center justify-center h-screen text-gray-400">
                {info && (
                    <div>
                        <h1 className="text-white text-4xl font-bold">Info</h1>
                        <p>
                            <strong>Version:</strong> {info.version}
                        </p>
                        <p>
                            <strong>API version:</strong> {info.version_api}
                        </p>
                        <p>
                            <strong>Flightplan version:</strong> {info.version_flightplan}
                        </p>
                        <p>
                            <strong>Platform:</strong> {info.platform}
                        </p>
                        <p>
                            <strong>Platform version:</strong> {info.platform_version}
                        </p>
                    </div>
                )}
            </div>
        </ContentBlock>
    );
}
