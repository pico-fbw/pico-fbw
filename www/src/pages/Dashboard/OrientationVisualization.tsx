/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { GET_SENSOR } from "helpers/apiTypes";

interface OrientationVisualizationProps {
    sensorData: GET_SENSOR | null;
}
interface AxisReadoutProps {
    label: string;
    value: number;
}

const PITCH_LIMIT = 45;
const PITCH_PIXELS_PER_DEGREE = 1.4;
const ATTITUDE_LADDER = [-30, -20, -10, 10, 20, 30];
const COMPASS_CARDINALS = [
    { label: "N", angle: 0 },
    { label: "E", angle: 90 },
    { label: "S", angle: 180 },
    { label: "W", angle: 270 },
];

const clamp = (value: number, min: number, max: number) => Math.min(max, Math.max(min, value));
const toRadians = (degrees: number) => (degrees * Math.PI) / 180;
const isFiniteNumber = (value: unknown): value is number => typeof value === "number" && Number.isFinite(value);

const normalizeHeading = (yaw: number) => {
    const wrapped = yaw % 360;
    return wrapped < 0 ? wrapped + 360 : wrapped;
};

const polarToCartesian = (angle: number, radius: number) => {
    const theta = toRadians(angle);
    return {
        x: Math.sin(theta) * radius,
        y: -Math.cos(theta) * radius,
    };
};

const formatAngle = (value: number, fractionDigits = 1) => {
    if (!isFiniteNumber(value)) {
        return "--";
    }
    return `${value.toFixed(fractionDigits)}°`;
};

function AxisReadout({ label, value }: AxisReadoutProps) {
    return (
        <div className="rounded-lg border border-gray-700 bg-gray-900/70 p-3 text-center">
            <p className="text-xs font-medium uppercase tracking-wide text-gray-400">{label}</p>
            <p className="mt-1 text-lg font-semibold text-white">{formatAngle(value)}</p>
        </div>
    );
}

function AttitudeIndicator({ roll, pitch }: { roll: number; pitch: number }) {
    const boundedPitch = clamp(pitch, -PITCH_LIMIT, PITCH_LIMIT);
    const horizonOffset = boundedPitch * PITCH_PIXELS_PER_DEGREE;

    return (
        <svg viewBox="0 0 200 200" className="mx-auto h-52 w-52 max-w-full">
            <defs>
                <clipPath id="attitude-clip">
                    <circle cx="100" cy="100" r="86" />
                </clipPath>
            </defs>

            <g clipPath="url(#attitude-clip)">
                <g transform={`translate(100 100) rotate(${roll}) translate(0 ${horizonOffset})`}>
                    <rect x="-220" y="-220" width="440" height="220" fill="#0369a1" />
                    <rect x="-220" y="0" width="440" height="220" fill="#92400e" />
                    <line x1="-220" y1="0" x2="220" y2="0" stroke="#e5e7eb" strokeWidth="2" />

                    {ATTITUDE_LADDER.map(step => (
                        <line
                            key={step}
                            x1="-40"
                            y1={-step * PITCH_PIXELS_PER_DEGREE}
                            x2="40"
                            y2={-step * PITCH_PIXELS_PER_DEGREE}
                            stroke="#d1d5db"
                            strokeWidth="1"
                            opacity="0.75"
                        />
                    ))}
                </g>
            </g>

            <circle cx="100" cy="100" r="86" fill="none" stroke="#4b5563" strokeWidth="3" />
            <line x1="58" y1="100" x2="90" y2="100" stroke="#f8fafc" strokeWidth="3" strokeLinecap="round" />
            <line x1="110" y1="100" x2="142" y2="100" stroke="#f8fafc" strokeWidth="3" strokeLinecap="round" />
            <circle cx="100" cy="100" r="3" fill="#f8fafc" />
            <polygon points="100,24 94,34 106,34" fill="#38bdf8" />
        </svg>
    );
}

function HeadingIndicator({ yaw }: { yaw: number }) {
    const heading = normalizeHeading(yaw);
    const tickAngles = Array.from({ length: 36 }, (_, index) => index * 10);

    return (
        <svg viewBox="0 0 200 200" className="mx-auto h-52 w-52 max-w-full">
            <circle cx="100" cy="100" r="86" fill="#111827" stroke="#4b5563" strokeWidth="3" />

            <g transform={`translate(100 100) rotate(${-heading})`}>
                {tickAngles.map(angle => {
                    const outer = polarToCartesian(angle, 76);
                    const inner = polarToCartesian(angle, angle % 30 === 0 ? 62 : 68);
                    return (
                        <line
                            key={angle}
                            x1={inner.x}
                            y1={inner.y}
                            x2={outer.x}
                            y2={outer.y}
                            stroke={angle % 30 === 0 ? "#e5e7eb" : "#9ca3af"}
                            strokeWidth={angle % 30 === 0 ? "2" : "1"}
                        />
                    );
                })}

                {COMPASS_CARDINALS.map(point => {
                    const labelPos = polarToCartesian(point.angle, 50);
                    return (
                        <text
                            key={point.label}
                            x={labelPos.x}
                            y={labelPos.y + 4}
                            textAnchor="middle"
                            className="fill-gray-100 text-sm font-semibold"
                        >
                            {point.label}
                        </text>
                    );
                })}
            </g>

            <polygon points="100,22 94,34 106,34" fill="#38bdf8" />
            <text x="100" y="104" textAnchor="middle" className="fill-white text-xl font-semibold">
                {heading.toFixed(0)}°
            </text>
            <text x="100" y="122" textAnchor="middle" className="fill-gray-400 text-xs uppercase tracking-wide">
                Heading
            </text>
        </svg>
    );
}

export default function OrientationVisualization({ sensorData }: OrientationVisualizationProps) {
    const roll = isFiniteNumber(sensorData?.imu.roll) ? sensorData.imu.roll : Number.NaN;
    const pitch = isFiniteNumber(sensorData?.imu.pitch) ? sensorData.imu.pitch : Number.NaN;
    const yaw = isFiniteNumber(sensorData?.imu.yaw) ? sensorData.imu.yaw : Number.NaN;

    const hasImuData = Number.isFinite(roll) && Number.isFinite(pitch) && Number.isFinite(yaw);

    return (
        <div className="space-y-4">
            <div className="grid grid-cols-1 gap-4 xl:grid-cols-2">
                <div className="rounded-lg border border-gray-700 bg-gray-900/60 p-4">
                    <AttitudeIndicator
                        roll={Number.isFinite(roll) ? roll : 0}
                        pitch={Number.isFinite(pitch) ? pitch : 0}
                    />
                </div>

                <div className="rounded-lg border border-gray-700 bg-gray-900/60 p-4">
                    <HeadingIndicator yaw={Number.isFinite(yaw) ? yaw : 0} />
                </div>
            </div>

            {!hasImuData && (
                <p className="text-center text-sm text-yellow-400">
                    IMU orientation data is unavailable. Check sensor status before flight.
                </p>
            )}
        </div>
    );
}
