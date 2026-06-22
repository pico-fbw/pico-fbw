/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { useState } from "preact/hooks";

import Modal from "elements/Modal";

interface NewFlightplanModalProps {
    open: boolean;
    setOpen: (open: boolean) => void;
    setLocation: (location: string) => void;
}

export default function NewFlightplanModal({ open, setOpen, setLocation }: NewFlightplanModalProps) {
    const [name, setName] = useState("");

    return (
        <Modal title="New Flightplan" open={open}>
            <input
                type="text"
                value={name}
                onChange={e => setName(e.currentTarget.value)}
                placeholder="Enter a name for your flightplan..."
                className="w-full p-2 mb-4 border-2 border-gray-700 text-gray-300 rounded"
            />
            <div className="flex justify-end">
                <button
                    onClick={() => {
                        setOpen(false);
                        setName("");
                    }}
                    className="mr-2 px-4 py-2 text-md font-semibold rounded-md shadow-sm text-white bg-gray-500 hover:bg-gray-500/50"
                >
                    Cancel
                </button>
                <button
                    onClick={() => {
                        if (name.trim() !== "") {
                            setLocation(`/planner/${name}`);
                        }
                        setOpen(false);
                        setName("");
                    }}
                    className="px-4 py-2 text-md font-semibold rounded-md shadow-sm text-white bg-sky-500 hover:bg-sky-600"
                >
                    Create
                </button>
            </div>
        </Modal>
    );
}
