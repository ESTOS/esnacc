// Shared CMake build-directory resolution for snacclib7 scripts.
import { spawnSync } from "node:child_process";
import { existsSync, mkdirSync, readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const scriptDir = dirname(fileURLToPath(import.meta.url));
export const repoRoot = join(scriptDir, "..");

const BUILD_DIR_CANDIDATES = [
	"build/x64_vc145",
	"build/win32_vc145",
	"build/release",
	"build/debug",
	"build_win/release",
	"build_win/debug",
	"build",
];

const CPP_TEST_TARGETS = [
	"snacc-cpp-tests",
];

/** Active CMake configuration (Release/Debug). */
export function getBuildConfiguration() {
	return process.env.SNACC_CONFIGURATION || "Release";
}

/**
 * Resolves the CMake build tree (existing cache or default `build/`).
 * @returns {string} Absolute build directory path.
 */
export function findCmakeBuildDir() {
	if (process.env.SNACC_CMAKE_BUILD_DIR) {
		return join(repoRoot, process.env.SNACC_CMAKE_BUILD_DIR);
	}
	for (const candidate of BUILD_DIR_CANDIDATES) {
		const buildDir = join(repoRoot, candidate);
		if (existsSync(join(buildDir, "CMakeCache.txt"))) {
			return buildDir;
		}
	}
	return join(repoRoot, "build");
}

/**
 * Reads a single key from CMakeCache.txt.
 * @param {string} cacheFile
 * @param {string} key
 * @returns {string | null}
 */
export function readCacheValue(cacheFile, key) {
	if (!existsSync(cacheFile)) {
		return null;
	}
	const prefix = `${key}:`;
	for (const line of readFileSync(cacheFile, "utf8").split(/\r?\n/)) {
		if (!line.startsWith(prefix)) {
			continue;
		}
		let value = line.slice(prefix.length);
		if (value.startsWith("UNINITIALIZED=")) {
			value = value.slice("UNINITIALIZED=".length);
		}
		return value;
	}
	return null;
}

/**
 * Runs cmake configure when the tree is missing or BUILD_TESTING is off.
 * @param {string} buildDir
 */
export function ensureCmakeConfigured(buildDir) {
	const cacheFile = join(buildDir, "CMakeCache.txt");
	const needsConfigure = !existsSync(cacheFile)
		|| readCacheValue(cacheFile, "BUILD_TESTING") !== "ON";

	if (!needsConfigure) {
		return;
	}

	mkdirSync(buildDir, { recursive: true });
	const args = [
		"-S",
		repoRoot,
		"-B",
		buildDir,
		"-DBUILD_TESTING=ON",
		"-DMSVC_STATIC_RUNTIME=ON",
		`-DCOMPILER_OUTPUT_PATH=${join(repoRoot, "output/bin")}`,
		"-DCOMPILER_OUTPUT_NAME=esnacc",
	];
	if (process.platform === "win32" && !process.env.SNACC_CMAKE_GENERATOR) {
		args.push("-A", "x64");
	}
	if (process.env.SNACC_CMAKE_GENERATOR) {
		args.splice(0, 0, "-G", process.env.SNACC_CMAKE_GENERATOR);
	}

	const result = spawnSync("cmake", args, { cwd: repoRoot, stdio: "inherit", shell: false });
	if (result.error) {
		throw result.error;
	}
	if (result.status !== 0) {
		process.exit(result.status ?? 1);
	}
}

/**
 * Builds compiler and C++ test executables.
 * @param {string} buildDir
 */
export function buildCppTestTargets(buildDir) {
	const config = getBuildConfiguration();
	// Rebuild gtest after CRT setting changes (shared snacc_gtest.cmake).
	const cleanGtest = spawnSync("cmake", [
		"--build",
		buildDir,
		"--config",
		config,
		"--target",
		"gtest",
		"gtest_main",
		"--clean-first",
	], { cwd: repoRoot, stdio: "inherit", shell: false });
	if (cleanGtest.error) {
		throw cleanGtest.error;
	}
	if (cleanGtest.status !== 0) {
		process.exit(cleanGtest.status ?? 1);
	}

	const args = [
		"--build",
		buildDir,
		"--config",
		config,
		"--target",
		"compiler",
		...CPP_TEST_TARGETS,
	];
	const result = spawnSync("cmake", args, { cwd: repoRoot, stdio: "inherit", shell: false });
	if (result.error) {
		throw result.error;
	}
	if (result.status !== 0) {
		process.exit(result.status ?? 1);
	}
}

/**
 * Runs CTest for discovered C++ tests (excludes the monolithic TS glue ctest).
 * @param {string} buildDir
 * @returns {number} Exit code from ctest.
 */
export function runCppCtest(buildDir) {
	const config = getBuildConfiguration();
	const args = [
		"--test-dir",
		buildDir,
		"-C",
		config,
		"--output-on-failure",
		"-LE",
		"ts",
	];
	const result = spawnSync("ctest", args, { cwd: repoRoot, stdio: "inherit", shell: false });
	if (result.error) {
		throw result.error;
	}
	return result.status ?? 1;
}

export { CPP_TEST_TARGETS };
