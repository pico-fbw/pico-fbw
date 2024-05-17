/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import { useState } from 'preact/hooks';

import Alert from './Alert';

import { api, EmptyResponse } from '../helpers/api';
import { Flightplan } from '../helpers/flightplan';
import { Settings } from '../helpers/settings';

export default function Uploader() {
    const [flightplan, setFlightplan] = useState('');
    const [error, setError] = useState('');
    const showOfflineNotice = Settings.get('showOfflineNotice') === '1';

    const sendFlightplan = async (input: string) => {
        let fplan: Flightplan;
        try {
            fplan = JSON.parse(input) as Flightplan;
        } catch (e) {
            console.error('Error parsing JSON:', (e as Error).message);
            setError('Invalid flightplan!');
            return;
        }
        try {
            await api<EmptyResponse>('set/fplan', fplan);
        } catch (e) {
            setError(`Server error whilst uploading: ${(e as Error).message}`);
        }
    };

    const uploadJson = () => {
        const input = document.createElement('input');
        input.type = 'file';
        input.accept = '.json';
        input.onchange = () => {
            const file = input.files?.[0];
            if (file) {
                const reader = new FileReader();
                reader.onload = async () => {
                    const uploaded = reader.result as string;
                    setFlightplan(uploaded);
                    await sendFlightplan(uploaded);
                };
                reader.readAsText(file);
            }
        };
        input.click();
        input.remove();
    };

    return (
        <div className="flex flex-col items-center justify-center space-y-4">
            {showOfflineNotice && (
                <Alert type="info" onClose={() => Settings.set('showOfflineNotice', '0')} className="mx-4 sm:mx-8 lg:mx-0">
                    It looks like you're offline. You can still upload flightplans that have been pre-generated at&nbsp;
                    <a
                        href="https://pico-fbw.org/tools/planner"
                        target="_blank"
                        rel="noreferrer"
                        className="hover:text-sky-500"
                    >
                        pico-fbw.org
                    </a>
                    .
                </Alert>
            )}
            <textarea
                className="p-2 bg-white/5 placeholder:text-gray-400 text-gray-200 ring-1 ring-inset ring-white/10 shadow-sm rounded resize-none w-full"
                placeholder={
                    window.innerWidth <= 768
                        ? 'Paste flightplan here...'
                        : 'Paste flightplan here, or click Upload to select a file...'
                }
                value={flightplan}
                onInput={e => setFlightplan(e.currentTarget.value)}
            />
            {error && (
                <Alert type="danger" className="mx-4 sm:mx-8 lg:mx-0">
                    {error}
                </Alert>
            )}
            <button
                className="bg-blue-600 hover:bg-sky-500 text-white font-bold py-2 px-4 rounded mt-4"
                onClick={() => {
                    if (flightplan) {
                        sendFlightplan(flightplan).catch(console.error);
                    } else {
                        uploadJson();
                    }
                }}
            >
                Upload
            </button>
        </div>
    );
}
