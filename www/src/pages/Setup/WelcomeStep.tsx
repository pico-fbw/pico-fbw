/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { ChevronRightOutline, PaperAirplaneOutline, UserCircleOutline, Cog6ToothOutline, BeakerOutline } from "preact-heroicons";

interface WelcomeStepProps {
    onNext: () => void;
}

export default function WelcomeStep({ onNext }: WelcomeStepProps) {
    return (
        <div className="max-w-3xl mx-auto px-4 py-12 text-center">
            <div className="mb-8 flex justify-center">
                <div className="rounded-full bg-sky-600/10 p-6">
                    <PaperAirplaneOutline className="h-16 w-16 text-sky-500" />
                </div>
            </div>
            <h1 className="text-3xl lg:text-4xl font-bold text-white mb-4">Welcome to pico-fbw!</h1>
            <p className="text-xl text-gray-300 mb-8">
                Let's get your flight controller set up. This wizard will guide you through the essential configuration
                steps to ensure your aircraft is ready for flight.
            </p>
            <div className="grid grid-cols-1 md:grid-cols-3 gap-6 mb-12">
                <div className="bg-gray-800 rounded-lg p-6">
                    <UserCircleOutline className="h-10 w-10 text-sky-500 mx-auto mb-3" />
                    <h3 className="text-lg font-semibold text-white mb-2">Personal Info</h3>
                    <p className="text-sm text-gray-400">Configure your pilot name and preferences</p>
                </div>
                <div className="bg-gray-800 rounded-lg p-6">
                    <Cog6ToothOutline className="h-10 w-10 text-sky-500 mx-auto mb-3" />
                    <h3 className="text-lg font-semibold text-white mb-2">Aircraft Config</h3>
                    <p className="text-sm text-gray-400">Set up your aircraft control configuration</p>
                </div>
                <div className="bg-gray-800 rounded-lg p-6">
                    <BeakerOutline className="h-10 w-10 text-sky-500 mx-auto mb-3" />
                    <h3 className="text-lg font-semibold text-white mb-2">System Calibration</h3>
                    <p className="text-sm text-gray-400">Calibrate systems for optimal performance</p>
                </div>
            </div>
            <button
                onClick={onNext}
                className="inline-flex items-center px-6 py-3 border border-transparent text-base font-medium rounded-md text-white bg-sky-600 hover:bg-sky-700 transition-colors duration-150"
            >
                Get Started
                <ChevronRightOutline className="ml-2 h-5 w-5" />
            </button>
        </div>
    );
}
