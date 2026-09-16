// Run: npx tsx typescript/TSLogicalFailure.loopback.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import * as ENetUC_Event_Manager from "./stub/ENetUC_Event_Manager.js";
import * as ENetUC_Event_Manager_Converter from "./stub/ENetUC_Event_Manager_Converter.js";
import * as ENetUC_Settings_Manager from "./stub/ENetUC_Settings_Manager.js";
import * as ENetUC_Settings_Manager_Converter from "./stub/ENetUC_Settings_Manager_Converter.js";
import { OperationIDs as SettingsOpIds } from "./stub/ENetUC_Settings_ManagerROSE.js";
import { InvokeProblemenum, type ROSEReject } from "./stub/SNACCROSE.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";
import { AsnInvokeProblem, handleRoseReject } from "./stub/TSROSEBase.js";
import { assertAsnRequestError, isRoseReject } from "./support/rose_result_codes.js";
import { SampleRuntimeHarness } from "./support/sample_runtime_harness.js";
import { asnGetSettingsResultTemplate, roseHandleInvokeTemplate, roseInvoke } from "./support/snacc_test_helpers.js";

async function unknownOperationReturnsReject(encoding: EASN1TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);

	const result = await harness.clientSettingsRose.handleInvoke(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		asnGetSettingsResultTemplate(),
		4999,
		"asnUnknownOperation",
		ENetUC_Settings_Manager_Converter.AsnGetSettingsArgument_Converter,
		ENetUC_Settings_Manager_Converter.AsnGetSettingsResult_Converter,
		{ encoding },
	);
	assert.ok(result instanceof AsnInvokeProblem);
	assert.equal(result.value, InvokeProblemenum.unrecognisedOperation);
}

async function missingHandlerReturnsReject(encoding: EASN1TransportEncoding): Promise<void> {
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
}

async function handlerCanReturnApplicationError(encoding: EASN1TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);
	harness.configureServerHandlers({ getSettingsReturnsError: true });

	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding },
	);
	assertAsnRequestError(result, 7001);
}

async function mistypedArgumentIsRejected(encoding: EASN1TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);

	const wrong = new ENetUC_Event_Manager.AsnCreateFancyEventsArgument({ iEventDelay: 1, iEventCount: 2 });
	const result = await harness.clientSettingsRose.handleInvoke(
		wrong,
		roseHandleInvokeTemplate(
			new ENetUC_Settings_Manager.AsnSetSettingsResult({} as ENetUC_Settings_Manager.AsnSetSettingsResult),
		),
		SettingsOpIds.OPID_asnSetSettings,
		"asnSetSettings",
		ENetUC_Event_Manager_Converter.AsnCreateFancyEventsArgument_Converter,
		ENetUC_Settings_Manager_Converter.AsnSetSettingsResult_Converter,
		{ encoding },
	);
	assert.ok(result instanceof AsnInvokeProblem);
	assert.notEqual(result.value, 0);
}

test("UnknownOperationReturnsReject (BER)", () => unknownOperationReturnsReject(EASN1TransportEncoding.BER));
test("UnknownOperationReturnsReject (JSON)", () => unknownOperationReturnsReject(EASN1TransportEncoding.JSON));
test("MissingHandlerReturnsReject (BER)", () => missingHandlerReturnsReject(EASN1TransportEncoding.BER));
test("MissingHandlerReturnsReject (JSON)", () => missingHandlerReturnsReject(EASN1TransportEncoding.JSON));
test("HandlerCanReturnApplicationError (BER)", () => handlerCanReturnApplicationError(EASN1TransportEncoding.BER));
test("HandlerCanReturnApplicationError (JSON)", () => handlerCanReturnApplicationError(EASN1TransportEncoding.JSON));
test("MistypedArgumentIsRejected (BER)", () => mistypedArgumentIsRejected(EASN1TransportEncoding.BER));
test("MistypedArgumentIsRejected (JSON)", () => mistypedArgumentIsRejected(EASN1TransportEncoding.JSON));

test("MissingArgumentIsRejected (JSON)", async () => {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.JSON);

	const wire = await harness.sendClientInvokeMessage(
		roseInvoke({
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
