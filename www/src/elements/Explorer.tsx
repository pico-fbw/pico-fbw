/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

import { ArrowRightSolid } from "preact-heroicons";
import { useLocation } from "wouter-preact";

import { api } from "helpers/api";

interface ExplorerProps {
    flightplans: string[];
    setFlightplans: (flightplans: string[]) => void;
}

export default function Explorer({ flightplans, setFlightplans }: ExplorerProps) {
    const [, setLocation] = useLocation();

    const deleteFlightplan = async (name: string) => {
        await api("set/flightplan", { name }).then(() => {
            // Flightplan has been removed from server, now remove it from client-side list
            setFlightplans(flightplans.filter(flightplan => flightplan !== name));
        });
    };

    interface EntryProps {
        name: string;
    }

    function Entry({ name }: EntryProps) {
        return (
            <div className="flex items-center justify-between py-6">
                <div className="flex items-center space-x-3">
                    <ArrowRightSolid className="h-6 w-6 text-sky-500" />
                    <p className="text-lg font-medium text-white">{name}</p>
                </div>
                <div className="flex items-center space-x-3">
                    <button
                        type="button"
                        onClick={() => setLocation(`/planner/${name}`)}
                        className="inline-flex items-center px-3 py-1.5 border border-transparent text-sm leading-4 font-medium rounded-md shadow-sm text-white bg-sky-500 hover:bg-sky-600 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-sky-500"
                    >
                        Edit
                    </button>
                    <button
                        type="button"
                        // eslint-disable-next-line @typescript-eslint/no-misused-promises
                        onClick={() => deleteFlightplan(name)}
                        className="inline-flex items-center px-3 py-1.5 border border-transparent text-sm leading-4 font-medium rounded-md shadow-sm text-white bg-red-500/60 hover:bg-red-500/90 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-red-500/60"
                    >
                        Delete
                    </button>
                </div>
            </div>
        );
    }

    return (
        <div>
            {flightplans.map((name, index) => (
                <Entry key={index} name={name} />
            ))}
        </div>
    );
}
