// One-shot setup for C++ (CTest/GTest) and TypeScript glue tests in VS Code.
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import {
	buildCppTestTargets,
	ensureCmakeConfigured,
	findCmakeBuildDir,
	getBuildConfiguration,
	repoRoot,
} from "./cmake-build-dir.mjs";
import { setupTsGlueTests } from "./setup-ts-glue-tests.mjs";

const tsOnly = process.argv.includes("--ts-only");
const cppOnly = process.argv.includes("--cpp-only");
const quick = process.argv.includes("--quick");

function isMainModule() {
	const entry = process.argv[1];
	if (!entry) {
		return false;
	}
	return fileURLToPath(import.meta.url) === entry
		|| fileURLToPath(import.meta.url) === join(process.cwd(), entry);
}

/**
 * Configures CMake with BUILD_TESTING and builds C++ test executables.
 */
function setupCppTests() {
	const buildDir = findCmakeBuildDir();
	console.log(`\n=== C++ tests: CMake (${getBuildConfiguration()}) ===`);
	console.log(`Build directory: ${buildDir}\n`);
	ensureCmakeConfigured(buildDir);
	buildCppTestTargets(buildDir);
}

if (isMainModule()) {
	if (tsOnly) {
		setupTsGlueTests({ quick });
	} else if (cppOnly) {
		setupCppTests();
	} else {
		console.log("snacclib7 test setup: C++ (CMake/GTest) + TypeScript glue");
		setupTsGlueTests({ quick });
		setupCppTests();
	}

	console.log("\nAll requested tests are ready for VS Code Test Explorer.");
	console.log("  C++:  CMake Tools section (compiler + cpp-lib tests)");
	console.log("  TS:   JavaScript/TypeScript Testing section");
	console.log("  Run everything: task \"Run all tests\" or Testing sidebar ▶ at the root");
	console.log(`  CMake build dir: ${findCmakeBuildDir()}`);
}
