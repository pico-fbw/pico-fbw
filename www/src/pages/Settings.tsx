import { useEffect, useState } from "preact/hooks";

import Alert from "../elements/Alert";
import ContentBlock from "../elements/ContentBlock";
import ConfigViewer from "../elements/ConfigViewer";

import { api, GET_INFO } from "../helpers/api";
import settings from "../helpers/settings";

function ConfigUI() {
    const [error, setError] = useState("");

    return (
        <div className="divide-y divide-white/5">
            <div className="grid max-w-7xl grid-cols-1 gap-x-8 gap-y-10 px-4 py-16 sm:px-6 md:grid-cols-3 lg:px-8">
                <h2 className="text-2xl font-bold leading-7 text-sky-500 sm:text-3xl sm:tracking-tight sm:col-span-1 my-auto">
                    Configuration
                </h2>
                <div className="sm:col-span-3 space-y-6">
                    {error ? (
                        <Alert type="danger" className="mx-4 sm:mx-8 lg:mx-0">
                            {error}
                        </Alert>
                    ) : (
                        <div className="flex flex-col">
                            <div className="flex-grow">
                                <ConfigViewer setError={setError} />
                            </div>
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
}

function SettingsUI() {
    const [altSamples, setAltSamples] = useState(Number(settings.get("altSamples")));
    const [defaultSpeed, setDefaultSpeed] = useState(Number(settings.get("defaultSpeed")));
    const [dropSecs, setDropSecs] = useState(Number(settings.get("dropSecs")));

    return (
        <div className="divide-y divide-white/5">
            <div className="grid max-w-7xl grid-cols-1 gap-x-8 gap-y-10 px-4 py-16 sm:px-6 md:grid-cols-3 lg:px-8">
                <h2 className="text-2xl font-bold leading-7 text-sky-500 sm:text-3xl sm:tracking-tight sm:col-span-1 my-auto">
                    Settings
                </h2>
                <div className="sm:col-span-3 space-y-6">
                    <h3 className="text-xl font-bold leading-6 text-sky-500">Altitude Offset Samples</h3>
                    <p className="text-sm font-medium text-gray-200 w-auto">
                        The number of altitude readings to take when determining the autopilot&apos;s ground offset altitude.
                        Note that higher values will require additional time before auto mode may be engaged, especially for
                        systems that rely on GPS for altitude.
                    </p>

                    <div>
                        <input
                            type="number"
                            id="setting0"
                            name="setting0"
                            className="block rounded-md border-0 bg-white/5 px-2 py-1.5 text-white shadow-sm ring-1 ring-inset ring-white/10 focus:ring-2 focus:ring-inset focus:ring-indigo-500 sm:text-sm sm:leading-6"
                            value={altSamples}
                            onChange={e => {
                                let setting = Number((e.target as HTMLInputElement).value);
                                if (!isNaN(setting)) {
                                    setting = Math.min(Math.max(Number((e.target as HTMLInputElement).value), 1), 100);
                                    setAltSamples(setting);
                                    settings.set("altSamples", String(setting));
                                }
                            }}
                        />
                    </div>
                </div>
                <div className="sm:col-span-3 space-y-6">
                    <h3 className="text-xl font-bold leading-6 text-sky-500">Default Waypoint Speed</h3>
                    <p className="text-sm font-medium text-gray-200 w-auto">
                        The default speed (in knots) to set at each waypoint. This is measured as the speed over the ground, not
                        as airspeed.
                    </p>

                    <div>
                        <input
                            type="number"
                            id="setting1"
                            name="setting1"
                            className="block rounded-md border-0 bg-white/5 px-2 py-1.5 text-white shadow-sm ring-1 ring-inset ring-white/10 focus:ring-2 focus:ring-inset focus:ring-indigo-500 sm:text-sm sm:leading-6"
                            value={defaultSpeed}
                            onChange={e => {
                                let setting = Number((e.target as HTMLInputElement).value);
                                if (!isNaN(setting)) {
                                    setting = Math.min(Math.max(Number((e.target as HTMLInputElement).value), 1), 100);
                                    setDefaultSpeed(setting);
                                    settings.set("defaultSpeed", String(setting));
                                }
                            }}
                        />
                    </div>
                </div>
                <div className="sm:col-span-3 space-y-6">
                    <h3 className="text-xl font-bold leading-6 text-sky-500">Drop Release Time</h3>
                    <p className="text-sm font-medium text-gray-200 w-auto">
                        The number of seconds the drop mechanism will stay released for, after a drop is initiated.
                        <br />
                        Setting this to -1 will keep the mechanism open indefinetly after a drop occurs.
                    </p>

                    <div>
                        <input
                            type="number"
                            id="setting2"
                            name="setting2"
                            className="block rounded-md border-0 bg-white/5 px-2 py-1.5 text-white shadow-sm ring-1 ring-inset ring-white/10 focus:ring-2 focus:ring-inset focus:ring-indigo-500 sm:text-sm sm:leading-6"
                            value={dropSecs}
                            onChange={e => {
                                let setting = Number((e.target as HTMLInputElement).value);
                                if (!isNaN(setting)) {
                                    setting = Math.min(Math.max(Number((e.target as HTMLInputElement).value), -1), 60);
                                    if (setting === 0) {
                                        setting = -1;
                                    }
                                    setDropSecs(setting);
                                    settings.set("dropSecs", String(setting));
                                }
                            }}
                        />
                    </div>
                </div>
            </div>
        </div>
    );
}

export default function Settings() {
    const [info, setInfo] = useState<GET_INFO | null>(null);

    useEffect(() => {
        api("get/info").then(setInfo).catch(console.error);
    }, []);

    return (
        <ContentBlock title="Settings" loading={!info}>
            <div className="divide-y divide-white/20">
                <SettingsUI />
                <ConfigUI />
            </div>
            <footer className="bg-gray-900 text-gray-500 p-4">
                <div className="w-full max-w-screen-xl mx-auto">
                    <hr className="my-6 border-gray-700" />
                    <span className="block text-sm text-gray-500 text-center">
                        {info ? (
                            <>
                                {info.platform} v{info.platform_version} | {info.version} | API v{info.version_api}
                            </>
                        ) : (
                            <>No device information available</>
                        )}
                    </span>
                </div>
            </footer>
        </ContentBlock>
    );
}
