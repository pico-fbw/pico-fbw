/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { useEffect, useState } from "preact/hooks";

import ContentBlock from "elements/ContentBlock";

import SystemStatus from "./SystemStatus";
import TransmitterDisplay from "./TransmitterDisplay";

import { api } from "helpers/api";
import { GET_LOGS, GET_SENSOR, GET_INPUT, GET_MODE, GET_CONFIG, GET_INFO } from "helpers/apiTypes";
import settings from "helpers/settings";

function LiveIndicator() {
    return (
        <div className="flex items-center space-x-2 bg-gray-800 rounded-full px-4 py-2 border border-gray-700">
            <div className="relative">
                <div className="w-2 h-2 bg-green-500 rounded-full" />
                <div className="absolute inset-0 w-2 h-2 bg-green-500 rounded-full animate-ping" />
            </div>
            <span className="text-sm font-semibold text-white uppercase tracking-wide">Live</span>
        </div>
    );
}

export default function Dashboard() {
    const [logs, setLogs] = useState<GET_LOGS | null>(null);
    const [sensorData, setSensorData] = useState<GET_SENSOR | null>(null);
    const [inputData, setInputData] = useState<GET_INPUT | null>(null);
    const [mode, setMode] = useState<GET_MODE | null>(null);
    const [config, setConfig] = useState<GET_CONFIG | null>(null);
    const [info, setInfo] = useState<GET_INFO | null>(null);
    const [error, setError] = useState("");

    useEffect(() => {
        // Fetch initial data
        api("get/logs").then(setLogs).catch(console.error);
        api("get/config").then(setConfig).catch(console.error);
        api("get/info").then(setInfo).catch(console.error);
        // Set up polling for real-time data
        const fetchRealtimeData = () => {
            api("get/sensor", { data: "all" }).then(setSensorData).catch(console.error);
            api("get/input").then(setInputData).catch(console.error);
            api("get/mode").then(setMode).catch(console.error);
        };
        // Initial fetch, then poll every second
        fetchRealtimeData();
        const interval = setInterval(fetchRealtimeData, 1000);
        return () => clearInterval(interval);
    }, []);

    return (
        <ContentBlock title="Dashboard" error={error} setError={setError}>
            <div className="min-h-screen bg-gray-900 p-4 sm:p-6 lg:p-8">
                {/* Header */}
                <div className="flex justify-between items-center mb-6">
                    <h1 className="text-3xl font-bold text-white">Let's get flying, {settings.get("pilotName")}!</h1>
                    <LiveIndicator />
                </div>
                {/* System health summary */}
                <div className="mb-6">
                    <SystemStatus sensorData={sensorData} modeData={mode} infoData={info} logsData={logs} />
                </div>
                {/* Main dashboard grid */}
                <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-6">
                    <div className="bg-gray-800 rounded-lg p-6 h-full">
                        <h2 className="text-xl font-bold text-white mb-6">Transmitter Input</h2>
                        <TransmitterDisplay inputData={inputData} config={config} />
                    </div>
                    <div className="bg-gray-800 rounded-lg p-6 h-full">
                        <h2 className="text-xl font-bold text-white mb-6">TODO</h2>
                    </div>
                </div>
            </div>
        </ContentBlock>
    );
}
