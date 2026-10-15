// Copies gluecode + sample stubs into typescript/stub for glue tests.
import { cpSync, existsSync, mkdirSync, readdirSync, rmSync, statSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const scriptDir = dirname(fileURLToPath(import.meta.url));
const repoRoot = join(scriptDir, "..");

const STUB_FILES = [
	"ENetUC_Common.ts",
	"ENetUC_Common_Converter.ts",
	"ENetUC_Settings_Manager.ts",
	"ENetUC_Settings_Manager_Converter.ts",
	"ENetUC_Settings_ManagerROSE.ts",
	"ENetUC_Settings_ManagerROSE_Interface.ts",
	"ENetUC_Event_Manager.ts",
	"ENetUC_Event_Manager_Converter.ts",
	"ENetUC_Event_ManagerROSE.ts",
	"ENetUC_Event_ManagerROSE_Interface.ts",
];

/** Resolved paths for TS glue tests (repo-root relative). */
export function getTsGlueTestPaths() {
	const testDir = join(repoRoot, "typescript");
	return {
		repoRoot,
		testDir,
		stub: join(testDir, "stub"),
		glueDir: join(repoRoot, "compiler/back-ends/ts-gen/gluecode"),
		stubDir: join(repoRoot, "samples/ts-microservice/node-client/src/stub"),
		nodeModules: join(repoRoot, "samples/ts-microservice/node-client/node_modules"),
	};
}

/**
 * Copies src onto dest when dest is missing or older than src.
 * A test click calls this often. Unchanged files keep their mtime so the JS emit stays valid.
 * @param {string} src
 * @param {string} dest
 * @returns {boolean} true when the file was copied
 */
function copyIfNewer(src, dest) {
	if (!existsSync(src)) {
		throw new Error(`Missing glue input ${src}. Run samples/prepare.js when ASN.1 or the compiler changed.`);
	}
	if (existsSync(dest)) {
		const srcStat = statSync(src);
		const destStat = statSync(dest);
		if (destStat.mtimeMs >= srcStat.mtimeMs && destStat.size === srcStat.size) {
			return false;
		}
	}
	cpSync(src, dest, { force: true });
	return true;
}

/**
 * Syncs gluecode and sample stubs into typescript/stub.
 * Does not run the ASN.1 compiler or pnpm. Those stay on the snacc-ts-glue-prepare target.
 * @param options.clean - remove stub output before copy (default false)
 */
export function prepareTsGlueStub(options = {}) {
	const clean = options.clean === true;
	const paths = getTsGlueTestPaths();

	if (!existsSync(join(paths.nodeModules, "@estos/asn1ts"))) {
		throw new Error("Run scripts/setup-snacc-tests first (full setup) or scripts/setup-snacc-tests --quick.");
	}

	if (clean && existsSync(paths.stub)) {
		rmSync(paths.stub, { recursive: true, force: true });
	}

	mkdirSync(paths.stub, { recursive: true });

	for (const name of readdirSync(paths.glueDir)) {
		copyIfNewer(join(paths.glueDir, name), join(paths.stub, name));
	}

	for (const name of STUB_FILES) {
		copyIfNewer(join(paths.stubDir, name), join(paths.stub, name));
	}

	return paths;
}

function isMainModule() {
	const entry = process.argv[1];
	if (!entry) {
		return false;
	}
	return fileURLToPath(import.meta.url) === entry || fileURLToPath(import.meta.url) === join(process.cwd(), entry);
}

if (isMainModule()) {
	const clean = process.argv.includes("--clean");
	prepareTsGlueStub({ clean });
}
