// Fast path for the VS Code Node test explorer and for one-off runs.
// Copies glue and sample stubs only when those files are newer, then emits
// typescript/dist only when TypeScript sources are newer. Does not build esnacc
// and does not run samples/prepare.js.
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { ensureTsGlueEmitted } from "./ensure-ts-glue-emitted.mjs";
import { prepareTsGlueStub } from "./prepare-ts-glue-stub.mjs";

const repoRoot = join(dirname(fileURLToPath(import.meta.url)), "..");
const testDir = join(repoRoot, "typescript");

prepareTsGlueStub();
ensureTsGlueEmitted(testDir);
