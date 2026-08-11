/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { useEffect, useState } from "preact/hooks";
import ContentBlock from "elements/ContentBlock";
import Modal from "elements/Modal";
import { useFileDownload, useFileUpload } from "helpers/hooks";
import { api } from "helpers/api";
import { GET_CONFIG, GET_INFO } from "helpers/apiTypes";

export default function Advanced() {
    const [error, setError] = useState("");
    const [info, setInfo] = useState<GET_INFO | null>(null);
    const [setupModalOpen, setSetupModalOpen] = useState(false);
    const [rebootModalOpen, setRebootModalOpen] = useState(false);
    const [licenseModalOpen, setLicenseModalOpen] = useState(false);

    const { downloadFile } = useFileDownload({ filename: "config.json", filetype: "application/json" });
    const { openFilePicker } = useFileUpload({
        accept: ".json",
        onFileChange: selectedFile => {
            const reader = new FileReader();
            reader.onload = async () => {
                try {
                    const uploaded = reader.result as string;
                    const parsed = JSON.parse(uploaded) as GET_CONFIG;
                    // Send full config blob to server and request save
                    const resp = await api("set/config_full", { ...parsed, save: true });
                    if (resp.error) {
                        setError(resp.error);
                    }
                } catch (e) {
                    setError(`Invalid config file: ${(e as Error).message}`);
                }
            };
            reader.readAsText(selectedFile);
        },
    });

    const downloadConfig = async () => {
        try {
            const cfg = await api("get/config");
            downloadFile(JSON.stringify(cfg, null, 2));
        } catch (e) {
            setError((e as Error).message);
        }
    };

    const rerunSetup = async () => {
        try {
            await api("set/config", { changes: [{ section: "WebUI", key: "setupComplete", value: "0" }], save: true });
        } catch (e) {
            setError((e as Error).message);
        } finally {
            setSetupModalOpen(false);
        }
    };

    const reboot = async () => {
        try {
            await api("misc/reboot");
        } catch (e) {
            setError((e as Error).message);
        } finally {
            setRebootModalOpen(false);
        }
    };

    useEffect(() => {
        api("get/info")
            .then(setInfo)
            .catch(e => setError((e as Error).message));
    }, []);

    return (
        <ContentBlock title="Advanced" loading={!info} error={error} setError={setError}>
            <div className="space-y-6 p-4 sm:p-6 lg:p-8">
                <h1 className="text-3xl font-bold text-white mb-4">Advanced</h1>
                <div className="bg-gray-800 rounded-lg p-6">
                    <h3 className="text-lg font-semibold text-white mb-3">Backup & Restore</h3>
                    <div className="flex gap-3">
                        <button
                            type="button"
                            // eslint-disable-next-line @typescript-eslint/no-misused-promises
                            onClick={downloadConfig}
                            className="inline-flex items-center px-4 py-2 border border-transparent text-md leading-4 font-semibold rounded-md shadow-sm text-white bg-gray-500 hover:bg-gray-500/50"
                        >
                            Download Config
                        </button>
                        <button
                            type="button"
                            onClick={openFilePicker}
                            className="inline-flex items-center px-4 py-2 border border-transparent text-md leading-4 font-semibold rounded-md shadow-sm text-white bg-gray-500 hover:bg-gray-500/50"
                        >
                            Upload Config
                        </button>
                    </div>
                </div>

                <div className="bg-gray-800 rounded-lg p-6">
                    <h3 className="text-lg font-semibold text-white mb-3">System Actions</h3>
                    <div className="flex gap-3">
                        <button
                            type="button"

                            onClick={() => setSetupModalOpen(true)}
                            className="inline-flex items-center px-4 py-2 border border-transparent text-md leading-4 font-semibold rounded-md shadow-sm text-white bg-gray-500 hover:bg-gray-500/50"
                        >
                            Re-run Setup
                        </button>
                        <button
                            type="button"
                            onClick={() => setRebootModalOpen(true)}
                            className="inline-flex items-center px-4 py-2 border border-transparent text-md leading-4 font-semibold rounded-md shadow-sm text-white bg-red-500/70 hover:bg-red-500/80"
                        >
                            Reboot
                        </button>
                    </div>
                </div>

                <div className="pt-4">
                    <div className="text-center text-sm text-gray-300">
                        <p>pico-fbw - lightweight flight-by-wire for hobby craft.</p>
                        <p className="mt-2 text-sm text-gray-400">pico-fbw v{info?.version ?? "Unknown"}</p>
                        <p className="text-sm text-gray-400">API v{info?.version_api ?? "Unknown"}</p>
                        <p className="text-sm text-gray-400">
                            {info?.platform ?? "Unknown"} {info?.platform_version ?? ""}
                        </p>
                        <div className="mt-2 flex gap-2 justify-center">
                            <a
                                className="text-sm text-gray-400 hover:text-gray-400/90 underline"
                                href="https://pico-fbw.org/about"
                                target="_blank"
                            >
                                About
                            </a>
                            <button
                                type="button"
                                onClick={() => setLicenseModalOpen(true)}
                                className="text-sm text-gray-400 hover:text-gray-400/90 underline"
                            >
                                View License
                            </button>
                            <a
                                className="text-sm text-gray-400 hover:text-gray-400/90 underline"
                                href="https://github.com/pico-fbw/pico-fbw"
                                target="_blank"
                            >
                                GitHub
                            </a>
                        </div>
                    </div>
                </div>

                <Modal title="Re-run setup" open={setupModalOpen}>
                    <p className="text-sm text-gray-300 mb-4">
                        Are you sure you want to restart the setup process? This will clear all existing calibration
                        data.
                    </p>
                    <div className="flex justify-end gap-2">
                        <button
                            onClick={() => setSetupModalOpen(false)}
                            className="mr-2 px-4 py-2 text-md font-semibold rounded-md shadow-sm text-white bg-gray-500 hover:bg-gray-500/90"
                        >
                            Cancel
                        </button>
                        <button
                            // eslint-disable-next-line @typescript-eslint/no-misused-promises
                            onClick={rerunSetup}
                            className="px-4 py-2 text-md font-semibold rounded-md shadow-sm text-white bg-red-500/70 hover:bg-red-500/80"
                        >
                            Yes, I'm sure
                        </button>
                    </div>
                </Modal>
                <Modal title="Reboot device" open={rebootModalOpen}>
                    <p className="text-sm text-gray-300 mb-4">Are you sure you want to reboot the device?</p>
                    <div className="flex justify-end gap-2">
                        <button
                            onClick={() => setRebootModalOpen(false)}
                            className="mr-2 px-4 py-2 text-md font-semibold rounded-md shadow-sm text-white bg-gray-500 hover:bg-gray-500/90"
                        >
                            Cancel
                        </button>
                        <button
                            // eslint-disable-next-line @typescript-eslint/no-misused-promises
                            onClick={reboot}
                            className="px-4 py-2 text-md font-semibold rounded-md shadow-sm text-white bg-red-500/70 hover:bg-red-500/80"
                        >
                            Reboot
                        </button>
                    </div>
                </Modal>
                <Modal title="License" open={licenseModalOpen}>
                    <p className="text-sm text-gray-300">
                        This project is licensed under the MIT License. For the full license text, visit the repository:
                    </p>
                    <a
                        className="text-sm text-sky-400 underline"
                        href="https://github.com/pico-fbw/pico-fbw/blob/alpha/LICENSE.README"
                        target="_blank"
                    >
                        https://github.com/pico-fbw/pico-fbw/blob/main/LICENSE
                    </a>
                    <div className="mt-4 flex justify-end gap-2">
                        <button
                            onClick={() => setLicenseModalOpen(false)}
                            className="mr-2 px-4 py-2 text-md font-semibold rounded-md shadow-sm text-white bg-gray-500 hover:bg-gray-500/90"
                        >
                            Close
                        </button>
                    </div>
                </Modal>
            </div>
        </ContentBlock>
    );
}
