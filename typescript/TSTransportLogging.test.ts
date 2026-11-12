// Run: node scripts/run-ts-glue-test-suite.mjs typescript/TSTransportLogging.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import * as ENetUC_Event_Manager from "./stub/ENetUC_Event_Manager.js";
import * as ENetUC_Settings_Manager from "./stub/ENetUC_Settings_Manager.js";
import { EAsnLogLevel } from "./stub/TSASN1Base.js";
import type { TSASN1Base } from "./stub/TSASN1Base.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";
import type { IROSELogger } from "./stub/TSROSEBase.js";
import { SampleRuntimeHarness } from "./support/sample_runtime_harness.js";

interface CapturedLog {
	outbound: boolean;
	error: boolean;
	payload: string;
	operationName?: string;
}

/**
 * Records transport lines that carry a direction and a payload.
 * Other stub logs are ignored so the assertions see the same rows as C++ CapturedLogEntry.
 */
function capturingLogger(entries: CapturedLog[], error: boolean): IROSELogger[keyof IROSELogger] {
	return (_message, _method, _logData, meta) => {
		const record = meta as { payLoad?: unknown; direction?: string; operationName?: string } | undefined;
		if (!record || record.payLoad === undefined || (record.direction !== "in" && record.direction !== "out"))
			return;
		const payload = typeof record.payLoad === "string" ? record.payLoad : JSON.stringify(record.payLoad);
		entries.push({
			outbound: record.direction === "out",
			error,
			payload,
			operationName: record.operationName,
		});
	};
}

/**
 * Installs one sink and the same bitmask on both ends.
 * logRawTransport stays off. The bitmask is the enablement, matching the C++ test level.
 */
function enableLogging(harness: SampleRuntimeHarness, level: EAsnLogLevel): { client: CapturedLog[]; server: CapturedLog[] } {
	const client: CapturedLog[] = [];
	const server: CapturedLog[] = [];
	const attach = (transport: TSASN1Base, entries: CapturedLog[]): void => {
		const debug = capturingLogger(entries, false);
		const fail = capturingLogger(entries, true);
		transport.setLogger({ error: fail, warn: debug, info: debug, debug });
		transport.setTransportLogLevel(level);
	};
	attach(harness.clientTransport, client);
	attach(harness.serverTransport, server);
	return { client, server };
}

function hasPretty(entries: CapturedLog[], outbound: boolean, first: string, second?: string, operationName?: string): boolean {
	return entries.some((entry) => {
		if (entry.outbound !== outbound || entry.error)
			return false;
		if (operationName && entry.operationName !== operationName)
			return false;
		if (entry.payload.includes("\"BER\""))
			return false;
		if (!entry.payload.includes("\n"))
			return false;
		if (!entry.payload.includes(first))
			return false;
		return !second || entry.payload.includes(second);
	});
}

function hasWrappedBer(entries: CapturedLog[], outbound: boolean): boolean {
	return entries.some((entry) => entry.outbound === outbound && !entry.error && entry.payload.includes("\"BER\""));
}

function hasRawBer(entries: CapturedLog[], outbound: boolean, prefix: string): boolean {
	return entries.some((entry) => {
		if (entry.outbound !== outbound || entry.error)
			return false;
		if (entry.payload.includes("{"))
			return false;
		return entry.payload.startsWith(prefix);
	});
}

async function berInvokeAndResult(): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.BER);
	const logs = enableLogging(harness, EAsnLogLevel.JSON_AND_BER);
	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.BER },
	);
	assert.ok(result instanceof ENetUC_Settings_Manager.AsnGetSettingsResult);
	assert.equal(hasPretty(logs.client, true, "\"invoke\"", undefined, "asnGetSettings"), true);
	assert.equal(hasPretty(logs.server, false, "\"invoke\"", undefined, "asnGetSettings"), true);
	assert.equal(hasPretty(logs.server, true, "\"result\"", "initial-user"), true);
	assert.equal(hasPretty(logs.client, false, "\"result\"", "initial-user"), true);
	assert.equal(hasWrappedBer(logs.client, true), true);
	assert.equal(hasWrappedBer(logs.server, false), true);
	assert.equal(hasWrappedBer(logs.server, true), true);
	assert.equal(hasWrappedBer(logs.client, false), true);
}

async function berError(): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.BER);
	harness.configureServerHandlers({ getSettingsReturnsError: true });
	const logs = enableLogging(harness, EAsnLogLevel.JSON_AND_BER);
	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.BER },
	);
	assert.ok(!(result instanceof ENetUC_Settings_Manager.AsnGetSettingsResult));
	assert.equal(hasPretty(logs.server, true, "\"error\"", "get settings handler returned error"), true);
	assert.equal(hasPretty(logs.client, false, "\"error\"", "get settings handler returned error"), true);
	assert.equal(hasWrappedBer(logs.server, true), true);
	assert.equal(hasWrappedBer(logs.client, false), true);
}

async function berReject(): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.BER);
	harness.configureServerHandlers({ implementGetSettings: false });
	const logs = enableLogging(harness, EAsnLogLevel.JSON_AND_BER);
	await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.BER },
	);
	assert.equal(hasPretty(logs.server, true, "\"reject\"", "is not implemented"), true);
	assert.equal(hasPretty(logs.client, false, "\"reject\"", "is not implemented"), true);
	assert.equal(hasWrappedBer(logs.server, true), true);
	assert.equal(hasWrappedBer(logs.client, false), true);
}

async function berEvent(): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.BER);
	const logs = enableLogging(harness, EAsnLogLevel.JSON_AND_BER);
	const result = await harness.clientEventRose.invoke_asnCreateFancyEvents(
		new ENetUC_Event_Manager.AsnCreateFancyEventsArgument({ iEventDelay: 0, iEventCount: 2 }),
		{ encoding: EASN1TransportEncoding.BER },
	);
	assert.ok(result instanceof ENetUC_Event_Manager.AsnCreateFancyEventsResult);
	assert.equal(hasPretty(logs.server, true, "\"invoke\"", "iEventCounter"), true);
	assert.equal(hasPretty(logs.client, false, "\"invoke\"", "iEventCounter"), true);
	assert.equal(hasWrappedBer(logs.server, true), true);
	assert.equal(hasWrappedBer(logs.client, false), true);
}

async function jsonRejectWithoutHex(): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.JSON);
	harness.configureServerHandlers({ implementGetSettings: false });
	const logs = enableLogging(harness, EAsnLogLevel.JSON);
	await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.JSON },
	);
	assert.equal(hasPretty(logs.server, true, "\"reject\"", "is not implemented"), true);
	assert.equal(hasPretty(logs.client, false, "\"reject\"", "is not implemented"), true);
	assert.equal(hasWrappedBer(logs.server, true), false);
	assert.equal(hasWrappedBer(logs.client, false), false);
}

async function jsonWithBothBitsSkipsHex(): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.JSON);
	const logs = enableLogging(harness, EAsnLogLevel.JSON_AND_BER);
	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.JSON },
	);
	assert.ok(result instanceof ENetUC_Settings_Manager.AsnGetSettingsResult);
	assert.equal(hasPretty(logs.client, true, "\"invoke\"", undefined, "asnGetSettings"), true);
	assert.equal(hasPretty(logs.server, false, "\"invoke\"", undefined, "asnGetSettings"), true);
	assert.equal(hasPretty(logs.server, true, "\"result\"", "initial-user"), true);
	assert.equal(hasPretty(logs.client, false, "\"result\"", "initial-user"), true);
	assert.equal(hasWrappedBer(logs.client, true), false);
	assert.equal(hasWrappedBer(logs.server, false), false);
	assert.equal(hasWrappedBer(logs.server, true), false);
	assert.equal(hasWrappedBer(logs.client, false), false);
}

async function berOnlyRejectIsHex(): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.BER);
	harness.configureServerHandlers({ implementGetSettings: false });
	const logs = enableLogging(harness, EAsnLogLevel.BER);
	await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.BER },
	);
	assert.equal(hasRawBer(logs.server, true, "A4"), true);
	assert.equal(hasRawBer(logs.client, false, "A4"), true);
	assert.equal(hasPretty(logs.server, true, "\"reject\"", "is not implemented"), false);
	assert.equal(hasPretty(logs.client, false, "\"reject\"", "is not implemented"), false);
}

async function prettyLevelAddsNewlines(): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.JSON);
	const pretty = EAsnLogLevel.JSON | EAsnLogLevel.JSON_ALWAYS_PRETTY_PRINTED;
	const logs = enableLogging(harness, pretty);
	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.JSON },
	);
	assert.ok(result instanceof ENetUC_Settings_Manager.AsnGetSettingsResult);
	assert.equal(hasPretty(logs.client, true, "\"invoke\"", undefined, "asnGetSettings"), true);
	assert.equal(hasPretty(logs.server, false, "\"invoke\"", undefined, "asnGetSettings"), true);
	assert.equal(hasPretty(logs.server, true, "\"result\"", "initial-user"), true);
	assert.equal(hasPretty(logs.client, false, "\"result\"", "initial-user"), true);
}

async function malformed(encoding: EASN1TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);
	const logs = enableLogging(harness, encoding === EASN1TransportEncoding.BER ? EAsnLogLevel.JSON_AND_BER : EAsnLogLevel.JSON);
	const raw = encoding === EASN1TransportEncoding.BER
		? new Uint8Array([0xa1, 0x01])
		: new TextEncoder().encode("J0000007{\"bad\":");
	await harness.serverTransport.receiveRaw(raw, encoding);
	if (encoding === EASN1TransportEncoding.BER)
		assert.equal(hasWrappedBer(logs.server, false), true);
	else
		assert.equal(logs.server.some((entry) => !entry.outbound && !entry.error && entry.payload.includes("{\"bad\":")), true);
	assert.equal(logs.server.some((entry) => !entry.outbound && entry.error && entry.payload.includes("exception")), true);
}

test("BerInvokeAndResultLoggedAsPrettyJsonAndHex (BER)", () => berInvokeAndResult());
test("BerErrorLoggedAsPrettyJsonAndHex (BER)", () => berError());
test("BerRejectLoggedAsPrettyJsonAndHex (BER)", () => berReject());
test("BerEventLoggedAsPrettyJsonAndHex (BER)", () => berEvent());
test("JsonRejectLoggedAsPrettyJsonWithoutHex (JSON)", () => jsonRejectWithoutHex());
test("JsonTransportWithBothBitsLogsJsonWithoutHex (JSON)", () => jsonWithBothBitsSkipsHex());
test("BerOnlyRejectLoggedAsHexWithoutDecodedJson (BER)", () => berOnlyRejectIsHex());
test("JsonPrettyLevelLogsNewlines (JSON)", () => prettyLevelAddsNewlines());
test("MalformedInboundPayloadLogsRawDataAndError (JSON)", () => malformed(EASN1TransportEncoding.JSON));
test("MalformedInboundPayloadLogsRawDataAndError (BER)", () => malformed(EASN1TransportEncoding.BER));
