import { defineConfig } from "vite";
import mockServer from "vite-plugin-mock-server";
import preact from "@preact/preset-vite";
import tailwindcss from "@tailwindcss/vite";

export default defineConfig(() => {
    // Use the DIST_DIR argument if provided, otherwise fallback to the default assets path
    const outDir = `${process.env.DIST_DIR}` || "../build/www/www";

    return {
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
        resolve: {
            alias: {
                elements: "/src/elements",
                helpers: "/src/helpers",
                pages: "/src/pages",
            },
        },
        build: {
            emptyOutDir: true,
            outDir,
        },
    };
});
