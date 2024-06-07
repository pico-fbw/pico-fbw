/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

const url = "https://pico-fbw.org"; // URL to run a GET request on to check for internet connection
const timeout = 1000; // Timeout for the request in ms

let hasInternetResult: boolean | null = null; // Cache the result

/**
 * Checks if there is an internet connection.
 * @returns true if there is presently an internet connection
 */
export default async (): Promise<boolean> => {
    // We only want to perform the request once per app lifecycle, so return the cached result if it exists
    if (hasInternetResult !== null) {
        return hasInternetResult;
    }
    // No cache, ping the URL to determine if there is an internet connection
    try {
        const controller = new AbortController();
        const id = setTimeout(() => controller.abort(), timeout);
        const response = await fetch(url, { signal: controller.signal, cache: "no-store" }); // Skip cache
        clearTimeout(id);
        hasInternetResult = response.ok;
    } catch (e) {
        hasInternetResult = false;
    }
    return hasInternetResult;
};
