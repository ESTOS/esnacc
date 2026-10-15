// Runs one glue spec through the shared suite runner.
// Callers pass the TypeScript file name. The suite emits and runs the JavaScript.
import { spawnSync } from "node:child_process";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const testFile = process.argv[2];
if (!testFile) {
	console.error("usage: node run-ts-glue-test-file.mjs <test-file.ts>");
	process.exit(1);
}

const suite = join(dirname(fileURLToPath(import.meta.url)), "run-ts-glue-test-suite.mjs");
const result = spawnSync(process.execPath, [suite, testFile], { stdio: "inherit" });
if (result.error) {
	console.error(result.error);
}
process.exit(result.status ?? 1);
