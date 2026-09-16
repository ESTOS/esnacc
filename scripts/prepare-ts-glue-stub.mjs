// Copies gluecode + sample stubs into typescript/stub for glue tests.
import { cpSync, existsSync, mkdirSync, readdirSync, rmSync } from "node:fs";
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
 * Syncs gluecode and sample stubs into tests/stub.
 * @param options.clean - remove stub output before copy (default false; overwrites files either way)
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
		cpSync(join(paths.glueDir, name), join(paths.stub, name), { force: true });
	}

	for (const name of STUB_FILES) {
		cpSync(join(paths.stubDir, name), join(paths.stub, name), { force: true });
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
