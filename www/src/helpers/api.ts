/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

export enum ERR {
    OK = 0,
    ARG = 1,
    NET = 2,
}

interface APIData {
    err: ERR; // Error code
    dat?: Array<{ [key: string]: any }>; // Data: array of key-value pairs, for example {"pi":3.14},{"client":"mom"} etc.
}

export async function api(endpoint: string, data?: object): Promise<APIData> {
    let options: RequestInit = {
        method: 'GET',
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

    return fetch(`/api/v1/${endpoint}`, options)
        .then(res => {
            if (!res.ok) {
                throw new Error(`HTTP error ${res.status}`);
            }
            return res.json();
        })
        .catch(error => {
            console.error(`Error fetching data from /api/v1/${endpoint}:`, error);
            return { err: ERR.NET };
        }) as Promise<APIData>;
}

/**
 * /api/v1:
 *  /ping => always returns ERR.OK
 *  /set
 *   /fplan => requires 'data' to be an array of Waypoint, returns an ERR code
 */
