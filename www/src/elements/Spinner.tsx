/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { useMemo } from "preact/hooks";

const splashTexts = [
    "Almost there...",
    "Hang tight...",
    "Let's get flying!",
    "Preparing for takeoff...",
    "Just a quick control check...",
    "Just a moment...",
    "Getting things ready...",
    "Warming up the engines...",
    "Calibrating instruments...",
];

const Spinner = () => {
    const splashText = useMemo(() => {
        return splashTexts[Math.floor(Math.random() * splashTexts.length)];
    }, []);

    return (
        <div className="relative top-1/2">
            <div className="absolute left-1/2 transform -translate-x-1/2 -translate-y-1/2 text-center">
                <div className="animate-spin rounded-full h-32 w-32 border-b-2 border-sky-500 mx-auto" />
                <p className="mt-6 text-lg text-gray-400 font-semibold">{splashText}</p>
            </div>
        </div>
    );
};

export default Spinner;
