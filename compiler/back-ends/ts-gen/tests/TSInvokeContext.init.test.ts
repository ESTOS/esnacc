// Run: npx tsx compiler/back-ends/ts-gen/tests/TSInvokeContext.init.test.ts
// Parity: cpp-lib/tests/invoke_context_tests.cpp (InvokeContextInitTest)
import assert from "node:assert/strict";
import test from "node:test";
import { ROSEInvoke, ROSEMessage } from "./workdir/SNACCROSE.js";
import {
	ASN1ClassInstanceType,
	TSASN1Base,
} from "./workdir/TSASN1Base.js";
import { EASN1TransportEncoding } from "./workdir/TSInvokeContext.js";
import { ReceiveInvokeContext, SendInvokeContext } from "./workdir/TSROSEBase.js";
import type { IASN1InvokeData } from "./workdir/TSROSEBase.js";
import type { ROSEError, ROSEReject, ROSEResult } from "./workdir/SNACCROSE.js";

class LookupServerTransport extends TSASN1Base {
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
}

const lookupHandler = {
	getNameForOperationID: (id: number): string | undefined => (id === 43210 ? "asnTestInboundName" : undefined),
	getIDForOperationName: (name: string): number | undefined => (name === "asnTestInboundName" ? 43210 : undefined),
	onInvoke: async () => undefined,
};

test("OutboundInitStoresExplicitOperationName", () => {
	const ctx = new SendInvokeContext({ operationName: "asnDeprecatedMethod" });
	assert.equal(ctx.operationName, "asnDeprecatedMethod");
});

test("OutboundInitWithoutNameLeavesOperationNameEmpty", () => {
	const ctx = new SendInvokeContext({});
	assert.equal(ctx.operationName, "");
});

test("InboundReceiveResolvesOperationNameFromLookupWhenWireNameAbsent", async () => {
	const transport = new LookupServerTransport();
	transport.registerOperation(
		lookupHandler,
		lookupHandler,
		43210,
		"asnTestInboundName",
		"TestModule",
		1,
		0,
		0,
		false,
	);

	const invokeContext = new ReceiveInvokeContext({ encoding: EASN1TransportEncoding.JSON });
	const message = new ROSEMessage({
		invoke: new ROSEInvoke({
			invokeID: 1,
			operationID: 43210,
		}),
	});

	await transport.receiveHandleROSEMessage(message, {}, invokeContext);
	assert.equal(invokeContext.operationName, "asnTestInboundName");
});

test("InboundReceiveUsesWireOperationNameWhenPresent", async () => {
	const transport = new LookupServerTransport();
	transport.registerOperation(
		lookupHandler,
		lookupHandler,
		43210,
		"asnTestInboundName",
		"TestModule",
		1,
		0,
		0,
		false,
	);

	const invokeContext = new ReceiveInvokeContext({ encoding: EASN1TransportEncoding.JSON });
	const message = new ROSEMessage({
		invoke: new ROSEInvoke({
			invokeID: 1,
			operationID: 43210,
			operationName: "wrongWireName",
		}),
	});

	await transport.receiveHandleROSEMessage(message, {}, invokeContext);
	// TS glue keeps wire operationName when present (C++ ignores wire name and uses lookup).
	assert.equal(invokeContext.operationName, "wrongWireName");
});

test("InboundResolvesOperationIdFromNameThenCanonicalContextName", () => {
	const transport = new LookupServerTransport();
	transport.registerOperation(
		lookupHandler,
		lookupHandler,
		43210,
		"asnTestInboundName",
		"TestModule",
		1,
		0,
		0,
		false,
	);

	const invoke = new ROSEInvoke({
		invokeID: 1,
		operationID: 0,
		operationName: "asnTestInboundName",
	});
	const resolvedId = transport.lookUpID("asnTestInboundName");
	assert.equal(resolvedId, 43210);
	invoke.operationID = resolvedId ?? 0;
	assert.equal(invoke.operationID, 43210);

	const ctx = ReceiveInvokeContext.create(invoke);
	assert.equal(ctx.operationName, "asnTestInboundName");
});
