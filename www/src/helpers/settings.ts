/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

import { api } from "helpers/api";

const sectionName = "WebUI" as const;
const webUIKeys = [
    "defaultSpeed",
    "dropSecs",
    "pilotName",
    "defaultMap",
    "lastMapPosition",
    "lastMapZoom",
    "setupComplete",
] as const;
type SettingKey = (typeof webUIKeys)[number];

const cache: Partial<Record<SettingKey, string>> = {};
let loadPromise: Promise<void> | null = null;

export class settings {
    /**
     * Load the on-device WebUI settings into memory.
     */
    static async load(): Promise<void> {
        if (loadPromise) {
            return loadPromise;
        }

        loadPromise = (async () => {
            const response = await api("get/config");
            const webUISection = response.sections.find(section => section.name === sectionName);
            if (!webUISection) {
                return;
            }

            webUIKeys.forEach((key, index) => {
                const value = webUISection.values[index];
                if (value !== undefined && value !== null) {
                    cache[key] = String(value);
                }
            });
        })();

        try {
            await loadPromise;
        } finally {
            loadPromise = null;
        }
    }

    /**
     * Get a setting from the cached on-device config.
     * @param key the setting to get
     * @returns the value of the setting
     */
    static get<K extends SettingKey>(key: K): string {
        return cache[key] ?? "";
    }

    /**
     * Set a setting in the on-device config.
     * @param key the setting to set
     * @param value the value to set the setting to
     */
    static set<K extends SettingKey>(key: K, value: string): void {
        const previousValue = this.get(key);
        cache[key] = value;

        void (async () => {
            try {
                const response = await api("set/config", {
                    changes: [
                        {
                            section: sectionName,
                            key,
                            value,
                        },
                    ],
                    save: true,
                });

                if (response.error) {
                    throw new Error(response.error);
                }
            } catch (error) {
                cache[key] = previousValue;
                console.error(`Failed to store ${key}:`, error);
            }
        })();
    }
}

export default settings;
