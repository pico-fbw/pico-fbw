/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { CheckCircleOutline } from "preact-heroicons";

interface StepIndicatorProps {
    numSteps: number;
    currentStep: number;
}

export default function StepIndicator({ numSteps, currentStep }: StepIndicatorProps) {
    return (
        <nav aria-label="Progress" className="mb-8 transition-all duration-500">
            <ol role="list" className="flex items-center justify-center space-x-5">
                {Array.from({ length: numSteps }, (_, index) => (
                    <li key={index} className="relative">
                        {index < currentStep ? (
                            // Completed step
                            <div className="flex items-center">
                                <div className="relative flex h-8 w-8 items-center justify-center rounded-full bg-sky-600 transition-all duration-300 scale-100">
                                    <CheckCircleOutline className="h-5 w-5 text-white animate-in zoom-in duration-300" />
                                </div>
                                {index !== numSteps - 1 && (
                                    <div className="absolute top-4 left-8 w-5 h-0.5 bg-sky-600 transition-all duration-500 origin-left scale-x-100" />
                                )}
                            </div>
                        ) : index === currentStep ? (
                            // Current step
                            <div className="flex items-center">
                                <div className="relative flex h-8 w-8 items-center justify-center rounded-full border-2 border-sky-600 bg-gray-800 transition-all duration-300 scale-110 shadow-lg shadow-sky-500/20">
                                    <span className="text-sky-600 font-semibold text-sm animate-in zoom-in duration-300">{index + 1}</span>
                                </div>
                                {index !== numSteps - 1 && (
                                    <div className="absolute top-4 left-8 w-5 h-0.5 bg-gray-600 transition-all duration-500" />
                                )}
                            </div>
                        ) : (
                            // Future step
                            <div className="flex items-center">
                                <div className="relative flex h-8 w-8 items-center justify-center rounded-full border-2 border-gray-600 bg-gray-800 transition-all duration-300 scale-90 opacity-60">
                                    <span className="text-gray-500 font-semibold text-sm">{index + 1}</span>
                                </div>
                                {index !== numSteps - 1 && (
                                    <div className="absolute top-4 left-8 w-5 h-0.5 bg-gray-600 transition-all duration-500" />
                                )}
                            </div>
                        )}
                    </li>
                ))}
            </ol>
        </nav>
    );
}
