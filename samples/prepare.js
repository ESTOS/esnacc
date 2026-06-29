#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");
const { spawnSync } = require("child_process");

const SAMPLES_DIR = __dirname;
const INTERFACE_DIR = path.join(SAMPLES_DIR, "interface");
const BIN_DIR = path.join(SAMPLES_DIR, "bin");
const NODE_VERSION = process.env.SNACC_NODE_VERSION || "24";

const PNPM_PACKAGES = [
	"ts-microservice/browser-client",
	"ts-microservice/node-server",
	"ts-microservice/node-client",
];
const PNPM_WORKSPACE_ROOT = path.join(SAMPLES_DIR, "ts-microservice");
const PNPM_WORKSPACE_FILE = path.join(PNPM_WORKSPACE_ROOT, "pnpm-workspace.yaml");

const COMPILER_CANDIDATES = process.platform === "win32"
	? ["esnaccd.exe", "esnacc.exe", "esnacc7d.exe", "esnacc7.exe"]
	: ["esnaccd", "esnacc"];

function fileIsExecutable(filePath) {
	try {
		fs.accessSync(filePath, fs.constants.X_OK);
		return true;
	} catch {
		return false;
	}
}

function resolveCompiler() {
	if (process.env.SNACC_COMPILER) {
		return path.resolve(process.env.SNACC_COMPILER);
	}
	if (process.env.CMAKE_COMPILER_TARGET) {
		return path.resolve(process.env.CMAKE_COMPILER_TARGET);
	}

	const searchDirs = [
		path.join(SAMPLES_DIR, "..", "output", "bin"),
		BIN_DIR,
		path.join(SAMPLES_DIR, "..", "..", "..", "buildtools"),
	];

	for (const dir of searchDirs) {
		for (const name of COMPILER_CANDIDATES) {
			const candidate = path.join(dir, name);
			if (fs.existsSync(candidate) && (process.platform === "win32" || fileIsExecutable(candidate))) {
				return candidate;
			}
		}
	}

	for (const name of COMPILER_CANDIDATES) {
		const lookup = spawnSync(process.platform === "win32" ? "where" : "which", [name], {
			encoding: "utf8",
			shell: process.platform === "win32",
		});
		if (lookup.status === 0 && lookup.stdout.trim()) {
			return lookup.stdout.trim().split(/\r?\n/)[0];
		}
	}

	return null;
}

function emptyDirectory(dir) {
	fs.mkdirSync(dir, { recursive: true });
	for (const entry of fs.readdirSync(dir)) {
		fs.rmSync(path.join(dir, entry), { recursive: true, force: true });
	}
}

function removeGeneratedJsonFiles(dir) {
	fs.mkdirSync(dir, { recursive: true });
	for (const entry of fs.readdirSync(dir)) {
		if (entry.endsWith(".json")) {
			fs.rmSync(path.join(dir, entry), { force: true });
		}
	}
}

function listAsn1Files() {
	return fs.readdirSync(INTERFACE_DIR)
		.filter((name) => name.endsWith(".asn1"))
		.map((name) => path.join(INTERFACE_DIR, name));
}

function runCompiler(compiler, outputDir, compilerArgs, asn1Files) {
	const args = [
		...compilerArgs,
		`-node:${NODE_VERSION}`,
		"-comments",
		"-versionfile",
		...asn1Files,
	];
	const result = spawnSync(compiler, args, {
		cwd: outputDir,
		stdio: "inherit",
		shell: false,
	});
	if (result.error) {
		throw result.error;
	}
	if (result.status !== 0) {
		return result.status ?? 1;
	}
	return 0;
}

function generateStubs() {
	const compiler = resolveCompiler();
	if (!compiler) {
		console.error("error: esnacc compiler not found. Build esnacc first, then run prepare.bat (Windows) or ./prepare.sh (Linux).");
		return 1;
	}

	console.log(`Using compiler: ${compiler}`);

	const asn1Files = listAsn1Files();
	if (asn1Files.length === 0) {
		console.error(`error: no .asn1 files found in ${INTERFACE_DIR}`);
		return 1;
	}

	const stubSets = [
		["browser-client", "ts-microservice/browser-client/src/stub", ["-JTE", "-j", "-RTS_CLIENT_BROWSER"]],
		["node-client", "ts-microservice/node-client/src/stub", ["-JTE", "-j", "-RTS_CLIENT_NODE"]],
		["node-server", "ts-microservice/node-server/src/stub", ["-JTE", "-j", "-RTS_SERVER"]],
	];

	for (const [label, stubDir, compilerArgs] of stubSets) {
		const absDir = path.resolve(SAMPLES_DIR, stubDir);
		console.log(`Generating ${label} stubs into ${absDir}`);
		emptyDirectory(absDir);
		const status = runCompiler(compiler, absDir, compilerArgs, asn1Files);
		if (status !== 0) {
			return status;
		}
	}

	const openApiSets = [
		["openapi example", "ts-microservice/openapi/example"],
		["openapi unpkg-example", "ts-microservice/openapi/unpkg-example"],
	];

	for (const [label, outputDir] of openApiSets) {
		const absDir = path.resolve(SAMPLES_DIR, outputDir);
		console.log(`Generating ${label} OpenAPI into ${absDir}`);
		removeGeneratedJsonFiles(absDir);
		const status = runCompiler(compiler, absDir, ["-JO"], asn1Files);
		if (status !== 0) {
			return status;
		}
	}

	console.log("Stub generation finished.");
	return 0;
}

function findExecutable(command) {
	const lookup = spawnSync(process.platform === "win32" ? "where" : "which", [command], {
		encoding: "utf8",
		shell: process.platform === "win32",
	});
	if (lookup.status !== 0 || !lookup.stdout.trim()) {
		return null;
	}
	return lookup.stdout.trim().split(/\r?\n/)[0];
}

function removeNodeModules(packageDir) {
	const nodeModulesDir = path.join(packageDir, "node_modules");
	if (fs.existsSync(nodeModulesDir)) {
		fs.rmSync(nodeModulesDir, { recursive: true, force: true });
	}
}

function runPnpmInstall(label, cwd, installArgs) {
	console.log(`\n=== ${label} ===\n`);
	const result = process.platform === "win32"
		? spawnSync(`pnpm ${installArgs.join(" ")}`, { cwd, stdio: "inherit", shell: true })
		: spawnSync("pnpm", installArgs, { cwd, stdio: "inherit", shell: false });
	if (result.error) {
		throw result.error;
	}
	if (result.status !== 0) {
		process.exit(result.status ?? 1);
	}
}

function prepare(options = {}) {
	const skipStubs = options.skipStubs ?? process.argv.includes("--skip-stubs");
	const skipInstall = options.skipInstall ?? process.argv.includes("--skip-install");
	const frozenLockfile = options.frozenLockfile ?? (
		process.argv.includes("--frozen-lockfile") || process.env.SNACC_PREPARE_FROZEN === "1"
	);
	const cleanInstall = options.cleanInstall ?? (
		process.argv.includes("--clean") || process.env.SNACC_PREPARE_CLEAN === "1"
	);

	if (!skipStubs) {
		const stubStatus = generateStubs();
		if (stubStatus !== 0) {
			return stubStatus;
		}
	}

	if (!skipInstall) {
		if (!findExecutable("pnpm")) {
			console.error("error: pnpm not found. Enable it with: corepack enable");
			return 1;
		}

		const installArgs = frozenLockfile ? ["install", "--frozen-lockfile"] : ["install"];

		if (fs.existsSync(PNPM_WORKSPACE_FILE)) {
			if (cleanInstall) {
				removeNodeModules(PNPM_WORKSPACE_ROOT);
				for (const packageDir of PNPM_PACKAGES) {
					removeNodeModules(path.join(SAMPLES_DIR, packageDir));
				}
			}
			runPnpmInstall(`pnpm ${installArgs.join(" ")} in ts-microservice workspace`, PNPM_WORKSPACE_ROOT, installArgs);
		} else {
			for (const packageDir of PNPM_PACKAGES) {
				const absDir = path.join(SAMPLES_DIR, packageDir);
				if (!fs.existsSync(path.join(absDir, "package.json"))) {
					console.error(`error: package.json not found in ${absDir}`);
					return 1;
				}
				if (cleanInstall) {
					console.log(`Removing node_modules in ${packageDir}`);
					removeNodeModules(absDir);
				}
				runPnpmInstall(`pnpm ${installArgs.join(" ")} in ${packageDir}`, absDir, installArgs);
			}
		}
	}

	console.log("\nPrepare finished.");
	return 0;
}

if (require.main === module) {
	process.exit(prepare());
}

module.exports = { prepare, generateStubs, PNPM_PACKAGES };
