// One TypeScript glue runner for CI, `pnpm test`, and the shell wrappers.
// The VS Code node:test explorer runs this same command: node --enable-source-maps --test
// on typescript/dist, after the same stub copy and emit.
// Does not build the compiler and does not run samples/prepare.js.
import { spawnSync } from "node:child_process";
import { existsSync, readFileSync, readdirSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { ensureTsGlueEmitted } from "./ensure-ts-glue-emitted.mjs";
import { wrapNodeJUnitForGitHub } from "./node-junit-for-github.mjs";
import { prepareTsGlueStub } from "./prepare-ts-glue-stub.mjs";

const repoRoot = join(dirname(fileURLToPath(import.meta.url)), "..");
const testDir = join(repoRoot, "typescript");

const watch = process.argv.includes("--watch");
const requested = process.argv.slice(2).filter((arg) => arg !== "--watch");

/**
 * Maps a source or emitted test path onto the JavaScript file node --test runs.
 * @param {string} file - A *.test.ts path, a dist/*.test.js path, or a file name.
 * @returns {string}
 */
function emittedTestPath(file) {
	const normalized = file.replace(/\\/g, "/");
	if (normalized.startsWith("dist/")) {
		return normalized;
	}
	const name = normalized.split("/").pop() ?? normalized;
	return `dist/${name.replace(/\.ts$/i, ".js")}`;
}

prepareTsGlueStub();
ensureTsGlueEmitted(testDir);

const env = { ...process.env };
const nodeModules = join(repoRoot, "samples/ts-microservice/node-client/node_modules");
if (existsSync(join(nodeModules, "@estos/asn1ts"))) {
	env.NODE_PATH = nodeModules;
}

// Pass each *.test.js. `node --test` on the dist directory itself tries to load it as a module.
const targets = requested.length > 0
	? requested.map(emittedTestPath)
	: readdirSync(join(testDir, "dist"))
		.filter((name) => name.endsWith(".test.js"))
		.map((name) => join("dist", name));
if (targets.length === 0) {
	console.error("No emitted glue tests in typescript/dist");
	process.exit(1);
}
const junitPath = join(testDir, "dist", "glue-junit.xml");
// spec stays on stdout so the CI log is readable. junit is the file GitHub lists per test.
// Watch mode keeps a single reporter; the report file is only for a finished run.
const reporterArgs = watch
	? []
	: ["--test-reporter=spec", "--test-reporter-destination=stdout", "--test-reporter=junit", `--test-reporter-destination=${junitPath}`];
const args = ["--enable-source-maps", "--test", ...reporterArgs, ...(watch ? ["--watch"] : []), ...targets];
const result = spawnSync(process.execPath, args, {
	cwd: testDir,
	env,
	stdio: "inherit",
});
if (!watch && existsSync(junitPath)) {
	writeFileSync(junitPath, wrapNodeJUnitForGitHub(readFileSync(junitPath, "utf8"), repoRoot));
}
if (result.error) {
	console.error(result.error);
}
process.exit(result.status ?? 1);
