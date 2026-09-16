// Runs one TS glue test file (cross-platform; fixes Windows --import path issues).
import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const repoRoot = join(dirname(fileURLToPath(import.meta.url)), "..");
const testDir = join(repoRoot, "typescript");
const registerUrl = pathToFileURL(join(repoRoot, "scripts/ts-glue-test-register.mjs")).href;
const testArg = process.argv[2];

if (!testArg) {
	console.error("usage: node run-ts-glue-test-file.mjs <test-file.ts>");
	process.exit(1);
}

const testFile = resolve(testDir, testArg);
const hasLocalTsx = existsSync(join(testDir, "node_modules/tsx/package.json"));

const env = { ...process.env };
const nodeModules = join(repoRoot, "samples/ts-microservice/node-client/node_modules");
if (existsSync(join(nodeModules, "@estos/asn1ts"))) {
	env.NODE_PATH = nodeModules;
}

const result = hasLocalTsx
	? spawnSync(process.execPath, ["--import", registerUrl, "--import", "tsx", testFile], {
		cwd: testDir,
		env,
		stdio: "inherit",
	})
	: spawnSync("npx", ["--yes", "tsx", "--import", registerUrl, testFile], {
		cwd: testDir,
		env,
		stdio: "inherit",
		shell: true,
	});

if (result.error) {
	console.error(result.error);
}
if (result.status === null) {
	console.error("glue test runner failed to start");
}
process.exit(result.status ?? 1);
