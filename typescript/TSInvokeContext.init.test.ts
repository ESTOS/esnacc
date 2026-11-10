// Run: npx tsx typescript/TSInvokeContext.init.test.ts
// Parity: cpp-lib/tests/invoke_context_tests.cpp (InvokeContextInitTest)
import assert from "node:assert/strict";
import test from "node:test";
import { ROSEMessage } from "./stub/SNACCROSE.js";
import type { ROSEError, ROSEReject, ROSEResult } from "./stub/SNACCROSE.js";
import { ASN1ClassInstanceType, TSASN1Base } from "./stub/TSASN1Base.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";
import { ReceiveInvokeContext, SendInvokeContext } from "./stub/TSROSEBase.js";
import { ERoseResult, ROSE_NOERROR } from "./stub/TSROSEBase.js";
import type { IReceiveInvokeContext, IReceiveInvokeContextInit } from "./stub/TSROSEBase.js";
import type { IASN1InvokeData } from "./stub/TSROSEBase.js";
import { roseInvoke } from "./support/snacc_test_helpers.js";

class LookupServerTransport extends TSASN1Base {
	public constructor() {
		super(EASN1TransportEncoding.JSON, ASN1ClassInstanceType.TSASN1Server);
	}

	public async sendInvoke(_data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		return undefined;
	}

	/** Accepts every encoded event. These tests do not send. */
	public sendEvent(_data: IASN1InvokeData): ERoseResult {
		return ROSE_NOERROR;
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
		lookupHandler as never,
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
		invoke: roseInvoke({
			invokeID: 1,
			operationID: 43210,
		}),
	});

	await transport.receiveHandleROSEMessage(message, {}, invokeContext);
	assert.equal(invokeContext.operationName, "asnTestInboundName");
});

test("InboundReceiveResolvesOperationNameFromLookupWhenWireNamePresent", async () => {
	const transport = new LookupServerTransport();
	transport.registerOperation(
		lookupHandler,
		lookupHandler as never,
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
		invoke: roseInvoke({
			invokeID: 1,
			operationID: 43210,
			operationName: "wrongWireName",
		}),
	});

	await transport.receiveHandleROSEMessage(message, {}, invokeContext);
	assert.equal(invokeContext.operationName, "asnTestInboundName");
});

test("InboundReceiveCopiesWireTimeoutIncludingZero", async () => {
	const transport = new LookupServerTransport();
	const invokeContext = new ReceiveInvokeContext({ encoding: EASN1TransportEncoding.JSON });
	const message = new ROSEMessage({
		invoke: roseInvoke({
			invokeID: 7,
			operationID: 1,
			timeout: 0,
		}),
	});

	await transport.receiveHandleROSEMessage(message, {}, invokeContext);
	assert.equal(invokeContext.invokeTimeout(), 0);
});

test("InboundReceiveLeavesTimeoutUnsetWhenWireFieldAbsent", async () => {
	const transport = new LookupServerTransport();
	const invokeContext = new ReceiveInvokeContext({
		encoding: EASN1TransportEncoding.JSON,
		invokeTimeoutMs: 20000,
	});
	const message = new ROSEMessage({
		invoke: roseInvoke({
			invokeID: 8,
			operationID: 1,
		}),
	});

	await transport.receiveHandleROSEMessage(message, {}, invokeContext);
	assert.equal(invokeContext.invokeTimeout(), undefined);
});

class ProductReceiveContext extends ReceiveInvokeContext {
	public readonly productMarker = "product-session";
}

class ProductTransport extends LookupServerTransport {
	/**
	 * Replaces the inbound context with a product type, the way C++ CreateInvokeContext does.
	 * The base method resolves the operation name and the wire timeout. This override returns that state on the product class.
	 */
	public override createReceiveInvokeContext(init: IReceiveInvokeContextInit): ReceiveInvokeContext {
		const created = super.createReceiveInvokeContext(init);
		return new ProductReceiveContext({
			encoding: created.encoding,
			invokeID: created.invokeID,
			operationID: created.operationID,
			operationName: created.operationName,
			clientConnectionID: created.clientConnectionID,
			...(created.invokeTimeout() !== undefined ? { invokeTimeoutMs: created.invokeTimeout() } : {}),
		});
	}
}

test("CreateReceiveInvokeContextOverrideReachesInboundHandler", async () => {
	const transport = new ProductTransport();
	let seen: IReceiveInvokeContext | undefined;
	const handler = {
		getNameForOperationID: (id: number): string | undefined => (id === 43210 ? "asnTestInboundName" : undefined),
		getIDForOperationName: (name: string): number | undefined => (name === "asnTestInboundName" ? 43210 : undefined),
		onInvoke: async (_invoke: unknown, ctx: IReceiveInvokeContext) => {
			seen = ctx;
			return undefined;
		},
	};
	transport.registerOperation(handler, handler as never, 43210, "asnTestInboundName", "TestModule", 1, 0, 0, false);

	const invokeContext = new ReceiveInvokeContext({ encoding: EASN1TransportEncoding.JSON });
	const message = new ROSEMessage({
		invoke: roseInvoke({
			invokeID: 3,
			operationID: 43210,
			operationName: "wrongWireName",
			timeout: 500,
		}),
	});

	await transport.receiveHandleROSEMessage(message, {}, invokeContext);
	assert.ok(seen instanceof ProductReceiveContext);
	assert.equal(seen.productMarker, "product-session");
	assert.equal(seen.operationName, "asnTestInboundName");
	assert.equal(seen.invokeTimeout(), 500);
});

test("InboundResolvesOperationIdFromNameThenCanonicalContextName", () => {
	const transport = new LookupServerTransport();
	transport.registerOperation(
		lookupHandler,
		lookupHandler as never,
		43210,
		"asnTestInboundName",
		"TestModule",
		1,
		0,
		0,
		false,
	);

	const invoke = roseInvoke({
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
