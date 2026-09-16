// Run: npx tsx typescript/TSCallFlow.loopback.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import * as ENetUC_Event_Manager from "./stub/ENetUC_Event_Manager.js";
import * as ENetUC_Settings_Manager from "./stub/ENetUC_Settings_Manager.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";
import { SampleRuntimeHarness } from "./support/sample_runtime_harness.js";

async function getSettingsRoundTripReturnsCurrentServerState(encoding: EASN1TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);

	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding },
	);
	assert.ok(result instanceof ENetUC_Settings_Manager.AsnGetSettingsResult);
	assert.equal(result.settings.bEnabled, false);
	assert.equal(result.settings.u8sUsername, "initial-user");
}

async function setSettingsRoundTripDispatchesClientEvent(encoding: EASN1TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);

	const setResult = await harness.clientSettingsRose.invoke_asnSetSettings(
		new ENetUC_Settings_Manager.AsnSetSettingsArgument({
			settings: { bEnabled: true, u8sUsername: "updated-user" },
		}),
		{ encoding },
	);
	assert.ok(setResult instanceof ENetUC_Settings_Manager.AsnSetSettingsResult);
	assert.equal(harness.getSettingsEventCount(), 1);
	assert.equal(harness.getLastSettingsEventEnabled(), true);
	assert.equal(harness.getLastSettingsEventUsername(), "updated-user");

	const getResult = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding },
	);
	assert.ok(getResult instanceof ENetUC_Settings_Manager.AsnGetSettingsResult);
	assert.equal(getResult.settings.bEnabled, true);
	assert.equal(getResult.settings.u8sUsername, "updated-user");
}

async function createFancyEventsDispatchesServerEventsToClient(encoding: EASN1TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);

	const result = await harness.clientEventRose.invoke_asnCreateFancyEvents(
		new ENetUC_Event_Manager.AsnCreateFancyEventsArgument({ iEventDelay: 0, iEventCount: 3 }),
		{ encoding },
	);
	assert.ok(result instanceof ENetUC_Event_Manager.AsnCreateFancyEventsResult);
	assert.equal(harness.getFancyEventCount(), 3);
	const events = harness.fancyEventsSnapshot();
	assert.deepEqual(events[0], { counter: 1, left: 2 });
	assert.deepEqual(events[1], { counter: 2, left: 1 });
	assert.deepEqual(events[2], { counter: 3, left: 0 });
}

test("GetSettingsRoundTripReturnsCurrentServerState (JSON)", () =>
	getSettingsRoundTripReturnsCurrentServerState(EASN1TransportEncoding.JSON));
test("GetSettingsRoundTripReturnsCurrentServerState (BER)", () =>
	getSettingsRoundTripReturnsCurrentServerState(EASN1TransportEncoding.BER));
test("SetSettingsRoundTripDispatchesClientEvent (JSON)", () =>
	setSettingsRoundTripDispatchesClientEvent(EASN1TransportEncoding.JSON));
test("SetSettingsRoundTripDispatchesClientEvent (BER)", () =>
	setSettingsRoundTripDispatchesClientEvent(EASN1TransportEncoding.BER));
test("CreateFancyEventsDispatchesServerEventsToClient (JSON)", () =>
	createFancyEventsDispatchesServerEventsToClient(EASN1TransportEncoding.JSON));
test("CreateFancyEventsDispatchesServerEventsToClient (BER)", () =>
	createFancyEventsDispatchesServerEventsToClient(EASN1TransportEncoding.BER));
