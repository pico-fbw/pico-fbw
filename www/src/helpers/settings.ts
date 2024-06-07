/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

export class settings {
    static setting = {
        altSamples: { default: "10" },
        defaultSpeed: { default: "25" },
        dropSecs: { default: "10" },
        // Internal settings, cannot be changed by user
        defaultMap: { default: "0" },
        lastMapPosition: { default: "" },
        lastMapZoom: { default: "" },
        showOfflineNotice: { default: "1" },
    } as const;

    /**
     * Get a setting from local storage, or the default value if it doesn't exist.
     * @param key the setting to get
     * @returns the value of the setting
     */
    static get<K extends keyof typeof this.setting>(key: K): string {
        const value = localStorage.getItem(key);
        if (value) {
            return value;
        }
        // No local storage entry found, return default value
        return this.setting[key].default;
    }

    /**
     * Set a setting in local storage.
     * @param key the setting to set
     * @param value the value to set the setting to
     */
    static set<K extends keyof typeof this.setting>(key: K, value: string): void {
        localStorage.setItem(key, value);
    }
}

export default settings;
