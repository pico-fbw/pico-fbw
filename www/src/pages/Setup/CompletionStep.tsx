/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { CheckCircleOutline, ChevronRightOutline } from "preact-heroicons";
import { Link } from "wouter-preact";

import settings from "helpers/settings";

export default function CompletionStep() {
    return (
        <div className="max-w-3xl mx-auto px-4 py-12 text-center">
            <div className="mb-8 flex justify-center">
                <div className="rounded-full bg-green-600/10 p-6">
                    <CheckCircleOutline className="h-16 w-16 text-green-500" />
                </div>
            </div>
            <h1 className="text-4xl font-bold text-white mb-4">Setup complete!</h1>
            <p className="text-xl text-gray-300 mb-8">
                Great job, {settings.get("pilotName")}! Your pico-fbw system is now configured and ready for flight.
            </p>

            <div className="bg-gray-800 rounded-lg p-6 mb-8 text-left">
                <h3 className="text-lg font-semibold text-white mb-4">What's next?</h3>
                <ul className="space-y-3">
                    <li className="flex items-start">
                        <CheckCircleOutline className="h-6 w-6 text-green-500 mr-3 flex-shrink-0 mt-0.5" />
                        <div>
                            <p className="text-white font-medium">Review your configuration</p>
                            <p className="text-sm text-gray-400">Visit the Settings page to fine-tune your setup</p>
                        </div>
                    </li>
                    <li className="flex items-start">
                        <CheckCircleOutline className="h-6 w-6 text-green-500 mr-3 flex-shrink-0 mt-0.5" />
                        <div>
                            <p className="text-white font-medium">Create a flight plan</p>
                            <p className="text-sm text-gray-400">
                                Use the Planner to create and manage your flight plans
                            </p>
                        </div>
                    </li>
                    <li className="flex items-start">
                        <CheckCircleOutline className="h-6 w-6 text-green-500 mr-3 flex-shrink-0 mt-0.5" />
                        <div>
                            <p className="text-white font-medium">Perform a ground test</p>
                            <p className="text-sm text-gray-400">Test your controls and sensors before taking flight</p>
                        </div>
                    </li>
                </ul>
            </div>

            <Link
                to="/"
                onClick={() => settings.set("setupComplete", "true")}
                className="inline-flex items-center px-6 py-3 border border-transparent text-base font-medium rounded-md text-white bg-sky-600 hover:bg-sky-700 transition-colors duration-150"
            >
                Go to Dashboard
                <ChevronRightOutline className="ml-2 h-5 w-5" />
            </Link>
        </div>
    );
}
