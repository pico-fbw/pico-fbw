/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { useState } from "preact/hooks";
import { DocumentOutline } from "preact-heroicons";
import { useLocation } from "wouter-preact";

import DeleteFlightplanModal from "./DeleteFlightplanModal";

import { api } from "helpers/api";
import { FlightplanList } from "helpers/apiTypes";

function formatBytes(bytes: number, decimals = 2) {
    if (bytes === 0) {
        return "0 B";
    }
    const k = 1024;
    const dm = decimals < 0 ? 0 : decimals;
    const sizes = ["B", "kB", "MB", "GB", "TB"];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return `${parseFloat((bytes / Math.pow(k, i)).toFixed(dm))} ${sizes[i]}`;
}

interface ExplorerProps {
    flightplans: FlightplanList;
    setFlightplans: (flightplans: FlightplanList) => void;
    setError: (error: string) => void;
}

export default function Explorer({ flightplans, setFlightplans, setError }: ExplorerProps) {
    const [, setLocation] = useLocation();

    const [flightplanToDelete, setFlightplanToDelete] = useState("");

    const deleteFlightplan = async (name: string) => {
        await api("set/flightplan", { flightplan: null, name }).then(() => {
            // Flightplan has been removed from server, now remove it from client-side list
            setFlightplans({
                ...flightplans,
                flightplans: flightplans.flightplans.filter(fp => fp.name !== name),
            });
        });
    };

    interface EntryProps {
        name: string;
        size: number;
        active?: boolean;
    }

    function Entry({ name, size, active }: EntryProps) {
        return (
            <div
                className={`flex max-width items-center py-4 px-6 mx-4 lg:mx-35 my-6 ${active ? "bg-gray-700 border-2 border-gray-300" : "bg-gray-800"} rounded-2xl`}
            >
                <div className="flex grow items-center space-x-3">
                    <DocumentOutline className="h-8 w-8 text-white" />
                    <div className="flex flex-col">
                        <p className="text-2xl font-semibold text-white">{name}</p>
                        <p className="text-xl font-medium text-white">{formatBytes(size)}</p>
                    </div>
                </div>
                <div className="flex flex-col gap-3 justify-center md:flex-row">
                    <button
                        type="button"
                        onClick={() => setLocation(`/planner/${name}`)}
                        className="grow px-4 py-2.5 lg:px-5 lg:py-4.5 border border-transparent text-xl leading-4 font-bold rounded-xl shadow-sm text-white bg-sky-500 hover:bg-sky-500/90 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-sky-500"
                    >
                        Edit
                    </button>
                    <button
                        type="button"
                        onClick={() => setFlightplanToDelete(name)}
                        className="grow px-4 py-2.5 lg:px-5 lg:py-4.5 border border-transparent text-xl leading-4 font-bold rounded-xl shadow-sm text-white bg-red-500/70 hover:bg-red-500/80 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-red-500/60"
                    >
                        Delete
                    </button>
                </div>
            </div>
        );
    }

    return (
        <div>
            {flightplans.flightplans.map((fp, index) => (
                <Entry key={index} name={fp.name} size={fp.size} active={fp.name === flightplans.active} />
            ))}
            <DeleteFlightplanModal
                name={flightplanToDelete}
                setName={setFlightplanToDelete}
                deleteFlightplan={deleteFlightplan}
                setError={setError}
            />
        </div>
    );
}
