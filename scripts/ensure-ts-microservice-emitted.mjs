// Emits node-client/dist when sample TypeScript is newer than the integration test file.
// The VS Code pretest and the CI microservice runner both call this so a new
// test() is in the file the explorer and node --test actually read.
import { spawnSync } from "node:child_process";
import { existsSync, readdirSync, statSync } from "node:fs";
import { join } from "node:path";

/**
 * Newest mtime under a directory, skipping node_modules.
 * @param {string} dir
 * @returns {number}
 */
function newestMtime(dir) {
	let newest = 0;
	for (const name of readdirSync(dir, { withFileTypes: true })) {
		if (name.name === "node_modules") {
			continue;
		}
		const full = join(dir, name.name);
		if (name.isDirectory()) {
			newest = Math.max(newest, newestMtime(full));
			continue;
		}
		if (name.name.endsWith(".ts")) {
			newest = Math.max(newest, statSync(full).mtimeMs);
		}
	}
	return newest;
}

/**
 * Runs the node-client tsc when sources are newer than the emitted test file.
 * Skips when the sample's TypeScript install is missing and a previous emit exists.
 * @param {string} repoRoot
 */
export function ensureMicroserviceTestsEmitted(repoRoot) {
	const clientRoot = join(repoRoot, "samples/ts-microservice/node-client");
	const srcRoot = join(clientRoot, "src");
	const outFile = join(clientRoot, "dist/microservice.integration.test.js");
	if (!existsSync(srcRoot)) {
		return;
	}
	const newest = newestMtime(srcRoot);
	let outTime = 0;
	try {
		outTime = statSync(outFile).mtimeMs;
	} catch {
		outTime = 0;
	}
	if (existsSync(outFile) && outTime >= newest) {
		return;
	}
	const tsc = join(clientRoot, "node_modules/typescript/lib/tsc.js");
	if (!existsSync(tsc)) {
		console.error("node-client TypeScript is not installed. Build the snacc-ts-all target so microservice tests can emit.");
		if (!existsSync(outFile)) {
			process.exit(1);
		}
		return;
	}
	const result = spawnSync(process.execPath, [tsc, "-p", "."], {
		cwd: clientRoot,
		stdio: "inherit",
	});
	if ((result.status ?? 1) !== 0) {
		process.exit(result.status ?? 1);
	}
}
