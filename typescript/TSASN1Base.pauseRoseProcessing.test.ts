// Run: npx tsx typescript/TSASN1Base.pauseRoseProcessing.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import { type ROSEError, ROSEReject, type ROSEResult } from "./stub/SNACCROSE.js";
import { ASN1ClassInstanceType, PendingInvoke, TSASN1Base } from "./stub/TSASN1Base.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";
import { type IASN1InvokeData, ReceiveInvokeContext, ROSE_TE_SHUTDOWN, SendInvokeContext } from "./stub/TSROSEBase.js";

class TestTransport extends TSASN1Base {
	public constructor() {
		super(EASN1TransportEncoding.JSON, ASN1ClassInstanceType.TSASN1NodeClient);
	}

	public async sendInvoke(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		const shutdownReject = this.completeIfProcessingShutdown(data.invoke);
		if (shutdownReject) {
			return shutdownReject;
		}
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
		if (shutdownReject) {
			return shutdownReject;
		}

		return new Promise((resolve) => {
			if (data.invoke.invokeID !== 99999) {
				this.pendingInvokes.set(data.invoke.invokeID, new PendingInvoke(data.invoke, resolve));
			} else {
				resolve(undefined);
			}
		});
	}
}

function createInvokeContext(
	transport: TestTransport,
	operationID: number,
	operationName: string,
	event: boolean,
): SendInvokeContext {
	return new SendInvokeContext(transport.getInvokeContextParams(undefined, operationID, operationName, event));
}

test("pauseRoseProcessing blocks outbound invoke and event sends", async () => {
	const transport = new TestTransport();
	transport.pauseRoseProcessing();

	const invokeReject = await transport.sendInvoke({
		invoke: { invokeID: 1, operationID: 100, operationName: "asnInvoke" },
		payLoad: {},
		invokeContext: createInvokeContext(transport, 100, "asnInvoke", false),
	});
	assert.ok(invokeReject instanceof ROSEReject);
	assert.equal(invokeReject.reject?.invokeProblem, ROSE_TE_SHUTDOWN);

	const eventContext = createInvokeContext(transport, 200, "asnEvent", true);
	eventContext.bSendEventSynchronous = true;
	const eventResult = transport.sendEvent({
		invoke: { invokeID: 99999, operationID: 200, operationName: "asnEvent" },
		payLoad: {},
		invokeContext: eventContext,
	});
	assert.equal(eventResult, false);

	transport.resumeRoseProcessing();
	assert.equal(transport.isProcessingAllowed(), true);
});

test("resumeRoseProcessing allows the next outbound invoke", async () => {
	const transport = new TestTransport();
	transport.pauseRoseProcessing();

	const blocked = await transport.sendInvoke({
		invoke: { invokeID: 1, operationID: 100, operationName: "asnInvoke" },
		payLoad: {},
		invokeContext: createInvokeContext(transport, 100, "asnInvoke", false),
	});
	assert.ok(blocked instanceof ROSEReject);
	assert.equal(blocked.reject?.invokeProblem, ROSE_TE_SHUTDOWN);

	transport.resumeRoseProcessing();
	const sent = await transport.sendInvoke({
		invoke: { invokeID: 2, operationID: 100, operationName: "asnInvoke" },
		payLoad: {},
		invokeContext: createInvokeContext(transport, 100, "asnInvoke", false),
	});
	assert.equal(sent, undefined);

	const eventContext = createInvokeContext(transport, 200, "asnEvent", true);
	eventContext.bSendEventSynchronous = true;
	assert.equal(
		transport.sendEvent({
			invoke: { invokeID: 99999, operationID: 200, operationName: "asnEvent" },
			payLoad: {},
			invokeContext: eventContext,
		}),
		true,
	);
});

test("pauseRoseProcessing completes pending invokes with shutdown", async () => {
	const transport = new PendingTestTransport();
	const pending = transport.sendInvoke({
		invoke: { invokeID: 7, operationID: 100, operationName: "asnInvoke" },
		payLoad: {},
		invokeContext: createInvokeContext(transport, 100, "asnInvoke", false),
	});

	transport.pauseRoseProcessing();
	const result = await pending;
	assert.ok(result instanceof ROSEReject);
	assert.equal(result.reject?.invokeProblem, ROSE_TE_SHUTDOWN);
});

test("receiveHandleROSEMessage rejects inbound invoke while paused", async () => {
	const transport = new TestTransport();
	transport.pauseRoseProcessing();

	const response = await transport.receiveHandleROSEMessage(
		{ invoke: { invokeID: 3, operationID: 100, operationName: "asnInvoke" } },
		{},
		new ReceiveInvokeContext({ encoding: EASN1TransportEncoding.JSON }),
	);

	assert.ok(response);
	assert.match(String(response?.payLoad), /"invokeProblem":2/);
});

test("pauseRoseProcessing blocks inbound dispatch until resumeRoseProcessing", async () => {
	const transport = new TestTransport();
	let handlerCalls = 0;
	const handler = {
		getNameForOperationID: () => undefined,
		getIDForOperationName: () => undefined,
		onInvoke: async () => {
			handlerCalls += 1;
			return undefined;
		},
	};
	transport.registerOperation(handler, handler as never, 100, "asnInvoke", "TestModule", 100, 0, 0, false);
	transport.pauseRoseProcessing();

	const blocked = await transport.receiveHandleROSEMessage(
		{ invoke: { invokeID: 3, operationID: 100, operationName: "asnInvoke" } },
		{},
		new ReceiveInvokeContext({ encoding: EASN1TransportEncoding.JSON }),
	);
	assert.match(String(blocked?.payLoad), /"invokeProblem":2/);
	assert.equal(handlerCalls, 0);

	transport.resumeRoseProcessing();
	await transport.receiveHandleROSEMessage(
		{ invoke: { invokeID: 4, operationID: 100, operationName: "asnInvoke" } },
		{},
		new ReceiveInvokeContext({ encoding: EASN1TransportEncoding.JSON }),
	);
	assert.equal(handlerCalls, 1);
});

test("late result after pauseRoseProcessing does not replace the shutdown reject", async () => {
	const transport = new PendingTestTransport();
	const pending = transport.sendInvoke({
		invoke: { invokeID: 7, operationID: 100, operationName: "asnInvoke" },
		payLoad: {},
		invokeContext: createInvokeContext(transport, 100, "asnInvoke", false),
	});

	transport.pauseRoseProcessing();
	const shutdown = await pending;
	assert.ok(shutdown instanceof ROSEReject);
	assert.equal(shutdown.reject?.invokeProblem, ROSE_TE_SHUTDOWN);

	const late = await transport.receiveHandleROSEMessage(
		{ result: { invokeID: 7, result: {} } },
		{},
		new ReceiveInvokeContext({ encoding: EASN1TransportEncoding.JSON }),
	);
	assert.equal(late, undefined);

	transport.resumeRoseProcessing();
	let settled = false;
	const next = transport
		.sendInvoke({
			invoke: { invokeID: 8, operationID: 100, operationName: "asnInvoke" },
			payLoad: {},
			invokeContext: createInvokeContext(transport, 100, "asnInvoke", false),
		})
		.then((value) => {
			settled = true;
			return value;
		});
	await new Promise((resolve) => setTimeout(resolve, 20));
	assert.equal(settled, false);
	transport.pauseRoseProcessing();
	const closed = await next;
	assert.ok(closed instanceof ROSEReject);
	assert.equal(closed.reject?.invokeProblem, ROSE_TE_SHUTDOWN);
});
