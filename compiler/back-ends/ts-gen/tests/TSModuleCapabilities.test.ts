// Run: npx tsx compiler/back-ends/ts-gen/tests/TSModuleCapabilities.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import {
	buildRemoteModuleCapabilities,
	buildRemoteModuleCapabilitiesFromAsn,
} from "../gluecode/TSModuleCapabilities.js";

test("buildRemoteModuleCapabilities populates invoke and event op ids", () => {
	const remote = buildRemoteModuleCapabilities([
		{
			moduleName: "ENetUC_Settings_Manager",
			version: "20240101.0.20240506",
			invokeOpIds: [4100, 4101],
			eventOpIds: [4150],
		},
	]);

	assert.equal(remote.size, 1);
	const module = remote.get("ENetUC_Settings_Manager");
	assert.ok(module);
	assert.equal(module.moduleName, "ENetUC_Settings_Manager");
	assert.equal(module.version, "20240101.0.20240506");
	assert.ok(module.invokes.has(4100));
	assert.ok(module.invokes.has(4101));
	assert.ok(module.events.has(4150));
});

test("buildRemoteModuleCapabilitiesFromAsn maps ASN module detail fields", () => {
	const remote = buildRemoteModuleCapabilitiesFromAsn([
		{
			u8sName: "ENetUC_Settings_Manager",
			u8sASN1ModuleVersion: "20240101.0.20240506",
			iOperations: [4100],
			iEvents: [4150],
		},
	]);

	const module = remote.get("ENetUC_Settings_Manager");
	assert.ok(module);
	assert.ok(module.invokes.has(4100));
	assert.ok(module.events.has(4150));
});
