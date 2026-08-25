// Run: npx tsx compiler/back-ends/ts-gen/tests/TSASN1Base.pauseRoseProcessing.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import {
	ASN1ClassInstanceType,
	PendingInvoke,
	TSASN1Base,
} from "./workdir/TSASN1Base.js";
import { EASN1TransportEncoding } from "./workdir/TSInvokeContext.js";
import {
	ReceiveInvokeContext,
	ROSE_TE_SHUTDOWN,
	type IASN1InvokeData,
} from "./workdir/TSROSEBase.js";
import { ROSEInvoke, type ROSEError, type ROSEReject, type ROSEResult } from "./workdir/SNACCROSE.js";

class TestTransport extends TSASN1Base {
	public constructor() {
		super(EASN1TransportEncoding.JSON, ASN1ClassInstanceType.TSASN1NodeClient);
	}

	public async sendInvoke(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		const shutdownReject = this.completeIfProcessingShutdown(data.invoke);
		if (shutdownReject)
			return shutdownReject;
		return undefined;
	}

	public sendEventSync(_data: IASN1InvokeData): boolean {
		return true;
	}

	public getSessionID(): string | undefined {
		return undefined;
	}
}

class PendingTestTransport extends TestTransport {
	public async sendInvoke(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		const shutdownReject = this.completeIfProcessingShutdown(data.invoke);
		if (shutdownReject)
			return shutdownReject;

		return new Promise((resolve) => {
			if (data.invoke.invokeID !== 99999)
				this.pendingInvokes.set(data.invoke.invokeID, new PendingInvoke(data.invoke, resolve));
			else
				resolve(undefined);
		});
	}
}

function createInvoke(operationID: number, operationName: string, invokeID = 1): ROSEInvoke {
	return {
		invokeID,
		operationID,
		operationName,
	} as ROSEInvoke;
}

test("pauseRoseProcessing blocks outbound invoke and event sends", async () => {
	const transport = new TestTransport();
	transport.pauseRoseProcessing();

	const invokeReject = await transport.sendInvoke({
		invoke: createInvoke(100, "asnInvoke"),
		payLoad: {},
		invokeContext: transport.getInvokeContextParams(undefined, 100, "asnInvoke", false),
	});
	assert.equal(invokeReject?.reject?.invokeProblem, ROSE_TE_SHUTDOWN);

	const eventResult = transport.sendEvent({
		invoke: createInvoke(200, "asnEvent", 99999),
		payLoad: {},
		invokeContext: transport.getInvokeContextParams(undefined, 200, "asnEvent", true),
	});
	assert.equal(eventResult, undefined);

	transport.resumeRoseProcessing();
	assert.equal(transport.isProcessingAllowed(), true);
});

test("pauseRoseProcessing completes pending invokes with shutdown", async () => {
	const transport = new PendingTestTransport();
	const pending = transport.sendInvoke({
		invoke: createInvoke(100, "asnInvoke", 7),
		payLoad: {},
		invokeContext: transport.getInvokeContextParams(undefined, 100, "asnInvoke", false),
	});

	transport.pauseRoseProcessing();
	const result = await pending;
	assert.equal(result?.reject?.invokeProblem, ROSE_TE_SHUTDOWN);
});

test("receiveHandleROSEMessage rejects inbound invoke while paused", async () => {
	const transport = new TestTransport();
	transport.pauseRoseProcessing();

	const response = await transport.receiveHandleROSEMessage(
		{ invoke: createInvoke(100, "asnInvoke", 3) },
		{},
		new ReceiveInvokeContext({ encoding: EASN1TransportEncoding.JSON }),
	);

	assert.ok(response);
	assert.match(String(response?.payLoad), /"invokeProblem":2/);
});
