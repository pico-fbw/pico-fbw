/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { Spinner } from "elements/Spinner";
import { GET_CONFIG, GET_INPUT } from "helpers/apiTypes";

function Stick({ x, y, label }: { x: number; y: number; label: string }) {
    // Convert -100 to 100 range to percentage position
    const xPos = ((x + 100) / 200) * 100;
    const yPos = ((y + 100) / 200) * 100;
    return (
        <div className="relative">
            <div className="w-32 h-32 bg-gray-900 rounded-lg border-2 border-gray-700 relative overflow-hidden">
                {/* Crosshair */}
                <div className="absolute inset-0 flex items-center justify-center">
                    <div className="w-full h-0.5 bg-gray-700" />
                </div>
                <div className="absolute inset-0 flex items-center justify-center">
                    <div className="w-0.5 h-full bg-gray-700" />
                </div>
                <div className="absolute top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2 w-2 h-2 bg-gray-600 rounded-full" />
                {/* Stick position */}
                <div
                    className="absolute w-6 h-6 bg-sky-500 rounded-full border-2 border-sky-400 shadow-lg transform -translate-x-1/2 -translate-y-1/2 transition-all duration-100"
                    style={{
                        left: `${xPos}%`,
                        top: `${yPos}%`,
                    }}
                >
                    <div className="absolute inset-0 bg-sky-400 rounded-full animate-ping opacity-20" />
                </div>
            </div>
            <p className="text-xs text-gray-400 text-center mt-2">{label}</p>
        </div>
    );
}

function Slider({ value, label }: { value: number; label: string }) {
    return (
        <div className="relative">
            <div className="w-12 h-32 bg-gray-900 rounded-lg border-2 border-gray-700 relative overflow-hidden">
                {/* Track markers */}
                <div className="absolute inset-0 flex flex-col justify-between p-1">
                    {[0, 1, 2, 3, 4].map(i => (
                        <div key={i} className="w-full h-0.5 bg-gray-700" />
                    ))}
                </div>
                {/* Fill */}
                <div
                    className="absolute bottom-0 w-full bg-gradient-to-t from-sky-600 to-sky-500 transition-all duration-100"
                    style={{ height: `${value}%` }}
                />
                {/* Thumb */}
                <div
                    className="absolute left-1/2 transform -translate-x-1/2 w-10 h-4 bg-sky-500 rounded border-2 border-sky-400 shadow-lg transition-all duration-100"
                    style={{ bottom: `calc(${value}% - 8px)` }}
                />
            </div>
            <p className="text-xs text-gray-400 text-center mt-2">{label}</p>
        </div>
    );
}

function Switch({ position, positions, label }: { position: number; positions: number; label: string }) {
    return (
        <div className="relative">
            <div className="w-16 h-32 bg-gray-900 rounded-lg border-2 border-gray-700 relative overflow-hidden p-2">
                <div className="flex flex-col justify-between h-full">
                    <div
                        className={`w-full h-6 rounded flex items-center justify-center text-xs font-bold transition-all duration-150 ${
                            position === 2 ? "bg-sky-500 text-white" : "bg-gray-800 text-gray-600"
                        }`}
                    >
                        UP
                    </div>
                    {positions === 3 && (
                        <div
                            className={`w-full h-6 rounded flex items-center justify-center text-xs font-bold transition-all duration-150 ${
                                position === 1 ? "bg-sky-500 text-white" : "bg-gray-800 text-gray-600"
                            }`}
                        >
                            MID
                        </div>
                    )}
                    <div
                        className={`w-full h-6 rounded flex items-center justify-center text-xs font-bold transition-all duration-150 ${
                            position === 0 ? "bg-sky-500 text-white" : "bg-gray-800 text-gray-600"
                        }`}
                    >
                        DN
                    </div>
                </div>
            </div>
            <p className="text-xs text-gray-400 text-center mt-2">{label}</p>
        </div>
    );
}

interface TransmitterDisplayProps {
    inputData: GET_INPUT | null;
    config: GET_CONFIG | null;
}

export default function TransmitterDisplay({ inputData, config }: TransmitterDisplayProps) {
    // Convert 0-180 degree values to -100 to 100 range
    const normalizeDegreeInput = (value: number) => {
        return ((value - 90) / 90) * 100;
    };
    // Convert throttle 0-100 to 100-0
    const normalizeThrottleInput = (value: number) => {
        return value * -2 + 100;
    };

    if (!inputData || !config) {
        return <Spinner>Waiting for input data...</Spinner>;
    }

    // Determine control mode
    const controlMode = config?.sections.find(s => s.name === "General").values[0] as number;
    const hasRudder = controlMode <= 1;
    const hasThrottle = controlMode % 2 === 0;

    // Get switch type (2-position or 3-position)
    const switchType = config?.sections.find(s => s.name === "General").values[1];
    const switchPositions = Number(switchType) === 0 ? 2 : 3;

    // Calculate switch position based on value
    const getSwitchPosition = (value: number | undefined) => {
        if (value === undefined) {
            return -1;
        }
        if (switchPositions === 2) {
            return value < 90 ? 0 : 2;
        }
        // Three position: 0-60 = 0, 60-120 = 1, 120-180 = 2
        if (value < 45) {
            return 0;
        }
        if (value < 135) {
            return 1;
        }
        return 2;
    };

    return (
        <div className="flex items-center justify-center gap-8">
            {/* Left stick/throttle slider (depending on aircraft) */}
            {hasRudder ? (
                <Stick
                    x={normalizeDegreeInput(inputData.rud)}
                    y={inputData.thr !== undefined ? normalizeThrottleInput(inputData.thr) : 0}
                    label={hasThrottle ? "Yaw / Throttle" : "Yaw"}
                />
            ) : hasThrottle ? (
                <Slider value={inputData.thr} label="Throttle" />
            ) : null}
            {/* Right stick */}
            <Stick
                x={normalizeDegreeInput(inputData.ail)}
                y={normalizeDegreeInput(inputData.ele)}
                label="Roll / Pitch"
            />
            {/* Mode switch */}
            <Switch position={getSwitchPosition(inputData.switch)} positions={switchPositions} label="Switch" />
        </div>
    );
}
