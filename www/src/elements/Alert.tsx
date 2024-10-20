/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import preact from "preact";
import { useState } from "preact/hooks";
import {
    CheckCircleOutline,
    ExclamationTriangleOutline,
    InformationCircleOutline,
    ShieldExclamationOutline,
    XMarkSolid,
} from "preact-heroicons";

import classNames from "../helpers/classNames";

interface AlertProps {
    type: "success" | "info" | "warning" | "danger";
    onClose?: () => void;
    className?: string;
    children: preact.ComponentChildren;
}

function getColors(type: AlertProps["type"]) {
    switch (type) {
        case "success":
            return "bg-green-400/10 text-green-500";
        case "info":
            return "bg-blue-400/10 text-blue-500";
        case "warning":
            return "bg-yellow-400/10 text-yellow-500";
        case "danger":
            return "bg-red-400/10 text-red-400";
    }
}

const Alert: preact.FunctionComponent<AlertProps> = ({ type, onClose, className = "", children }) => {
    const [hidden, setHidden] = useState(false);

    /**
     * Handles the closure of the alert.
     */
    const handleClose = () => {
        setHidden(true);
        onClose(); // Call the provided onClose function
    };

    return (
        <div className={classNames("relative rounded-md p-4", getColors(type), className, hidden && "hidden")}>
            {onClose && (
                <div onClick={handleClose} className="absolute top-0 right-0 p-1">
                    <button className="flex items-center justify-center h-6 w-6 text-gray-400 hover:text-gray-500">
                        <XMarkSolid className="h-5 w-5" />
                    </button>
                </div>
            )}
            <div className="flex">
                <div className="flex-shrink-0">
                    {type === "success" ? (
                        <CheckCircleOutline className={"mr-2 h-6 w-6 text-green-500"} />
                    ) : type === "info" ? (
                        <InformationCircleOutline className={"mr-2 h-6 w-6 text-blue-500"} />
                    ) : type === "warning" ? (
                        <ExclamationTriangleOutline className={"mr-2 h-6 w-6 text-yellow-500"} />
                    ) : (
                        <ShieldExclamationOutline className={"mr-2 h-6 w-6 text-red-400"} />
                    )}
                </div>
                <div className={`ml-3 ${onClose ? "mr-4" : ""} flex-1 md:flex md:justify-between`}>{children}</div>
            </div>
        </div>
    );
};

export default Alert;
