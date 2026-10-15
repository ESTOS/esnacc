// Emits typescript/*.ts and typescript/stub to typescript/dist.
// The glue suite and the VS Code pretest call this. When the emit stamp is newer
// than the TypeScript sources, it returns without starting tsc.
import { spawnSync } from "node:child_process";
import { closeSync, mkdirSync, openSync, readdirSync, statSync, unlinkSync, writeFileSync } from "node:fs";
import { join } from "node:path";

const LOCK_WAIT_MS = 15 * 60 * 1000;
const STALE_LOCK_MS = 15 * 60 * 1000;

/**
 * Newest mtime among TypeScript inputs for the emit.
 * Skips node_modules and dist. Includes tsconfig.emit.json so a compiler-option edit rebuilds.
 * @param {string} testDir
 * @returns {number}
 */
function newestInputMtime(testDir) {
	let newest = 0;
	const visit = (dir) => {
		for (const entry of readdirSync(dir, { withFileTypes: true })) {
			if (entry.name === "node_modules" || entry.name === "dist") {
				continue;
			}
			const full = join(dir, entry.name);
			if (entry.isDirectory()) {
				visit(full);
				continue;
			}
			if (entry.name.endsWith(".ts") || entry.name === "tsconfig.emit.json") {
				const mtime = statSync(full).mtimeMs;
				if (mtime > newest) {
					newest = mtime;
				}
			}
		}
	};
	visit(testDir);
	return newest;
}

/**
 * Blocks the current thread while another ctest process holds the emit lock.
 * @param {number} ms
 */
function sleep(ms) {
	Atomics.wait(new Int32Array(new SharedArrayBuffer(4)), 0, 0, ms);
}

/**
 * Drops a lock left by a crashed tsc. A lock older than STALE_LOCK_MS is abandoned.
 * @param {string} lockPath
 */
function removeStaleLock(lockPath) {
	try {
		if (Date.now() - statSync(lockPath).mtimeMs > STALE_LOCK_MS) {
			unlinkSync(lockPath);
		}
	} catch {
		// Another waiter removed it, or it never existed.
	}
}

/**
 * True when dist/.emit-stamp exists and is at least as new as every TypeScript input.
 * @param {string} stamp
 * @param {string} testDir
 * @returns {boolean}
 */
function isEmitCurrent(stamp, testDir) {
	let stampTime;
	try {
		stampTime = statSync(stamp).mtimeMs;
	} catch {
		return false;
	}
	return stampTime >= newestInputMtime(testDir);
}

/**
 * Runs tsc --noCheck into typescript/dist when sources are newer than the emit stamp.
 * --noCheck matches tsx, which transpiles without failing the run on type errors.
 * Source maps are written next to the JavaScript so stack traces still name the .ts files.
 * Callers are CTest glue cases. Parallel runs share dist/.emit.lock.
 * @param {string} testDir - Absolute path to the typescript/ directory.
 */
export function ensureTsGlueEmitted(testDir) {
	const distDir = join(testDir, "dist");
	const stamp = join(distDir, ".emit-stamp");
	if (isEmitCurrent(stamp, testDir)) {
		return;
	}

	mkdirSync(distDir, { recursive: true });
	const lockPath = join(distDir, ".emit.lock");
	const deadline = Date.now() + LOCK_WAIT_MS;
	for (;;) {
		if (isEmitCurrent(stamp, testDir)) {
			return;
		}
		let fd;
		try {
			fd = openSync(lockPath, "wx");
		} catch (error) {
			if (error && error.code !== "EEXIST") {
				throw error;
			}
			removeStaleLock(lockPath);
			if (Date.now() > deadline) {
				throw new Error("timed out waiting for TypeScript glue emit");
			}
			sleep(200);
			continue;
		}
		try {
			if (isEmitCurrent(stamp, testDir)) {
				return;
			}
			const tsc = join(testDir, "node_modules/typescript/lib/tsc.js");
			const result = spawnSync(process.execPath, [tsc, "-p", join(testDir, "tsconfig.emit.json"), "--noCheck", "--pretty", "false"], {
				cwd: testDir,
				stdio: "inherit",
			});
			if (result.error) {
				throw result.error;
			}
			if (result.status !== 0) {
				process.exit(result.status ?? 1);
			}
			writeFileSync(stamp, "");
			return;
		} finally {
			closeSync(fd);
			try {
				unlinkSync(lockPath);
			} catch {
				// The lock is already gone.
			}
		}
	}
}
