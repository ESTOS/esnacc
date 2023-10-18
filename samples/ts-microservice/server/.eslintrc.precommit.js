module.exports = {
	"extends": [
		".eslintrc.js"
	],
	"rules": {
		"no-console": "error",
		"no-unreachable": "warn",
		"no-unused-vars": "error",
		"no-empty": "error",
		"prefer-const": "error",
		"import/no-deprecated": "error",
		"@typescript-eslint/no-unused-vars": [
			"error", {
				"vars": "all",
				"args": "none",
				"ignoreRestSiblings": false
			}
		]
	}
}
