module.exports = {
	"extends": [
		".eslintrc.js"
	],
	"rules": {
		"no-console": "error",
		"no-unreachable": "warn",
		"no-unused-vars": "error",
		"no-empty": "error",
		"import/no-deprecated": "error",
		"@typescript-eslint/no-unused-vars": [
			"error", {
				"vars": "all",
				"args": "none",
				"ignoreRestSiblings": false
			}
		]
	},
	"overrides": [{
			"files": [
				"src/__test__/**/*.ts"
			],
			"rules": {
				"jsdoc/require-jsdoc": 0,
				"@typescript-eslint/no-explicit-any": 0,
				"no-console": 0
			}
		}
	]
}
