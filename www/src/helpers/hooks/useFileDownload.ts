/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import { useCallback } from "preact/hooks";

interface UseFileDownloadInput {
    filename: string;
    filetype: string;
}

interface UseFileDownloadOutput {
    downloadFile: (content: BlobPart) => void;
}

export default ({ filename, filetype }: UseFileDownloadInput): UseFileDownloadOutput => {
    const downloadFile = useCallback(
        (content: BlobPart) => {
            const blob = new Blob([content], { type: filetype });
            const url = URL.createObjectURL(blob);

            const link = document.createElement("a");
            link.href = url;
            link.download = filename;
            link.click();

            URL.revokeObjectURL(url);
        },
        [filename, filetype],
    );

    return { downloadFile };
};
