/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

export class settings {
    static setting = {
        altSamples: { default: "10" },
        defaultMap: { default: "0" },
        defaultSpeed: { default: "25" },
        dropSecs: { default: "10" },
        showOfflineNotice: { default: "1" },
    } as const;

    static get<K extends keyof typeof this.setting>(key: K): string {
        const value = localStorage.getItem(key);
        if (value) {
            return value;
        }
        // No local storage entry found, return default value
        return this.setting[key].default;
    }

    static set<K extends keyof typeof this.setting>(key: K, value: string): void {
        localStorage.setItem(key, value);
    }
}

export default settings;
