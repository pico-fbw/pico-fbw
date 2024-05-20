/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

import { useState } from "preact/hooks";

interface UseFileUploadInput {
    accept?: string;
    multiple?: boolean;
    maxSize?: number;
    onFileChange: (file: File) => void;
}

interface UseFileUploadOutput {
    openFilePicker: () => void;
    file?: File | null;
}

export default ({ accept, multiple, maxSize, onFileChange }: UseFileUploadInput): UseFileUploadOutput => {
    const [file, setFile] = useState<File | null>(null);

    const openFilePicker = () => {
        const inputElement = document.createElement("input");
        inputElement.type = "file";
        inputElement.accept = accept || "";
        inputElement.multiple = multiple || false;
        inputElement.onchange = () => {
            const selectedFile = inputElement.files?.[0];
            if (selectedFile) {
                if (maxSize && selectedFile.size > maxSize) {
                    console.log("File too large");
                    return;
                }
                setFile(selectedFile);
                onFileChange(selectedFile);
            }
        };
        inputElement.click();
        inputElement.remove();
    };

    return { openFilePicker, file };
};
