// Run: npx tsx typescript/TSTransportFailure.loopback.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import * as ENetUC_Settings_Manager from "./stub/ENetUC_Settings_Manager.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";
import { AsnInvokeProblem } from "./stub/TSROSEBase.js";
import {
	assertAsnInvokeProblem,
	CustomInvokeProblemEnum,
	ROSE_TE_TRANSPORTFAILED,
} from "./support/rose_result_codes.js";
import { SampleRuntimeHarness, TransportActionKind } from "./support/sample_runtime_harness.js";

async function sendFailureIsReportedAsTransportFailure(encoding: EASN1TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);
	harness.clientTransport.enqueueAction({ kind: TransportActionKind.Fail, result: ROSE_TE_TRANSPORTFAILED });

	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding, invokeTimeoutMs: 250 },
	);
	assert.ok(result instanceof AsnInvokeProblem);
	assertAsnInvokeProblem(result, ROSE_TE_TRANSPORTFAILED);
}

async function droppedResponseTimesOut(encoding: EASN1TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);
	harness.clientTransport.dropNextResponse = true;

	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding, invokeTimeoutMs: 25 },
	);
	assert.ok(result instanceof AsnInvokeProblem);
	assert.equal(result.value, CustomInvokeProblemEnum.requestTimedOut);
}

async function delayedResponseCanArriveAfterTimeoutWithoutBreakingNextCall(
	encoding: EASN1TransportEncoding,
): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);
	harness.clientTransport.dropNextResponse = true;

	const first = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding, invokeTimeoutMs: 25 },
	);
	assert.ok(first instanceof AsnInvokeProblem);
	assert.equal(first.value, CustomInvokeProblemEnum.requestTimedOut);

	const second = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding, invokeTimeoutMs: 250 },
	);
	assert.ok(second instanceof ENetUC_Settings_Manager.AsnGetSettingsResult);
	assert.equal(second.settings.u8sUsername, "initial-user");
}

async function malformedInboundPayloadDoesNotDispatchHandlers(encoding: EASN1TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);
	await harness.serverTransport.receiveRaw(harness.malformedPayload(encoding), encoding);

	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding, invokeTimeoutMs: 250 },
	);
	assert.ok(result instanceof ENetUC_Settings_Manager.AsnGetSettingsResult);
	assert.equal(result.settings.u8sUsername, "initial-user");
	assert.equal(harness.getSettingsEventCount(), 0);
	assert.equal(harness.getFancyEventCount(), 0);
}

test("SendFailureIsReportedAsTransportFailure (BER)", () =>
	sendFailureIsReportedAsTransportFailure(EASN1TransportEncoding.BER));
test("SendFailureIsReportedAsTransportFailure (JSON)", () =>
	sendFailureIsReportedAsTransportFailure(EASN1TransportEncoding.JSON));
test("DroppedResponseTimesOut (BER)", () => droppedResponseTimesOut(EASN1TransportEncoding.BER));
test("DroppedResponseTimesOut (JSON)", () => droppedResponseTimesOut(EASN1TransportEncoding.JSON));
test("DelayedResponseCanArriveAfterTimeoutWithoutBreakingNextCall (BER)", () =>
	delayedResponseCanArriveAfterTimeoutWithoutBreakingNextCall(EASN1TransportEncoding.BER));
test("DelayedResponseCanArriveAfterTimeoutWithoutBreakingNextCall (JSON)", () =>
	delayedResponseCanArriveAfterTimeoutWithoutBreakingNextCall(EASN1TransportEncoding.JSON));
test("MalformedInboundPayloadDoesNotDispatchHandlers (BER)", () =>
	malformedInboundPayloadDoesNotDispatchHandlers(EASN1TransportEncoding.BER));
test("MalformedInboundPayloadDoesNotDispatchHandlers (JSON)", () =>
	malformedInboundPayloadDoesNotDispatchHandlers(EASN1TransportEncoding.JSON));
