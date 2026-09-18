// Runs C++ CTest suites and TypeScript glue tests (same as VS Code \"Run all tests\" task).
import { spawnSync } from "node:child_process";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { findCmakeBuildDir, repoRoot, runCppCtest } from "./cmake-build-dir.mjs";

const scriptDir = dirname(fileURLToPath(import.meta.url));

let exitCode = 0;

console.log("=== C++ tests (CTest, excluding TS glue ctest) ===\n");
const buildDir = findCmakeBuildDir();
const cppStatus = runCppCtest(buildDir);
if (cppStatus !== 0) {
	exitCode = cppStatus;
}

console.log("\n=== TypeScript glue tests (same specs as typescript/*.test.ts) ===\n");
const tsResult = spawnSync(process.execPath, [join(scriptDir, "run-ts-glue-test-suite.mjs")], {
	cwd: repoRoot,
	stdio: "inherit",
	shell: false,
});
if (tsResult.error) {
	throw tsResult.error;
}
if (tsResult.status !== 0) {
	exitCode = tsResult.status ?? 1;
}

process.exit(exitCode);
