import { defineConfig } from "vite";
import mockServer from "vite-plugin-mock-server";
import preact from "@preact/preset-vite";

// https://vitejs.dev/config/
export default defineConfig({
    plugins: [
        mockServer({
            mockRootDir: "api",
            printStartupLog: false,
        }),
        preact(),
    ],
    build: {
        emptyOutDir: true,
        outDir: "../build/www/www",
    },
});
