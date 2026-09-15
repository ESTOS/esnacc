// Run: npx tsx compiler/back-ends/ts-gen/tests/TSLogicalFailure.loopback.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import * as ENetUC_Event_Manager from "./workdir/ENetUC_Event_Manager.js";
import * as ENetUC_Event_Manager_Converter from "./workdir/ENetUC_Event_Manager_Converter.js";
import * as ENetUC_Settings_Manager from "./workdir/ENetUC_Settings_Manager.js";
import * as ENetUC_Settings_Manager_Converter from "./workdir/ENetUC_Settings_Manager_Converter.js";
import { OperationIDs as SettingsOpIds } from "./workdir/ENetUC_Settings_ManagerROSE.js";
import { InvokeProblemenum, ROSEInvoke, type ROSEReject } from "./workdir/SNACCROSE.js";
import { ConverterErrors } from "./workdir/TSConverterBase.js";
import { AsnInvokeProblem, handleRoseReject } from "./workdir/TSROSEBase.js";
import { EASN1TransportEncoding } from "./workdir/TSInvokeContext.js";
import {
	assertAsnInvokeProblem,
	assertAsnRequestError,
	isRoseReject,
} from "./support/rose_result_codes.js";
import { SampleRuntimeHarness } from "./support/sample_runtime_harness.js";

const encodings = [
	{ label: "BER", encoding: EASN1TransportEncoding.BER },
	{ label: "JSON", encoding: EASN1TransportEncoding.JSON },
] as const;

for (const { label, encoding } of encodings) {
	test(`UnknownOperationReturnsReject (${label})`, async () => {
		const harness = new SampleRuntimeHarness();
		harness.resetState();
		harness.setEncoding(encoding);

		const result = await harness.clientSettingsRose.handleInvoke(
			new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
			new ENetUC_Settings_Manager.AsnGetSettingsResult(),
			4999,
			"asnUnknownOperation",
			ENetUC_Settings_Manager_Converter.AsnGetSettingsArgument_Converter,
			ENetUC_Settings_Manager_Converter.AsnGetSettingsResult_Converter,
			{ encoding },
		);
		assert.ok(result instanceof AsnInvokeProblem);
		assert.equal(result.value, InvokeProblemenum.unrecognisedOperation);
	});

	test(`MissingHandlerReturnsReject (${label})`, async () => {
		const harness = new SampleRuntimeHarness();
		harness.resetState();
		harness.setEncoding(encoding);
		harness.configureServerHandlers({ implementGetSettings: false });

		const result = await harness.clientSettingsRose.invoke_asnGetSettings(
			new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
			{ encoding },
		);
		assert.ok(result instanceof AsnInvokeProblem);
		assert.equal(result.value, InvokeProblemenum.unrecognisedOperation);
	});

	test(`HandlerCanReturnApplicationError (${label})`, async () => {
		const harness = new SampleRuntimeHarness();
		harness.resetState();
		harness.setEncoding(encoding);
		harness.configureServerHandlers({ getSettingsReturnsError: true });

		const result = await harness.clientSettingsRose.invoke_asnGetSettings(
			new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
			{ encoding },
		);
		assertAsnRequestError(result, 7001);
	});

	test(`MistypedArgumentIsRejected (${label})`, async () => {
		const harness = new SampleRuntimeHarness();
		harness.resetState();
		harness.setEncoding(encoding);

		const wrong = new ENetUC_Event_Manager.AsnCreateFancyEventsArgument({ iEventDelay: 1, iEventCount: 2 });
		const result = await harness.clientSettingsRose.handleInvoke(
			wrong,
			new ENetUC_Settings_Manager.AsnSetSettingsResult(),
			SettingsOpIds.OPID_asnSetSettings,
			"asnSetSettings",
			ENetUC_Event_Manager_Converter.AsnCreateFancyEventsArgument_Converter,
			ENetUC_Settings_Manager_Converter.AsnSetSettingsResult_Converter,
			{ encoding },
		);
		assert.ok(result instanceof AsnInvokeProblem);
		assert.notEqual(result.value, 0);
	});
}

test("MissingArgumentIsRejected (JSON)", async () => {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.JSON);

	const wire = await harness.sendClientInvokeMessage(
		new ROSEInvoke({
			invokeID: harness.clientTransport.getNextInvokeID(),
			operationID: 4101,
			operationName: "asnSetSettings",
		}),
		undefined,
		EASN1TransportEncoding.JSON,
	);
	const problem = handleRoseReject(wire as ROSEReject);
	assert.ok(isRoseReject(problem.value ?? 0) || problem.value === InvokeProblemenum.mistypedArgument);
});
