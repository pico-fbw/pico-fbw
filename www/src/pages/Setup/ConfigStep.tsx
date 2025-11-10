/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { useEffect, useState } from "preact/hooks";
import { ChevronLeftOutline, ChevronRightOutline, Cog6ToothOutline } from "preact-heroicons";

import { api } from "helpers/api";
import { GET_CONFIG } from "helpers/apiTypes";

interface ConfigStepProps {
    onNext: () => void;
    onBack: () => void;
    setError: (msg: string) => void;
}

export default function ConfigStep({ onNext, onBack, setError }: ConfigStepProps) {
    const [config, setConfig] = useState<GET_CONFIG | null>(null);
    const [aircraftType, setAircraftType] = useState<"conventional" | "rudderless" | "flying-wing">("rudderless");
    const [autothrottle, setAutothrottle] = useState<boolean>(true);
    const [switchType, setSwitchType] = useState<"two" | "three">("three");
    const [loading, setLoading] = useState(true);

    useEffect(() => {
        api("get/config")
            .then((response) => {
                setConfig(response);
                const general = response.sections.find((s) => s.name === "General");
                if (general) {
                    const controlMode = Number(general.values[0]);
                    console.log(general.values);
                    console.log("controlMode:", controlMode);
                    // Map control mode to aircraft type and autothrottle
                    if (controlMode === 0 || controlMode === 1) {
                        setAircraftType("conventional");
                        setAutothrottle(controlMode === 0);
                    } else if (controlMode === 2 || controlMode === 3) {
                        setAircraftType("rudderless");
                        setAutothrottle(controlMode === 2);
                    } else {
                        setAircraftType("flying-wing");
                        setAutothrottle(controlMode === 4);
                    }
                    setSwitchType(Number(general.values[1]) === 0 ? "two" : "three");
                }
                setLoading(false);
            })
            .catch((e) => {
                setError(`Failed to load configuration: ${e.message}`);
                setLoading(false);
            });
    }, []);

    const handleNext = async () => {
        try {
            setLoading(true);
            // Calculate control mode from aircraft type and autothrottle
            let controlMode: number;
            if (aircraftType === "conventional") {
                controlMode = autothrottle ? 0 : 1;
            } else if (aircraftType === "rudderless") {
                controlMode = autothrottle ? 2 : 3;
            } else {
                controlMode = autothrottle ? 4 : 5;
            }
            await api("set/config", {
                changes: [
                    { section: "General", key: "controlMode", value: String(controlMode) },
                    { section: "General", key: "switchType", value: switchType === "two" ? "0" : "1" },
                ],
                save: true,
            });
            onNext();
        } catch (e) {
            setError(`Failed to save configuration: ${(e as Error).message}`);
            setLoading(false);
        }
    };

    if (loading) {
        return (
            <div className="max-w-2xl mx-auto px-4 py-8 text-center">
                <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-sky-500 mx-auto"></div>
                <p className="text-gray-400 mt-4">Loading configuration...</p>
            </div>
        );
    }

    return (
        <div className="max-w-2xl mx-auto px-4 py-8">
            <div className="mb-8">
                <Cog6ToothOutline className="h-12 w-12 text-sky-500 mx-auto mb-4" />
                <h2 className="text-3xl font-bold text-white text-center mb-2">Aircraft Configuration</h2>
                <p className="text-gray-400 text-center">Tell us about your aircraft</p>
            </div>

            <div className="bg-gray-800 rounded-lg p-8 mb-8">
                <div className="space-y-8 lg:space-y-4">
                    {/* Aircraft Type */}
                    <div className="flex flex-wrap items-center gap-2 text-lg">
                        <span className="text-gray-300">I have a</span>
                        <select
                            value={aircraftType}
                            onChange={(e) => setAircraftType(e.currentTarget.value as any)}
                            className="inline-flex px-4 py-2 rounded-md border-0 bg-gray-700 text-white shadow-sm ring-1 ring-inset ring-gray-600 focus:ring-2 focus:ring-inset focus:ring-sky-500 font-semibold"
                        >
                            <option value="conventional">conventional</option>
                            <option value="rudderless">rudderless</option>
                            <option value="flying-wing">flying wing</option>
                        </select>
                        <span className="text-gray-300">aircraft,</span>
                    </div>

                    {/* Autothrottle */}
                    <div className="flex flex-wrap items-center gap-2 text-lg">
                        <span className="text-gray-300">I</span>
                        <select
                            value={autothrottle ? "do" : "dont"}
                            onChange={(e) => setAutothrottle(e.currentTarget.value === "do")}
                            className="inline-flex px-4 py-2 rounded-md border-0 bg-gray-700 text-white shadow-sm ring-1 ring-inset ring-gray-600 focus:ring-2 focus:ring-inset focus:ring-sky-500 font-semibold"
                        >
                            <option value="do">do</option>
                            <option value="dont">don't</option>
                        </select>
                        <span className="text-gray-300">want autothrottle,</span>
                    </div>

                    {/* Switch Type */}
                    <div className="flex flex-wrap items-center gap-2 text-lg">
                        <span className="text-gray-300">and my flight mode switch has</span>
                        <select
                            value={switchType}
                            onChange={(e) => setSwitchType(e.currentTarget.value as any)}
                            className="inline-flex px-4 py-2 rounded-md border-0 bg-gray-700 text-white shadow-sm ring-1 ring-inset ring-gray-600 focus:ring-2 focus:ring-inset focus:ring-sky-500 font-semibold"
                        >
                            <option value="two">two</option>
                            <option value="three">three</option>
                        </select>
                        <span className="text-gray-300">positions.</span>
                    </div>
                </div>

                <div className="mt-6 pt-6 border-t border-gray-700">
                    <p className="text-sm text-gray-400">
                        You can always change these settings later.
                    </p>
                </div>
            </div>

            <div className="flex justify-between">
                <button
                    onClick={onBack}
                    className="inline-flex items-center px-4 py-2 border border-gray-600 text-sm font-medium rounded-md text-gray-300 bg-gray-800 hover:bg-gray-700 transition-colors duration-150"
                >
                    <ChevronLeftOutline className="mr-2 h-5 w-5" />
                    Back
                </button>
                <button
                    onClick={handleNext}
                    disabled={loading}
                    className="inline-flex items-center px-4 py-2 border border-transparent text-sm font-medium rounded-md text-white bg-sky-600 hover:bg-sky-700 disabled:opacity-50 disabled:cursor-not-allowed transition-colors duration-150"
                >
                    Continue
                    <ChevronRightOutline className="ml-2 h-5 w-5" />
                </button>
            </div>
        </div>
    );
}
