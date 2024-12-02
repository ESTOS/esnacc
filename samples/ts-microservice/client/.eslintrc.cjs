module.exports = {
	"ignorePatterns": [
		"node_modules",
		"dist",
		"coverage",
		"__snapshots__",
		"__mocks__",
		"*.test.ts",
		"*.md",
		"*.bak",
		"*.json",
		"*.css",
		"*.svg",
		"*.scss",
		"*.png",
		"*.sample",
		"*.woff2",
		"*.json",
		"*.buildagent",
		"*.eslintrc.cjs",
		"*.eslintrc.precommit.cjs"
	],
	"extends": [
		"standard",
		"eslint:recommended",
		"plugin:compat/recommended",
		"plugin:promise/recommended",
		"plugin:import/recommended",
		"plugin:jsdoc/recommended",
		"plugin:@typescript-eslint/eslint-recommended",
		"plugin:@typescript-eslint/recommended"
	],
	"parser": "@typescript-eslint/parser",
	"plugins": [
		"@typescript-eslint",
		"jsdoc",
		"eslint-plugin-tsdoc"
	],
	"env": {
		"browser": true
	},
	"parserOptions": {
		"ecmaVersion": 11,
		"sourceType" : "module",
		"project": "./tsconfig.json",
		"tsconfigRootDir": __dirname,
		"extraFileExtensions": []
	},
	"rules": {
		"brace-style": "off",
		"no-tabs": "off",
		"max-len": "off",
		"max-lines": ["warn", {
				"max": 1000,
				"skipComments": true,
				"skipBlankLines": true
			}
		],
		"indent": "off",
		"default-case": "error",
		"dot-notation": "off",
		"semi": [
			"error",
			"always"
		],
		"no-void": ["error", {
				"allowAsStatement": true
			}
		],
		"no-debugger": "off",
		"no-useless-constructor": "off",
		"quotes": [
			"error",
			"double", {
				"allowTemplateLiterals": true
			}
		],
		"space-before-function-paren": [
			"error", {
				"anonymous": "never",
				"named": "never",
				"asyncArrow": "always"
			}
		],
		"block-scoped-var": "error",
		"camelcase": "off",
		"curly": [
			"error",
			"multi-or-nest"
		],
		"nonblock-statement-body-position": [
			"error",
			"below"
		],
		"no-unneeded-ternary": "off",
		"no-use-before-define": "off",
		"no-empty": "off",
		"no-self-compare": "error",
		"no-var": "error",
		"prefer-const": "error",
		"@typescript-eslint/no-unused-vars": "off",
		"@typescript-eslint/no-use-before-define": "error",
		"@typescript-eslint/no-misused-new": "error",
		"@typescript-eslint/no-floating-promises": "error",
		"@typescript-eslint/no-misused-promises": ["error", {
				"checksVoidReturn": false
			}
		],
		"@typescript-eslint/indent": ["error", "tab", { "SwitchCase": 1 }],
		"@typescript-eslint/brace-style": ["error", "1tbs", { "allowSingleLine": true }],
		"@typescript-eslint/no-confusing-void-expression": "error",
		"@typescript-eslint/no-for-in-array": "error",
		"@typescript-eslint/no-unnecessary-type-arguments": "error",
		"@typescript-eslint/no-unsafe-assignment": "error",
		"@typescript-eslint/no-unsafe-member-access": "error",
		"@typescript-eslint/no-unsafe-return": "error",
		"@typescript-eslint/explicit-member-accessibility": "error",
		"key-spacing": ["error", {
				"beforeColon": false,
				"afterColon": true
			}
		],
		"promise/always-return": "off",
		"promise/no-callback-in-promise": "error",
		"promise/no-promise-in-callback": "error",
		"promise/no-nesting": "off",
		"promise/catch-or-return": [
			"error", {
				"allowFinally": true,
				"terminationMethod": [
					"catch",
					"finally"
				]
			}
		],
		"import/no-unresolved": "off",
		"@typescript-eslint/camelcase": "off",
		"@typescript-eslint/consistent-type-definitions": ["error", "interface"],
		"@typescript-eslint/no-explicit-any": ["error", {
				"ignoreRestArgs": true
			}
		],
		"@typescript-eslint/no-empty-function": "off",
		"@typescript-eslint/no-this-alias": "error",
		"@typescript-eslint/type-annotation-spacing": "error",
		"@typescript-eslint/ban-types": ["error", {
				"types": {
					"String": {
						"message": "Use string instead",
						"fixWith": "string"
					},
					"Boolean": {
						"message": "Use boolean instead",
						"fixWith": "boolean"
					},
					"Number": {
						"message": "Use number instead",
						"fixWith": "number"
					},
					"Object": {
						"message": "Use object instead",
						"fixWith": "object"
					},
					"Symbol": {
						"message": "Use symbol instead",
						"fixWith": "symbol"
					},
					"Function": {
						"message": "The `Function` type accepts any function-like value.\nIt provides no type safety when calling the function, which can be a common source of bugs.\nIt also accepts things like class declarations, which will throw at runtime as they will not be called with `new`.\nIf you are expecting the function to accept certain arguments, you should explicitly define the function shape."
					}
				},
				"extendDefaults": false
			}
		],
		"@typescript-eslint/member-delimiter-style": [
			"error", {
				"multiline": {
					"delimiter": "semi",
					"requireLast": true
				},
				"singleline": {
					"delimiter": "comma",
					"requireLast": false
				},
			}
		],
		"@typescript-eslint/naming-convention": [
			"error", {
				"selector": "variable",
				"format": ["camelCase", "snake_case"]
			}, {
				"selector": "parameter",
				"format": ["camelCase", "snake_case"]
			}, {
				"selector": "class",
				"format": ["PascalCase"]
			}, {
				"selector": "method",
				"format": null,
				"custom": {
					"regex": "^[a-z].*",
					"match": true
				}
			}, {
				"selector": "interface",
				"format": ["PascalCase"],
				"prefix": ["I"]
			}
		],
		"tsdoc/syntax": "warn",
		"jsdoc/check-indentation": 1,
		"jsdoc/check-syntax": 1,
		"jsdoc/no-types": 1,
		"jsdoc/tag-lines": ["error", "any", {"startLines":1, "tags":{"param":{"lines":"never"}}}],
		"jsdoc/require-description": 1,
		"jsdoc/require-param-type": 0,
		"jsdoc/require-returns-type": 0,
		"jsdoc/require-jsdoc": [1, {
				"require": {
					"ArrowFunctionExpression": false,
					"ClassDeclaration": true,
					"ClassExpression": true,
					"FunctionDeclaration": true,
					"FunctionExpression": true,
					"MethodDefinition": true
				}
			}
		]
	},
	"overrides": [{
			"files": [
				"*.js",
				"*.jsx"
			],
			"rules": {
				"@typescript-eslint/class-name-casing": "off",
				"@typescript-eslint/no-empty-function": "off",
				"@typescript-eslint/interface-name-prefix": "off",
				"@typescript-eslint/camelcase": "off",
				"@typescript-eslint/no-var-requires": "off",
				"@typescript-eslint/explicit-function-return-type": "off",
				"@typescript-eslint/explicit-member-accessibility": "off"
			}
		}, {
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
