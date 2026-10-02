// Node --import hook: sync glue test stub before test modules load.
// CI sets SNACC_TS_GLUE_STUB_PREPARED after the ctest fixture has copied typescript/stub.
// Individual glue ctests then skip the copy so they can run in parallel without rewriting those files.
import { prepareTsGlueStub } from "./prepare-ts-glue-stub.mjs";

if (process.env.SNACC_TS_GLUE_STUB_PREPARED !== "1") {
	prepareTsGlueStub();
}
