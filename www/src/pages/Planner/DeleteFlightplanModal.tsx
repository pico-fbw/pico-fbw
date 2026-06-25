/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import Modal from "elements/Modal";

interface DeleteFlightplanModalProps {
    name: string;
    setName: (name: string) => void;
    deleteFlightplan: (name: string) => Promise<void>;
}

export default function DeleteFlightplanModal({ name, setName, deleteFlightplan }: DeleteFlightplanModalProps) {
    return (
        <Modal title="Delete Flightplan" open={name !== ""}>
            <p className="mb-4 text-gray-300">Are you sure you want to delete this flightplan?</p>
            <div className="flex justify-end">
                <button
                    onClick={() => setName("")}
                    className="mr-2 px-4 py-2 text-md font-semibold rounded-md shadow-sm text-white bg-gray-500 hover:bg-gray-500/50"
                >
                    Cancel
                </button>
                <button
                    onClick={() => {
                        deleteFlightplan(name).catch(console.error);
                        setName("");
                    }}
                    className="px-4 py-2 text-md font-semibold rounded-md shadow-sm text-white bg-red-500/60 hover:bg-red-500/90"
                >
                    Delete
                </button>
            </div>
        </Modal>
    );
}
