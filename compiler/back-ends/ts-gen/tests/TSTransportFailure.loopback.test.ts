// Run: npx tsx compiler/back-ends/ts-gen/tests/TSTransportFailure.loopback.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import * as ENetUC_Settings_Manager from "./workdir/ENetUC_Settings_Manager.js";
import { AsnInvokeProblem, handleRoseReject } from "./workdir/TSROSEBase.js";
import { ConverterErrors } from "./workdir/TSConverterBase.js";
import { EASN1TransportEncoding } from "./workdir/TSInvokeContext.js";
import {
	assertAsnInvokeProblem,
	CustomInvokeProblemEnum,
	ROSE_TE_TRANSPORTFAILED,
} from "./support/rose_result_codes.js";
import { SampleRuntimeHarness, TransportActionKind } from "./support/sample_runtime_harness.js";

const encodings = [
	{ label: "BER", encoding: EASN1TransportEncoding.BER },
	{ label: "JSON", encoding: EASN1TransportEncoding.JSON },
] as const;

for (const { label, encoding } of encodings) {
	test(`SendFailureIsReportedAsTransportFailure (${label})`, async () => {
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
	});

	test(`DroppedResponseTimesOut (${label})`, async () => {
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
	});

	test(`DelayedResponseCanArriveAfterTimeoutWithoutBreakingNextCall (${label})`, async () => {
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
	});

	test(`MalformedInboundPayloadDoesNotDispatchHandlers (${label})`, async () => {
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
	});
}
