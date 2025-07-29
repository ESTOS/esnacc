import config from "@estos/config-eslint";

export default [...config, {
	ignores: ["node_modules", "dist"],
	languageOptions: {
		parserOptions: { tsconfigRootDir: import.meta.dirname, projectService: { defaultProject: "tsconfig.json" } },
	},
	rules: { "no-debugger": "off", "@typescript-eslint/no-misused-promises": ["error", { checksVoidReturn: false }] },
}];
