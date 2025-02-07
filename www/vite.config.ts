import { defineConfig } from "vite";
import mockServer from "vite-plugin-mock-server";
import preact from "@preact/preset-vite";
import tailwindcss from "@tailwindcss/vite";

// https://vitejs.dev/config/
export default defineConfig({
    plugins: [
        mockServer({
            mockRootDir: "api",
            printStartupLog: false,
        }),
        preact(),
        tailwindcss(),
    ],
    optimizeDeps: {
        exclude: ["preact-heroicons"],
    },
    build: {
        emptyOutDir: true,
        outDir: "../build/www/www",
    },
});
