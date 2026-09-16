// One-shot setup for typescript/ glue tests (samples, pnpm, stub).
import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { repoRoot } from "./cmake-build-dir.mjs";
import { prepareTsGlueStub } from "./prepare-ts-glue-stub.mjs";

const scriptDir = dirname(fileURLToPath(import.meta.url));
const testDir = join(repoRoot, "typescript");

/**
 * Runs a subprocess; prints step banner and exits on failure.
 * @param {string} label
 * @param {string} command
 * @param {string[]} args
 * @param {{ cwd?: string; env?: NodeJS.ProcessEnv; shell?: boolean }} [options]
 */
function runStep(label, command, args, options = {}) {
	console.log(`\n=== ${label} ===\n`);
	const result = spawnSync(command, args, {
		cwd: options.cwd ?? repoRoot,
		env: options.env ?? process.env,
		stdio: "inherit",
		shell: options.shell ?? false,
	});
	if (result.error) {
		throw result.error;
	}
	if (result.status !== 0) {
		process.exit(result.status ?? 1);
	}
}

/** Builds or locates the esnacc compiler via ensure_compiler script. */
export function ensureCompiler() {
	const script = process.platform === "win32"
		? join(scriptDir, "ensure_compiler.bat")
		: join(scriptDir, "ensure_compiler.sh");
	if (process.platform === "win32") {
		runStep("Build or locate esnacc compiler", "cmd", ["/c", script]);
	} else {
		runStep("Build or locate esnacc compiler", "bash", [script]);
	}
}

/** Generates sample stubs and installs the pnpm workspace. */
export function prepareSamples() {
	runStep("Generate sample stubs and install pnpm workspace", process.execPath, [
		join(repoRoot, "samples/prepare.js"),
	]);
}

/** Installs pnpm workspace dependencies (root lockfile includes typescript/). */
export function installGlueTestDeps() {
	if (process.platform === "win32") {
		runStep("Install pnpm workspace dependencies", "cmd", ["/c", "pnpm", "install"], { cwd: repoRoot });
	} else {
		runStep("Install pnpm workspace dependencies", "pnpm", ["install"], { cwd: repoRoot });
	}
}

/** Copies gluecode and stubs into typescript/stub. */
export function syncStub() {
	console.log("\n=== Sync glue test stub ===\n");
	prepareTsGlueStub({ clean: true });
}

/**
 * Prepares TypeScript glue tests for VS Code / pnpm test.
 * @param {{ quick?: boolean }} [options]
 */
export function setupTsGlueTests(options = {}) {
	const quick = options.quick === true;
	console.log(
		quick
			? "TS glue test setup (quick): pnpm install + stub"
			: "TS glue test setup: compiler, samples, pnpm, stub",
	);

	if (!quick) {
		ensureCompiler();
		prepareSamples();
	}

	installGlueTestDeps();
	syncStub();

	const asn1ts = join(
		repoRoot,
		"samples/ts-microservice/node-client/node_modules/@estos/asn1ts",
	);
	if (!existsSync(asn1ts)) {
		console.error("\nerror: @estos/asn1ts still missing after setup.");
		process.exit(1);
	}
}

function isMainModule() {
	const entry = process.argv[1];
	if (!entry) {
		return false;
	}
	return fileURLToPath(import.meta.url) === entry
		|| fileURLToPath(import.meta.url) === join(process.cwd(), entry);
}

if (isMainModule()) {
	setupTsGlueTests({ quick: process.argv.includes("--quick") });
	console.log("\nTS glue tests are ready.");
	console.log("  VS Code: Testing sidebar (TypeScript section)");
}
