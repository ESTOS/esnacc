// Run: npx tsx typescript/TSInvokeContext.receive.test.ts
// Parity: cpp-lib/tests/invoke_context/invoke_context_tests.cpp
//   OutboundWireInvokeTimeoutReachesInboundHandlerJson / Ber
import assert from "node:assert/strict";
import test from "node:test";
import * as ENetUC_Settings_Manager from "./stub/ENetUC_Settings_Manager.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";
import { SampleRuntimeHarness } from "./support/sample_runtime_harness.js";

test("OutboundWireInvokeTimeoutReachesInboundHandler (JSON)", async () => {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.JSON);

	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.JSON, invokeTimeoutMs: 500 },
	);
	assert.ok(result instanceof ENetUC_Settings_Manager.AsnGetSettingsResult);
	assert.equal(harness.wasInboundHandlerInvoked(), true);
	assert.equal(harness.getLastInboundTimeout(), 500);
	assert.equal(harness.getLastInboundOperationName(), "asnGetSettings");
});

test("OutboundWireInvokeTimeoutReachesInboundHandler (BER)", async () => {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.BER);
	harness.clientTransport.setSendTimeoutInInvokes(true);

	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.BER, invokeTimeoutMs: 500 },
	);
	assert.ok(result instanceof ENetUC_Settings_Manager.AsnGetSettingsResult);
	assert.equal(harness.wasInboundHandlerInvoked(), true);
	assert.equal(harness.getLastInboundTimeout(), 500);
});

test("OutboundFireAndForgetTimeoutReachesInboundHandler (JSON)", async () => {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.JSON);

	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.JSON, invokeTimeoutMs: 0 },
	);
	assert.equal(result, undefined);
	assert.equal(harness.wasInboundHandlerInvoked(), true);
	assert.equal(harness.getLastInboundTimeout(), 0);
});

test("OutboundWireTimeoutAbsentLeavesInboundUnset (BER)", async () => {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.BER);

	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.BER, invokeTimeoutMs: 500 },
	);
	assert.ok(result instanceof ENetUC_Settings_Manager.AsnGetSettingsResult);
	assert.equal(harness.wasInboundHandlerInvoked(), true);
	assert.equal(harness.getLastInboundTimeout(), undefined);
});
