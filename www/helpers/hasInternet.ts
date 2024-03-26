import axios from 'axios';

const url = 'https://httpbin.org/status/200'; // URL to run a GET request on to check for internet connection
const timeout = 1000; // Timeout for the request in ms

export default async (): Promise<boolean> => {
    try {
        await axios.get(url, { timeout: timeout });
        return true;
    } catch (error) {
        return false;
    }
}
