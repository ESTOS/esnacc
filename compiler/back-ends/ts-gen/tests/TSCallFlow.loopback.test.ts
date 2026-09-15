// Run: npx tsx compiler/back-ends/ts-gen/tests/TSCallFlow.loopback.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import * as ENetUC_Event_Manager from "./workdir/ENetUC_Event_Manager.js";
import * as ENetUC_Settings_Manager from "./workdir/ENetUC_Settings_Manager.js";
import { EASN1TransportEncoding } from "./workdir/TSInvokeContext.js";
import { SampleRuntimeHarness } from "./support/sample_runtime_harness.js";

const encodings = [
	{ label: "JSON", encoding: EASN1TransportEncoding.JSON },
	{ label: "BER", encoding: EASN1TransportEncoding.BER },
] as const;

for (const { label, encoding } of encodings) {
	test(`GetSettingsRoundTripReturnsCurrentServerState (${label})`, async () => {
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
	});

	test(`SetSettingsRoundTripDispatchesClientEvent (${label})`, async () => {
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
	});

	test(`CreateFancyEventsDispatchesServerEventsToClient (${label})`, async () => {
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
	});
}
