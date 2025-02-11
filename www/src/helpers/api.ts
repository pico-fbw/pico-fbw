/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

import { EndpointMap } from "helpers/apiTypes";

const timeout = 2000; // Timeout for an API request in ms

/**
 * Executes an API request.
 * @param endpoint the API endpoint to call
 * @param data the optional data to send to the endpoint
 * @returns the response from the API
 */
export async function api<E extends keyof EndpointMap>(endpoint: E, data?: object): Promise<EndpointMap[E]> {
    const controller = new AbortController();
    let options: RequestInit = {
        method: "GET",
        signal: controller.signal,
    };
    if (data) {
        options = {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(data),
        };
    }

    const id = setTimeout(() => controller.abort(), timeout);
    const response = fetch(`/api/v1/${endpoint}`, options).then(res => {
        if (!res.ok) {
            throw new Error(res.status.toString());
        }
        if (res.status === 204) {
            return {};
        }
        return res.json();
    }) as Promise<EndpointMap[E]>;
    clearTimeout(id);
    return response;
}
