export interface APIData {
    err: number; // Error code: 0 if no error
    dat?: Array<{ [key: string]: any }>; // Data: array of key-value pairs, for example {"pi":3.14},{"client":"mom"} etc.
}

export const ERR_OK = 0;
export const ERR_ARG = 1;

export default async (endpoint: string, data?: object): Promise<APIData> => {
    const options: RequestInit = {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
    };
    return fetch(`/api/v1/${endpoint}`, options).then((res) => res.json());
}
