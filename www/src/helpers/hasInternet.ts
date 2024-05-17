/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

const url = 'https://pico-fbw.org'; // URL to run a GET request on to check for internet connection
const timeout = 1000; // Timeout for the request in ms

export default async (): Promise<boolean> => {
    try {
        const controller = new AbortController();
        const id = setTimeout(() => controller.abort(), timeout);
        const response = await fetch(url, { signal: controller.signal, cache: 'no-store' }); // Skip cache
        clearTimeout(id);
        return response.ok;
    } catch (e) {
        return false;
    }
};
