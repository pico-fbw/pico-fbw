import { defineConfig } from "eslint/config";
import preact from "eslint-config-preact";
import tseslint from "typescript-eslint";
import eslintPluginPrettier from "eslint-plugin-prettier";

export default defineConfig([
	...preact,
	...tseslint.configs.recommended,
	...tseslint.configs.recommendedTypeChecked,
	{
		languageOptions: {
			ecmaVersion: "latest",
			sourceType: "module",
			parser: tseslint.parser,
			parserOptions: {
				project: true,
				tsconfigRootDir: import.meta.dirname,
			},
		},
		plugins: {
			prettier: eslintPluginPrettier,
			"@typescript-eslint": tseslint.plugin,
		},
		rules: {
			eqeqeq: "error",
			curly: ["error", "all"],
			"prettier/prettier": ["error", {}, { usePrettierrc: true }],
			"@typescript-eslint/no-explicit-any": "off",
			"@typescript-eslint/no-non-null-assertion": "off",
			"no-use-before-define": "off",
			"@typescript-eslint/no-use-before-define": "warn",
			"@typescript-eslint/no-unused-vars": [
				"warn",
				{ argsIgnorePattern: "^_", varsIgnorePattern: "^_" }
			],
			"@typescript-eslint/ban-ts-comment": [
				"error",
				{ "ts-expect-error": "allow-with-description" }
			],
		},
	},
]);
