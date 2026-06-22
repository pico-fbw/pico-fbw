/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

interface ModalProps {
    title: string;
    open: boolean;
    children: preact.ComponentChildren;
}

export default function Modal({ title, open, children }: ModalProps) {
    return open ? (
        <div className="fixed inset-0 bg-black/80 bg-opacity-50 flex items-center justify-center z-50">
            <div className="bg-gray-900 p-6 rounded-md shadow-md w-11/12 max-w-md">
                <h2 className="text-xl font-bold mb-4 text-white">{title}</h2>
                {children}
            </div>
        </div>
    ) : (
        <></>
    );
}
