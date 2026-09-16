// Run: npx tsx typescript/TSROSEBase.invokeTimeout.test.ts
// Parity: cpp-lib/tests/invoke_context_tests.cpp (InvokeWireTimeoutTest, CreateOutboundInvokeContextOptionalTimeout, outbound wire encode)
import assert from "node:assert/strict";
import test from "node:test";
import * as ENetUC_Common from "./stub/ENetUC_Common.js";
import * as ENetUC_Common_Converter from "./stub/ENetUC_Common_Converter.js";
import { ROSEMessage } from "./stub/SNACCROSE.js";
import { ROSEInvoke_Converter, ROSEMessage_Converter } from "./stub/SNACCROSE_Converter.js";
import { ConverterErrors } from "./stub/TSConverterBase.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";
import { ReceiveInvokeContext, SendInvokeContext } from "./stub/TSROSEBase.js";
import { CapturingClientTransport, noopInvokeHandler, TestRose } from "./support/rose_test_transport.js";
import { roseHandleInvokeTemplate, roseInvoke } from "./support/snacc_test_helpers.js";

test("CreateOutboundInvokeContextOptionalTimeout: unset, zero, and explicit ms", () => {
	const unset = new SendInvokeContext({});
	assert.equal(unset.invokeTimeout(), undefined);

	const fireAndForget = new SendInvokeContext({ invokeTimeoutMs: 0 });
	assert.equal(fireAndForget.invokeTimeout(), 0);

	const explicit = new SendInvokeContext({ invokeTimeoutMs: 250 });
	assert.equal(explicit.invokeTimeout(), 250);
});

test("ClearInvokeTimeoutRestoresUnset", () => {
	const ctx = new SendInvokeContext({});
	ctx.setInvokeTimeout(250);
	assert.equal(ctx.invokeTimeout(), 250);
	ctx.clearInvokeTimeout();
	assert.equal(ctx.invokeTimeout(), undefined);
});

test("RoseInvokeJsonRoundTripPreservesTimeout", () => {
	const errors = new ConverterErrors();
	const encoded = ROSEInvoke_Converter.toJSON(
		roseInvoke({ invokeID: 1, operationID: 42, timeout: 500 }),
		errors,
	);
	assert.ok(encoded);
	assert.equal(errors.length, 0);

	const decoded = ROSEInvoke_Converter.fromJSON(encoded, errors);
	assert.ok(decoded);
	assert.equal(errors.length, 0);
	assert.equal(decoded.timeout, 500);
});

test("RoseInvokeBerRoundTripPreservesTimeout", () => {
	const errors = new ConverterErrors();
	const ber = ROSEInvoke_Converter.toBER(
		roseInvoke({ invokeID: 1, operationID: 42, timeout: 500 }),
		errors,
		undefined,
		undefined,
		1,
	);
	assert.ok(ber);
	assert.equal(errors.length, 0);

	const decoded = ROSEInvoke_Converter.fromBER(ber, undefined, undefined, undefined, false);
	assert.ok(decoded);
	assert.equal(decoded.timeout, 500);
});

test("InboundInitCopiesWireInvokeTimeout", () => {
	const ctx = ReceiveInvokeContext.create(roseInvoke({
		invokeID: 1,
		operationID: 43210,
		timeout: 750,
	}));
	assert.equal(ctx.invokeTimeout(), 750);
});

test("InboundInitWithoutWireTimeoutLeavesUnset", () => {
	const ctx = ReceiveInvokeContext.create(roseInvoke({
		invokeID: 1,
		operationID: 43210,
	}));
	assert.equal(ctx.invokeTimeout(), undefined);
});

test("InboundInitIgnoresZeroWireTimeout", () => {
	const ctx = ReceiveInvokeContext.create(roseInvoke({
		invokeID: 1,
		operationID: 43210,
		timeout: 0,
	}));
	assert.equal(ctx.invokeTimeout(), undefined);
});

test("OutboundWireInvokeTimeoutEncodesOnInvokeJson", async () => {
	const transport = new CapturingClientTransport();
	transport.registerOperation(
		noopInvokeHandler,
		noopInvokeHandler as never,
		4100,
		"asnTestInvoke",
		"TestModule",
		100,
		0,
		0,
		false,
	);
	const rose = new TestRose(transport);
	const argument = new ENetUC_Common.AsnOptionalParamChoice({ integerdata: 1 });

	await rose.handleInvoke(
		argument,
		roseHandleInvokeTemplate(new ENetUC_Common.AsnOptionalParamChoice({} as ENetUC_Common.AsnOptionalParamChoice)),
		4100,
		"asnTestInvoke",
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		{ invokeTimeoutMs: 500, encoding: EASN1TransportEncoding.JSON },
	);

	assert.ok(transport.lastInvokeData);
	assert.equal(transport.lastInvokeData.invoke.timeout, 500);
	assert.equal(transport.lastInvokeData.invokeContext.invokeTimeout(), 500);

	const errors = new ConverterErrors();
	const decoded = ROSEMessage_Converter.fromJSON(
		transport.lastInvokeData.payLoad as object,
		errors,
	) as ROSEMessage;
	assert.equal(errors.length, 0);
	assert.ok(decoded.invoke);
	assert.equal(decoded.invoke.timeout, 500);
});

test("OutboundDefaultInvokeTimeoutOmitsWireValue", async () => {
	const transport = new CapturingClientTransport();
	transport.registerOperation(
		noopInvokeHandler,
		noopInvokeHandler as never,
		4100,
		"asnTestInvoke",
		"TestModule",
		100,
		0,
		0,
		false,
	);
	const rose = new TestRose(transport);
	const argument = new ENetUC_Common.AsnOptionalParamChoice({ integerdata: 1 });

	await rose.handleInvoke(
		argument,
		roseHandleInvokeTemplate(new ENetUC_Common.AsnOptionalParamChoice({} as ENetUC_Common.AsnOptionalParamChoice)),
		4100,
		"asnTestInvoke",
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		{ encoding: EASN1TransportEncoding.JSON },
	);

	assert.ok(transport.lastInvokeData);
	assert.equal(transport.lastInvokeData.invoke.timeout, undefined);
	assert.equal(transport.lastInvokeData.invokeContext.invokeTimeout(), undefined);
});

test("OutboundFireAndForgetOmitsWireValue", async () => {
	const transport = new CapturingClientTransport();
	transport.registerOperation(
		noopInvokeHandler,
		noopInvokeHandler as never,
		4100,
		"asnTestInvoke",
		"TestModule",
		100,
		0,
		0,
		false,
	);
	const rose = new TestRose(transport);
	const argument = new ENetUC_Common.AsnOptionalParamChoice({ integerdata: 1 });

	await rose.handleInvoke(
		argument,
		roseHandleInvokeTemplate(new ENetUC_Common.AsnOptionalParamChoice({} as ENetUC_Common.AsnOptionalParamChoice)),
		4100,
		"asnTestInvoke",
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		{ invokeTimeoutMs: 0, encoding: EASN1TransportEncoding.JSON },
	);

	assert.ok(transport.lastInvokeData);
	assert.equal(transport.lastInvokeData.invoke.timeout, undefined);
	assert.equal(transport.lastInvokeData.invokeContext.invokeTimeout(), 0);
});

test("OutboundEventOmitsWireTimeoutEvenWhenSet", async () => {
	const transport = new CapturingClientTransport();
	transport.registerOperation(
		noopInvokeHandler,
		noopInvokeHandler as never,
		4150,
		"asnTestEvent",
		"TestModule",
		100,
		0,
		0,
		true,
	);
	const rose = new TestRose(transport);
	const argument = new ENetUC_Common.AsnOptionalParamChoice({ integerdata: 1 });

	rose.handleEvent(
		argument,
		4150,
		"asnTestEvent",
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		{ invokeTimeoutMs: 500, encoding: EASN1TransportEncoding.JSON },
	);

	await new Promise<void>((resolve) => setImmediate(resolve));

	assert.ok(transport.lastInvokeData);
	assert.equal(transport.lastInvokeData.invoke.invokeID, 99999);
	assert.equal(transport.lastInvokeData.invoke.timeout, undefined);
});
