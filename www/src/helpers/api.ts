/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

const timeout = 2000; // Timeout for any API request in ms

export type EmptyResponse = Record<string, never>;

export async function api<T = any>(endpoint: string, data?: object): Promise<T> {
    const controller = new AbortController();
    let options: RequestInit = {
        method: 'GET',
        signal: controller.signal,
    };
    if (data) {
        options = {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(data),
        };
    }

    const id = setTimeout(() => controller.abort(), timeout);
    const response = fetch(`/api/v1/${endpoint}`, options).then(res => {
        if (!res.ok) {
            throw new Error(`API error (${res.status})`);
        }
        return res.json();
    }) as Promise<T>;
    clearTimeout(id);
    return response;
}

/**
 * /api/v1:
 *  /ping => always returns ERR.OK
 *  /set
 *   /fplan => requires 'data' to be an array of Waypoint, returns an ERR code
 */
