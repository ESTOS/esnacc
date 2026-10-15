// One runner for the sample client/server tests.
// VS Code runs the same emitted file, dist/microservice.integration.test.js, through node --test.
// CI runs this script so GitHub gets one row per test() in microservice-junit.xml.
// The file's before/after hooks start and stop node-server. Glue specs do not.
import { spawnSync } from "node:child_process";
import { existsSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { ensureMicroserviceTestsEmitted } from "./ensure-ts-microservice-emitted.mjs";
import { wrapNodeJUnitForGitHub } from "./node-junit-for-github.mjs";

const repoRoot = join(dirname(fileURLToPath(import.meta.url)), "..");
const clientRoot = join(repoRoot, "samples/ts-microservice/node-client");

ensureMicroserviceTestsEmitted(repoRoot);

const testFile = join(clientRoot, "dist/microservice.integration.test.js");
if (!existsSync(testFile)) {
	console.error(`Missing ${testFile}. Build the snacc-ts-all target first.`);
	process.exit(1);
}

const junitPath = join(clientRoot, "dist/microservice-junit.xml");
const args = [
	"--enable-source-maps",
	"--test",
	"--test-concurrency=1",
	"--test-reporter=spec",
	"--test-reporter-destination=stdout",
	"--test-reporter=junit",
	`--test-reporter-destination=${junitPath}`,
	testFile,
];
const result = spawnSync(process.execPath, args, {
	cwd: clientRoot,
	env: process.env,
	stdio: "inherit",
});
if (existsSync(junitPath)) {
	writeFileSync(junitPath, wrapNodeJUnitForGitHub(readFileSync(junitPath, "utf8"), repoRoot));
}
if (result.error) {
	console.error(result.error);
}
process.exit(result.status ?? 1);
