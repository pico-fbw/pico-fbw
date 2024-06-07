/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

const timeout = 2000; // Timeout for an API request in ms

type EmptyResponse = Record<string, never>;

// GET endpoints
export type GET_CONFIG = {
    sections: {
        name: string;
        keys: (number | string)[];
    }[];
};
export type GET_INFO = {
    version: string;
    version_api: string;
    version_flightplan: string;
    platform: string;
    platform_version: string;
};

// SET endpoints
export type SET_CONFIG = EmptyResponse;
export type SET_FLIGHTPLAN = {
    message: string;
};

// MISC endpoints
export type PING = EmptyResponse;

// Maps API URIs to their respective response types
type EndpointMap = {
    "get/config": GET_CONFIG;
    "get/info": GET_INFO;
    "set/config": SET_CONFIG;
    "set/flightplan": SET_FLIGHTPLAN;
    ping: PING;
};

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
        return res.json();
    }) as Promise<EndpointMap[E]>;
    clearTimeout(id);
    return response;
}
