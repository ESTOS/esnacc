// Runs one top-level node:test case from a TypeScript glue spec.
// CTest starts this once per discovered test() so the CI summary lists each case.
// Exits 0 only when that exact case passes. A pattern that matches nothing is a failure:
// node --test still exits 0 and reports the file itself as a passing test.
import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const repoRoot = join(dirname(fileURLToPath(import.meta.url)), "..");
const testDir = join(repoRoot, "typescript");
const registerUrl = pathToFileURL(join(repoRoot, "scripts/ts-glue-test-register.mjs")).href;
const testFileArg = process.argv[2];
const testName = process.argv[3];

if (!testFileArg || testName === undefined) {
	console.error("usage: node run-ts-glue-one-test.mjs <test-file.ts> <exact test name>");
	process.exit(1);
}

/**
 * Escapes a test title for --test-name-pattern. The pattern is anchored so a shorter
 * title cannot run a longer one (for example " (JSON)" next to " (BER)").
 * @param {string} value
 * @returns {string}
 */
function escapeRegExp(value) {
	return value.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

/**
 * True when TAP reports this exact case as passed. The file-level "ok" line that node
 * emits when the name pattern matches nothing does not satisfy this.
 * @param {string} output
 * @param {string} name
 * @returns {boolean}
 */
function tapPassedExact(output, name) {
	const line = new RegExp(`^ok \\d+ - ${escapeRegExp(name)}$`, "m");
	return line.test(output);
}

/**
 * Last TAP summary count for a label (`# pass 1`). Null when the runner printed no summary.
 * @param {string} output
 * @param {string} label
 * @returns {number | null}
 */
function tapCount(output, label) {
	const matches = [...output.matchAll(new RegExp(`^# ${label} (\\d+)$`, "gm"))];
	if (matches.length === 0) {
		return null;
	}
	return Number(matches[matches.length - 1][1]);
}

const testFile = resolve(testDir, testFileArg);
const hasLocalTsx = existsSync(join(testDir, "node_modules/tsx/package.json"));
const pattern = `^${escapeRegExp(testName)}$`;
const env = { ...process.env };
const nodeModules = join(repoRoot, "samples/ts-microservice/node-client/node_modules");
if (existsSync(join(nodeModules, "@estos/asn1ts"))) {
	env.NODE_PATH = nodeModules;
}

// shell:false so the name pattern (parentheses, spaces) reaches node as one argument.
const args = hasLocalTsx
	? [
		"--import",
		registerUrl,
		"--import",
		"tsx",
		"--test",
		"--test-concurrency=1",
		"--test-reporter=tap",
		`--test-name-pattern=${pattern}`,
		testFile,
	]
	: [
		"--yes",
		"tsx",
		"--import",
		registerUrl,
		"--test",
		"--test-concurrency=1",
		"--test-reporter=tap",
		`--test-name-pattern=${pattern}`,
		testFile,
	];

const result = hasLocalTsx
	? spawnSync(process.execPath, args, { cwd: testDir, env, encoding: "utf8" })
	: spawnSync("npx", args, { cwd: testDir, env, encoding: "utf8" });

const output = `${result.stdout ?? ""}${result.stderr ?? ""}`;
const pass = tapCount(output, "pass");
const fail = tapCount(output, "fail");
const ranExactCase = tapPassedExact(output, testName);
if (result.error || result.status !== 0 || pass !== 1 || fail !== 0 || !ranExactCase) {
	if (result.error) {
		console.error(result.error);
	}
	process.stdout.write(result.stdout ?? "");
	process.stderr.write(result.stderr ?? "");
	if (!ranExactCase && fail === 0) {
		console.error(`glue test did not run: ${testFileArg} :: ${testName}`);
	}
	process.exit(result.status && result.status !== 0 ? result.status : 1);
}
