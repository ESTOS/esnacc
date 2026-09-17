// Run: npx tsx typescript/TSRoseHttpStatus.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import { InvokeProblemenum, ROSEError, ROSEResult } from "./stub/SNACCROSE.js";
import { ASN1ClassInstanceType, TSASN1Base } from "./stub/TSASN1Base.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";
import {
	CustomInvokeProblemEnum,
	createInvokeReject,
	httpStatusFromInvokeProblem,
	httpStatusFromRoseOutcome,
	httpStatusFromRoseReject,
	ReceiveInvokeContext,
	ROSE_HTTP_APPLICATION_ERROR,
	ROSE_HTTP_OK,
	ROSE_HTTP_REJECT_FALLBACK,
} from "./stub/TSROSEBase.js";
import type { IASN1InvokeData, ROSEReject } from "./stub/TSROSEBase.js";
import { SampleRuntimeHarness } from "./support/sample_runtime_harness.js";
import * as ENetUC_Settings_Manager from "./stub/ENetUC_Settings_Manager.js";

class HandleResultProbe extends TSASN1Base {
	public constructor() {
		super(EASN1TransportEncoding.JSON, ASN1ClassInstanceType.TSASN1Server);
	}

	public async sendInvoke(_data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		return undefined;
	}

	public sendEventSync(_data: IASN1InvokeData): boolean {
		return true;
	}

	public getSessionID(): string | undefined {
		return undefined;
	}

	public httpStatusFor(result: ROSEReject | ROSEResult | ROSEError): number | undefined {
		const response = this.handleResult(result, new ReceiveInvokeContext({ encoding: EASN1TransportEncoding.JSON }));
		return response?.httpStatusCode;
	}
}

test("httpStatusFromRoseOutcome maps result, error, and reject arms", () => {
	assert.equal(httpStatusFromRoseOutcome(new ROSEResult({ invokeID: 1 })), ROSE_HTTP_OK);

	const appError = new ROSEError({ invokedID: 1, error_value: 7001, error: new Uint8Array() });
	assert.equal(httpStatusFromRoseOutcome(appError), ROSE_HTTP_APPLICATION_ERROR);

	const reject = createInvokeReject(1, InvokeProblemenum.unrecognisedOperation);
	assert.equal(httpStatusFromRoseOutcome(reject), 501);
});

test("application ROSEError uses 500 even when error_value is a large app code", () => {
	const probe = new HandleResultProbe();
	const status = probe.httpStatusFor(
		new ROSEError({ invokedID: 1, error_value: 7001, error: new Uint8Array([1, 2, 3]) }),
	);
	assert.equal(status, ROSE_HTTP_APPLICATION_ERROR);
});

test("internalError reject maps to 502 not 500", () => {
	assert.equal(httpStatusFromInvokeProblem(CustomInvokeProblemEnum.internalError), ROSE_HTTP_REJECT_FALLBACK);
	assert.equal(
		httpStatusFromRoseReject(
			createInvokeReject(1, CustomInvokeProblemEnum.internalError, "encode failed"),
		),
		ROSE_HTTP_REJECT_FALLBACK,
	);
});

test("invokeProblem table covers common reject cases", () => {
	assert.equal(httpStatusFromInvokeProblem(InvokeProblemenum.mistypedArgument), 400);
	assert.equal(httpStatusFromInvokeProblem(InvokeProblemenum.invalidSessionID), 401);
	assert.equal(httpStatusFromInvokeProblem(InvokeProblemenum.resourceLimitation), 503);
	assert.equal(httpStatusFromInvokeProblem(CustomInvokeProblemEnum.messageTooBig), 413);
	assert.equal(httpStatusFromInvokeProblem(CustomInvokeProblemEnum.requestTimedOut), 504);
	assert.equal(httpStatusFromInvokeProblem(InvokeProblemenum.unexpectedChildOperation), ROSE_HTTP_REJECT_FALLBACK);
});

test("loopback invoke sets HTTP status on server response path", async () => {
	const harness = new SampleRuntimeHarness();
	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.JSON);

	await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.JSON },
	);
	assert.equal(harness.serverTransport.lastResponseHttpStatus, ROSE_HTTP_OK);

	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.JSON);
	harness.configureServerHandlers({ getSettingsReturnsError: true });
	await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.JSON },
	);
	assert.equal(harness.serverTransport.lastResponseHttpStatus, ROSE_HTTP_APPLICATION_ERROR);

	harness.resetState();
	harness.setEncoding(EASN1TransportEncoding.JSON);
	harness.configureServerHandlers({ implementGetSettings: false });
	await harness.clientSettingsRose.invoke_asnGetSettings(
		new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
		{ encoding: EASN1TransportEncoding.JSON },
	);
	assert.equal(harness.serverTransport.lastResponseHttpStatus, 501);
});
