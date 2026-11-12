// Run: npx tsx typescript/TSTelemetry.test.ts
// Parity: cpp-lib/tests/telemetry/telemetry_tests.cpp
// Asserts outcome and reason. It does not assert C++ ownership.
import assert from "node:assert/strict";
import test from "node:test";
import * as ENetUC_Common from "./stub/ENetUC_Common.js";
import * as ENetUC_Event_Manager from "./stub/ENetUC_Event_Manager.js";
import { ENetUC_Settings_ManagerROSE } from "./stub/ENetUC_Settings_ManagerROSE.js";
import * as ENetUC_Settings_Manager from "./stub/ENetUC_Settings_Manager.js";
import { ROSEResult, ROSEResultSeq } from "./stub/SNACCROSE.js";
import { SnaccTelemetryData } from "./stub/SnaccTelemetry.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";
import { AsnInvokeProblem, ERoseResult, SendInvokeContext } from "./stub/TSROSEBase.js";
import type { IASN1InvokeData } from "./stub/TSROSEBase.js";
import { CapturingClientTransport } from "./support/rose_test_transport.js";
import { SampleRuntimeHarness, type TransportEncoding } from "./support/sample_runtime_harness.js";

function hasTelemetry(
	entries: SnaccTelemetryData[],
	direction: SnaccTelemetryData.Direction,
	outcome: SnaccTelemetryData.Outcome,
	stage: SnaccTelemetryData.Stage,
	reason: SnaccTelemetryData.Reason,
	roseResult?: number,
): boolean {
	return entries.some((entry) => {
		if (entry.direction !== direction || entry.outcome !== outcome || entry.stage !== stage || entry.reason !== reason)
			return false;
		if (roseResult === undefined)
			return true;
		return entry.roseResult === roseResult;
	});
}

function collect(harness: SampleRuntimeHarness): SnaccTelemetryData[] {
	const entries: SnaccTelemetryData[] = [];
	const sink = {
		onInvokeProcessed(data: SnaccTelemetryData): void {
			entries.push(data);
		},
	};
	harness.clientTransport.setTelemetryCallback(sink);
	harness.serverTransport.setTelemetryCallback(sink);
	return entries;
}

async function happyPath(encoding: TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);
	const entries = collect(harness);

	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding },
	);
	assert.ok(result instanceof ENetUC_Settings_Manager.AsnGetSettingsResult);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.OUTBOUND, SnaccTelemetryData.Outcome.RESULT, SnaccTelemetryData.Stage.OUTBOUND_WAIT, SnaccTelemetryData.Reason.REMOTE_RESULT, ERoseResult.ROSE_NOERROR), true);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.INBOUND, SnaccTelemetryData.Outcome.RESULT, SnaccTelemetryData.Stage.INBOUND_INVOKE, SnaccTelemetryData.Reason.LOCAL_RESULT, ERoseResult.ROSE_NOERROR), true);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.INBOUND, SnaccTelemetryData.Outcome.RESULT, SnaccTelemetryData.Stage.INBOUND_RESPONSE, SnaccTelemetryData.Reason.REMOTE_RESULT, ERoseResult.ROSE_NOERROR), true);
}

async function applicationError(encoding: TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);
	harness.configureServerHandlers({ getSettingsReturnsError: true });
	const entries = collect(harness);

	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding },
	);
	assert.ok(result instanceof ENetUC_Common.AsnRequestError);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.OUTBOUND, SnaccTelemetryData.Outcome.ERR, SnaccTelemetryData.Stage.OUTBOUND_WAIT, SnaccTelemetryData.Reason.REMOTE_ERROR, ERoseResult.ROSE_ERROR_VALUE), true);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.INBOUND, SnaccTelemetryData.Outcome.ERR, SnaccTelemetryData.Stage.INBOUND_INVOKE, SnaccTelemetryData.Reason.LOCAL_ERROR, ERoseResult.ROSE_NOERROR), true);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.INBOUND, SnaccTelemetryData.Outcome.ERR, SnaccTelemetryData.Stage.INBOUND_RESPONSE, SnaccTelemetryData.Reason.REMOTE_ERROR, ERoseResult.ROSE_ERROR_VALUE), true);
}

async function reject(encoding: TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);
	harness.configureServerHandlers({ implementGetSettings: false });
	const entries = collect(harness);

	await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding },
	);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.OUTBOUND, SnaccTelemetryData.Outcome.REJECT, SnaccTelemetryData.Stage.OUTBOUND_WAIT, SnaccTelemetryData.Reason.REMOTE_REJECT), true);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.INBOUND, SnaccTelemetryData.Outcome.REJECT, SnaccTelemetryData.Stage.INBOUND_RESPONSE, SnaccTelemetryData.Reason.REMOTE_REJECT), true);
}

async function decodeFailure(encoding: TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);
	const entries = collect(harness);
	await harness.serverTransport.receiveRaw(harness.malformedPayload(encoding), encoding);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.INBOUND, SnaccTelemetryData.Outcome.UNHANDLED, SnaccTelemetryData.Stage.INBOUND_DECODE, SnaccTelemetryData.Reason.DECODE_FAILED), true);
}

async function timeout(encoding: TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);
	harness.clientTransport.dropNextResponse = true;
	const entries = collect(harness);

	await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding, invokeTimeoutMs: 40 },
	);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.OUTBOUND, SnaccTelemetryData.Outcome.UNHANDLED, SnaccTelemetryData.Stage.OUTBOUND_WAIT, SnaccTelemetryData.Reason.TIMEOUT, ERoseResult.ROSE_TE_TIMEOUT), true);
}

async function waitSkipped(encoding: TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);
	const entries = collect(harness);

	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding, invokeTimeoutMs: 0 },
	);
	assert.equal(result, undefined);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.OUTBOUND, SnaccTelemetryData.Outcome.DISPATCHED, SnaccTelemetryData.Stage.OUTBOUND_WAIT, SnaccTelemetryData.Reason.WAIT_SKIPPED, ERoseResult.ROSE_NOERROR), true);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.OUTBOUND, SnaccTelemetryData.Outcome.UNHANDLED, SnaccTelemetryData.Stage.OUTBOUND_WAIT, SnaccTelemetryData.Reason.UNKNOWN_FAILURE), false);
}

async function events(encoding: TransportEncoding): Promise<void> {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(encoding);
	const entries = collect(harness);

	const result = await harness.clientEventRose.invoke_asnCreateFancyEvents(
		new ENetUC_Event_Manager.AsnCreateFancyEventsArgument({ iEventDelay: 0, iEventCount: 2 }),
		{ encoding },
	);
	assert.ok(result instanceof ENetUC_Event_Manager.AsnCreateFancyEventsResult);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.OUTBOUND, SnaccTelemetryData.Outcome.EVENT, SnaccTelemetryData.Stage.OUTBOUND_SEND, SnaccTelemetryData.Reason.LOCAL_EVENT, ERoseResult.ROSE_NOERROR), true);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.INBOUND, SnaccTelemetryData.Outcome.EVENT, SnaccTelemetryData.Stage.INBOUND_INVOKE, SnaccTelemetryData.Reason.LOCAL_EVENT, ERoseResult.ROSE_NOERROR), true);
}

class NotedSendContext extends SendInvokeContext {
	public note = "custom";
	public prepared = false;

	/** Marks the telemetry clone. The live context must stay unmarked. */
	public override prepareForTelemetry(): void {
		this.prepared = true;
		this.note = `prepared:${this.note}`;
	}
}

class BadResultTransport extends CapturingClientTransport {
	public readonly entries: SnaccTelemetryData[] = [];

	public constructor() {
		super();
		this.setTelemetryCallback({
			onInvokeProcessed: (data: SnaccTelemetryData): void => {
				this.entries.push(data);
			},
		});
	}

	/** Returns a result envelope whose payload is not the expected settings type. */
	public override async sendInvoke(data: IASN1InvokeData): Promise<ROSEResult> {
		this.lastInvokeData = data;
		return new ROSEResult({
			invokeID: data.invoke.invokeID,
			result: new ROSEResultSeq({ resultValue: 0, result: true }),
		});
	}
}

test("HappyPathTelemetry (JSON)", async () => {
	await happyPath(EASN1TransportEncoding.JSON);
});

test("HappyPathTelemetry (BER)", async () => {
	await happyPath(EASN1TransportEncoding.BER);
});

test("ApplicationErrorTelemetry (JSON)", async () => {
	await applicationError(EASN1TransportEncoding.JSON);
});

test("ApplicationErrorTelemetry (BER)", async () => {
	await applicationError(EASN1TransportEncoding.BER);
});

test("RejectTelemetry (JSON)", async () => {
	await reject(EASN1TransportEncoding.JSON);
});

test("RejectTelemetry (BER)", async () => {
	await reject(EASN1TransportEncoding.BER);
});

test("DecodeFailureTelemetry (JSON)", async () => {
	await decodeFailure(EASN1TransportEncoding.JSON);
});

test("DecodeFailureTelemetry (BER)", async () => {
	await decodeFailure(EASN1TransportEncoding.BER);
});

test("TimeoutTelemetry (JSON)", async () => {
	await timeout(EASN1TransportEncoding.JSON);
});

test("TimeoutTelemetry (BER)", async () => {
	await timeout(EASN1TransportEncoding.BER);
});

test("WaitSkippedTelemetry (JSON)", async () => {
	await waitSkipped(EASN1TransportEncoding.JSON);
});

test("WaitSkippedTelemetry (BER)", async () => {
	await waitSkipped(EASN1TransportEncoding.BER);
});

test("ResultPayloadDecodeFailureTelemetry (JSON)", async () => {
	const transport = new BadResultTransport();
	const rose = new ENetUC_Settings_ManagerROSE(transport, false);
	const answer = await rose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.JSON, invokeTimeoutMs: 500 },
	);
	assert.ok(answer instanceof AsnInvokeProblem);
	assert.equal(hasTelemetry(transport.entries, SnaccTelemetryData.Direction.OUTBOUND, SnaccTelemetryData.Outcome.UNHANDLED, SnaccTelemetryData.Stage.OUTBOUND_WAIT, SnaccTelemetryData.Reason.DECODE_FAILED, ERoseResult.ROSE_RE_DECODE_FAILED), true);
	assert.equal(hasTelemetry(transport.entries, SnaccTelemetryData.Direction.OUTBOUND, SnaccTelemetryData.Outcome.RESULT, SnaccTelemetryData.Stage.OUTBOUND_WAIT, SnaccTelemetryData.Reason.REMOTE_RESULT), false);
});

test("EventTelemetry (JSON)", async () => {
	await events(EASN1TransportEncoding.JSON);
});

test("EventTelemetry (BER)", async () => {
	await events(EASN1TransportEncoding.BER);
});

test("CustomContextIsClonedAndPrepared (JSON)", async () => {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.JSON);
	const entries = collect(harness);
	const caller = new NotedSendContext({ invokeTimeoutMs: 500, encoding: EASN1TransportEncoding.JSON });

	const result = await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		caller,
	);
	assert.ok(result instanceof ENetUC_Settings_Manager.AsnGetSettingsResult);
	assert.equal(caller.prepared, false);
	assert.equal(caller.note, "custom");

	const telemetry = entries.find((entry) => entry.direction === SnaccTelemetryData.Direction.OUTBOUND && entry.stage === SnaccTelemetryData.Stage.OUTBOUND_WAIT && entry.reason === SnaccTelemetryData.Reason.REMOTE_RESULT);
	assert.ok(telemetry);
	assert.ok(telemetry.context instanceof NotedSendContext);
	assert.equal(telemetry.context.prepared, true);
	assert.equal(telemetry.context.note, "prepared:custom");
});

test("ShutdownPendingInvokeTelemetry (JSON)", async () => {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.JSON);
	const entries = collect(harness);
	harness.clientTransport.pauseRoseProcessing();

	await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.JSON },
	);
	assert.equal(hasTelemetry(entries, SnaccTelemetryData.Direction.OUTBOUND, SnaccTelemetryData.Outcome.UNHANDLED, SnaccTelemetryData.Stage.OUTBOUND_SEND, SnaccTelemetryData.Reason.SHUTDOWN, ERoseResult.ROSE_TE_SHUTDOWN), true);
});
