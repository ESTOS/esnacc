// Runs the full TS glue test suite via node:test (npm test / CI helper).
import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { prepareTsGlueStub } from "./prepare-ts-glue-stub.mjs";

const repoRoot = join(dirname(fileURLToPath(import.meta.url)), "..");
const testDir = join(repoRoot, "typescript");
const registerUrl = pathToFileURL(join(repoRoot, "scripts/ts-glue-test-register.mjs")).href;

prepareTsGlueStub({ clean: true });

const env = { ...process.env };
const nodeModules = join(repoRoot, "samples/ts-microservice/node-client/node_modules");
if (existsSync(join(nodeModules, "@estos/asn1ts"))) {
	env.NODE_PATH = nodeModules;
}

const hasLocalTsx = existsSync(join(testDir, "node_modules/tsx/package.json"));
const args = hasLocalTsx
	? ["--import", registerUrl, "--import", "tsx", "--test", "*.test.ts"]
	: ["--yes", "tsx", "--import", registerUrl, "--test", "*.test.ts"];

const result = hasLocalTsx
	? spawnSync(process.execPath, args, { cwd: testDir, env, stdio: "inherit", shell: true })
	: spawnSync("npx", args, { cwd: testDir, env, stdio: "inherit", shell: true });

process.exit(result.status ?? 1);
