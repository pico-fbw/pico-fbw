/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { useState } from "preact/hooks";
import { ChevronLeftOutline, ChevronRightOutline, UserCircleOutline } from "preact-heroicons";

import settings from "helpers/settings";

interface UserInfoStepProps {
    onNext: () => void;
    onBack: () => void;
}

export default function UserInfoStep({ onNext, onBack }: UserInfoStepProps) {
    const [pilotName, setPilotName] = useState(settings.get("pilotName"));
    const [isShaking, setIsShaking] = useState(false);

    const handleNext = () => {
        if (!pilotName.trim()) {
            setIsShaking(true);
            setTimeout(() => setIsShaking(false), 500);
            return;
        }
        settings.set("pilotName", pilotName.trim());
        onNext();
    };

    return (
        <div className="max-w-2xl mx-auto px-4 py-8">
            <div className="mb-8">
                <UserCircleOutline className="h-12 w-12 text-sky-500 mx-auto mb-4" />
                <h2 className="text-3xl font-bold text-white text-center mb-2">Personal Information</h2>
                <p className="text-gray-400 text-center">Tell us a bit about yourself</p>
            </div>

            <div className="bg-gray-800 rounded-lg p-6 mb-8">
                <label htmlFor="pilotName" className="block text-sm font-medium text-gray-300 mb-2">
                    Pilot Name
                </label>
                <input
                    id="pilotName"
                    type="text"
                    value={pilotName}
                    onInput={(e) => setPilotName((e.target as HTMLInputElement).value)}
                    placeholder="Enter your name"
                    className={`block w-full rounded-md border-0 bg-gray-700 px-4 py-3 text-white shadow-sm ring-1 ring-inset ring-gray-600 placeholder:text-gray-400 focus:ring-2 focus:ring-inset focus:ring-sky-500 sm:text-sm sm:leading-6 transition-all ${isShaking ? "animate-[shake_0.5s_ease-in-out]" : ""}`}
                />
                <style>{`
                    @keyframes shake {
                        0%, 100% { transform: translateX(0); }
                        10%, 30%, 50%, 70%, 90% { transform: translateX(-8px); }
                        20%, 40%, 60%, 80% { transform: translateX(8px); }
                    }
                `}</style>
                <p className="mt-2 text-sm text-gray-400">
                    This will be used to identify you across the system.
                </p>
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
                    className="inline-flex items-center px-4 py-2 border border-transparent text-sm font-medium rounded-md text-white bg-sky-600 hover:bg-sky-700 transition-colors duration-150"
                >
                    Continue
                    <ChevronRightOutline className="ml-2 h-5 w-5" />
                </button>
            </div>
        </div>
    );
}
