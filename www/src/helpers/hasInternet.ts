const url = 'https://httpbin.org/status/200'; // URL to run a GET request on to check for internet connection
const timeout = 1000; // Timeout for the request in ms

export default async (): Promise<boolean> => {
    try {
        const controller = new AbortController();
        const id = setTimeout(() => controller.abort(), timeout);
        const response = await fetch(url, { signal: controller.signal });
        clearTimeout(id);
        return response.ok;
    } catch (error) {
        return false;
    }
};
