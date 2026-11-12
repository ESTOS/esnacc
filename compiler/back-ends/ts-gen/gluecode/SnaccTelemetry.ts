// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do NOT edit or modify this code as it is machine generated
// and will be overwritten with every code generation of the esnacc.exe

// dprint-ignore-file
/* eslint-disable */

import type { IInvokeContextBase } from "./TSROSEBase.js";

/**
 * One ROSE lifecycle record. Same fields and enum values as C++ `SnaccTelemetryData`.
 *
 * The runtime creates it when a send or a receive starts and a telemetry callback is
 * installed, and finalizes it once, when the local lifecycle is done. `onInvokeProcessed`
 * then receives the record. When no callback is installed, the runtime does not create the
 * record and does not clone the invoke context. Duration is the local time inside this
 * runtime. A product reads the record and does not mutate it.
 */
export class SnaccTelemetryData {
	readonly direction: SnaccTelemetryData.Direction;
	readonly operationID: number;
	readonly operationName: string;
	readonly requestData: number;
	readonly createdAt: number;

	durationMs = 0;
	responseData?: number;
	context?: IInvokeContextBase;
	outcome: SnaccTelemetryData.Outcome = SnaccTelemetryData.Outcome.UNHANDLED;
	stage: SnaccTelemetryData.Stage = SnaccTelemetryData.Stage.UNKNOWN;
	reason: SnaccTelemetryData.Reason = SnaccTelemetryData.Reason.NONE;
	roseResult?: number;

	private finalized = false;

	private constructor(
		direction: SnaccTelemetryData.Direction,
		operationID: number,
		operationName: string | undefined,
		requestData: number,
		createdAt: number,
	) {
		this.direction = direction;
		this.operationID = operationID;
		this.operationName = operationName ?? "";
		this.requestData = requestData;
		this.createdAt = createdAt;
	}

	/**
	 * Starts a record that the caller finalizes later.
	 * Called from the outbound wait path, which learns the outcome only after the answer arrives.
	 */
	public static create(
		direction: SnaccTelemetryData.Direction,
		operationID: number,
		operationName: string | undefined,
		requestData: number,
		createdAt = performance.now(),
	): SnaccTelemetryData {
		return new SnaccTelemetryData(direction, operationID, operationName, requestData, createdAt);
	}

	/**
	 * Starts a record and finalizes it in the same call.
	 * Called when the outcome is already known, such as a decode failure or an inbound response.
	 */
	public static createFinalized(
		direction: SnaccTelemetryData.Direction,
		operationID: number,
		operationName: string | undefined,
		requestData: number,
		outcome: SnaccTelemetryData.Outcome,
		stage: SnaccTelemetryData.Stage,
		reason: SnaccTelemetryData.Reason,
		roseResult: number,
		responseData?: number,
		context?: IInvokeContextBase,
		createdAt = performance.now(),
	): SnaccTelemetryData {
		const telemetry = SnaccTelemetryData.create(direction, operationID, operationName, requestData, createdAt);
		telemetry.finalize(outcome, stage, reason, roseResult, responseData, context);
		return telemetry;
	}

	/**
	 * Fills the outcome once. A second call throws.
	 * When `context` is set, the stored context is a clone whose `prepareForTelemetry` already ran.
	 * The caller's live context is not modified.
	 */
	public finalize(
		outcome: SnaccTelemetryData.Outcome,
		stage: SnaccTelemetryData.Stage,
		reason: SnaccTelemetryData.Reason,
		roseResult?: number,
		responseData?: number,
		context?: IInvokeContextBase,
	): void {
		if (this.finalized)
			throw new Error("SnaccTelemetryData.finalize may only be called once");
		this.durationMs = Math.max(0, performance.now() - this.createdAt);
		this.outcome = outcome;
		this.stage = stage;
		this.reason = reason;
		this.roseResult = roseResult;
		this.responseData = responseData;
		this.context = context?.cloneForTelemetryRetention();
		this.finalized = true;
	}
}

export namespace SnaccTelemetryData {
	export enum Direction {
		INBOUND = 0,
		OUTBOUND = 1,
	}

	export enum Outcome {
		RESULT = 0,
		ERR = 1,
		REJECT = 2,
		EVENT = 3,
		UNHANDLED = 4,
		DISPATCHED = 5,
	}

	export enum Stage {
		UNKNOWN = 0,
		INBOUND_DECODE = 1,
		INBOUND_INVOKE = 2,
		INBOUND_RESPONSE = 3,
		OUTBOUND_SEND = 4,
		OUTBOUND_WAIT = 5,
	}

	export enum Reason {
		NONE = 0,
		LOCAL_RESULT = 1,
		LOCAL_ERROR = 2,
		LOCAL_REJECT = 3,
		LOCAL_EVENT = 4,
		REMOTE_RESULT = 5,
		REMOTE_ERROR = 6,
		REMOTE_REJECT = 7,
		DECODE_FAILED = 8,
		ENCODE_FAILED = 9,
		SEND_FAILED = 10,
		TIMEOUT = 11,
		SHUTDOWN = 12,
		INVALID_RESPONSE = 13,
		WAIT_SKIPPED = 14,
		INVOKE_EXCEPTION = 15,
		REJECT_PROTOCOL = 16,
		REJECT_SESSION = 17,
		REJECT_AUTHENTICATION = 18,
		UNKNOWN_FAILURE = 19,
		OUTBOUND_BLOCKED = 20,
	}
}

/**
 * Receives one finalized record per ROSE lifecycle.
 * The transport calls `onInvokeProcessed` after finalize. The callback must not mutate the record.
 */
export interface SnaccTelemetryCallback {
	onInvokeProcessed(data: SnaccTelemetryData): void;
}

/**
 * Byte length of a transport payload. Used as the telemetry request or response size.
 */
export function payloadByteLength(payload: unknown): number {
	if (payload instanceof Uint8Array)
		return payload.byteLength;
	if (typeof payload === "string")
		return payload.length;
	const withBer = payload as { toBER?: () => Uint8Array } | undefined;
	if (withBer && typeof withBer.toBER === "function")
		return withBer.toBER().byteLength;
	try {
		return JSON.stringify(payload).length;
	} catch {
		return 0;
	}
}
