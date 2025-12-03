/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { useEffect, useState } from "preact/hooks";
import { ChevronLeftOutline, ChevronRightOutline, BeakerOutline } from "preact-heroicons";

import Alert from "elements/Alert";
import { Spinner } from "elements/Spinner";

import { api } from "helpers/api";
import { GET_SENSOR } from "helpers/apiTypes";

interface CalibrationStepProps {
    onNext: () => void;
    onBack: () => void;
    setError: (msg: string) => void;
}

// TODO: add pwm, esc calibration
export default function CalibrationStep({ onNext, onBack, setError }: CalibrationStepProps) {
    const [sensorData, setSensorData] = useState<GET_SENSOR | null>(null);
    const [calibrating, setCalibrating] = useState(false);
    const [calibrationStep, setCalibrationStep] = useState(0);

    const calibrationSteps = [
        {
            title: "Level Position",
            instruction: "Place your aircraft on a level surface and keep it completely still.",
        },
        {
            title: "Nose Up",
            instruction: "Tilt the nose up at 45 degrees and hold steady.",
        },
        {
            title: "Nose Down",
            instruction: "Tilt the nose down at 45 degrees and hold steady.",
        },
        {
            title: "Right Wing Down",
            instruction: "Roll the aircraft 45 degrees to the right and hold steady.",
        },
        {
            title: "Left Wing Down",
            instruction: "Roll the aircraft 45 degrees to the left and hold steady.",
        },
    ];

    useEffect(() => {
        api("get/sensor", { data: "all" }).then(setSensorData).catch(console.log);
    }, []);

    const startCalibration = () => {
        setCalibrating(true);
        setCalibrationStep(0);
    };

    const nextCalibrationStep = () => {
        if (calibrationStep < calibrationSteps.length - 1) {
            setCalibrationStep(calibrationStep + 1);
        } else {
            // Calibration complete
            setCalibrating(false);
            // TODO: when implemented, trigger calibration via API
        }
    };

    if (!sensorData) {
        return <Spinner>Checking sensors...</Spinner>;
    }

    const imuAvailable = sensorData.imu.roll !== null;

    return (
        <div className="max-w-2xl mx-auto px-4 py-8">
            <div className="mb-8">
                <BeakerOutline className="h-12 w-12 text-sky-500 mx-auto mb-4" />
                <h2 className="text-3xl font-bold text-white text-center mb-2">System Calibration</h2>
                <p className="text-gray-400 text-center">Calibrate your systems for accurate flight control</p>
            </div>

            {!imuAvailable && (
                <Alert type="warning" className="mb-6">
                    IMU sensors not detected. Please check your sensor connections and restart the system.
                </Alert>
            )}

            {!calibrating ? (
                <div className="space-y-6 mb-8">
                    {/* Sensor status */}
                    <div className="bg-gray-800 rounded-lg p-6">
                        <h3 className="text-lg font-semibold text-white mb-4">Sensor Status</h3>
                        <div className="grid grid-cols-2 gap-4">
                            <div className="bg-gray-900 rounded p-4">
                                <p className="text-sm text-gray-400 mb-1">IMU</p>
                                <p className="text-lg font-semibold text-white">
                                    {imuAvailable ? (
                                        <span className="text-green-500">✓ Connected</span>
                                    ) : (
                                        <span className="text-red-500">✗ Not Detected</span>
                                    )}
                                </p>
                            </div>
                            <div className="bg-gray-900 rounded p-4">
                                <p className="text-sm text-gray-400 mb-1">GPS</p>
                                <p className="text-lg font-semibold text-white">
                                    {sensorData.gps && sensorData.gps.lat !== null ? (
                                        <span className="text-green-500">✓ Connected</span>
                                    ) : (
                                        <span className="text-yellow-500">○ Optional</span>
                                    )}
                                </p>
                            </div>
                        </div>
                    </div>

                    {/* Calibration info */}
                    <div className="bg-gray-800 rounded-lg p-6">
                        <h3 className="text-lg font-semibold text-white mb-3">About Calibration</h3>
                        <p className="text-gray-400 mb-4">
                            IMU calibration ensures accurate attitude readings during flight. The process takes about 2
                            minutes and requires you to position your aircraft in several orientations.
                        </p>
                        <button
                            onClick={startCalibration}
                            disabled={!imuAvailable}
                            className="w-full inline-flex justify-center items-center px-4 py-3 border border-transparent text-sm font-medium rounded-md text-white bg-sky-600 hover:bg-sky-700 disabled:opacity-50 disabled:cursor-not-allowed transition-colors duration-150"
                        >
                            <BeakerOutline className="mr-2 h-5 w-5" />
                            Start Calibration
                        </button>
                    </div>
                </div>
            ) : (
                // Calibration process
                <div className="mb-8">
                    <div className="bg-gray-800 rounded-lg p-6 mb-6">
                        <div className="mb-4">
                            <div className="flex justify-between text-sm text-gray-400 mb-2">
                                <span>
                                    Step {calibrationStep + 1} of {calibrationSteps.length}
                                </span>
                                <span>{Math.round(((calibrationStep + 1) / calibrationSteps.length) * 100)}%</span>
                            </div>
                            <div className="w-full bg-gray-700 rounded-full h-2">
                                <div
                                    className="bg-sky-600 h-2 rounded-full transition-all duration-300"
                                    style={{
                                        width: `${((calibrationStep + 1) / calibrationSteps.length) * 100}%`,
                                    }}
                                />
                            </div>
                        </div>
                        <h3 className="text-xl font-bold text-white mb-2">{calibrationSteps[calibrationStep].title}</h3>
                        <p className="text-gray-300 mb-6">{calibrationSteps[calibrationStep].instruction}</p>
                        <button
                            onClick={nextCalibrationStep}
                            className="w-full inline-flex justify-center items-center px-4 py-3 border border-transparent text-sm font-medium rounded-md text-white bg-sky-600 hover:bg-sky-700 transition-colors duration-150"
                        >
                            {calibrationStep < calibrationSteps.length - 1 ? "Next Step" : "Complete Calibration"}
                            <ChevronRightOutline className="ml-2 h-5 w-5" />
                        </button>
                    </div>
                </div>
            )}

            {!calibrating && (
                <div className="flex justify-between">
                    <button
                        onClick={onBack}
                        className="inline-flex items-center px-4 py-2 border border-gray-600 text-sm font-medium rounded-md text-gray-300 bg-gray-800 hover:bg-gray-700 transition-colors duration-150"
                    >
                        <ChevronLeftOutline className="mr-2 h-5 w-5" />
                        Back
                    </button>
                    <button
                        onClick={onNext}
                        className="inline-flex items-center px-4 py-2 border border-transparent text-sm font-medium rounded-md text-white bg-sky-600 hover:bg-sky-700 transition-colors duration-150"
                    >
                        {imuAvailable ? "Skip for now" : "Continue"}
                        <ChevronRightOutline className="ml-2 h-5 w-5" />
                    </button>
                </div>
            )}
        </div>
    );
}
