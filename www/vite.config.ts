import { defineConfig } from 'vite';
import { pluginJsonServer } from 'vite-plugin-json-server';
import preact from '@preact/preset-vite';

// https://vitejs.dev/config/
export default defineConfig({
    plugins: [
        pluginJsonServer({
            apiPath: '/api/v1',
            source: 'api.json',
            delay: 200,
        }),
        preact(),
    ],
    build: {
        emptyOutDir: true,
        outDir: '../build/www/www',
    },
});
