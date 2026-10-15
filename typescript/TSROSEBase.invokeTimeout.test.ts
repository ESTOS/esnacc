// Run: npx tsx typescript/TSROSEBase.invokeTimeout.test.ts
// Parity: cpp-lib/tests/invoke_context_tests.cpp (InvokeWireTimeoutTest, CreateOutboundInvokeContextOptionalTimeout, outbound wire encode)
// JSON always writes the resolved timeout onto ROSEInvoke.timeout. BER writes it only after setSendTimeoutInInvokes(true).
// The outbound context still stores the local wait either way. 0 is fire-and-forget and is not an expired queue deadline.
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
import { roseInvoke } from "./support/snacc_test_helpers.js";

/**
 * A normal non-event ROSEInvoke carries a timeout. A suppressed stub default may omit it.
 * Without expected, the value must be a positive deadline. 0 is fire-and-forget and is still present.
 *
 * @param timeout - ROSEInvoke.timeout taken from the in-memory invoke or the decoded payload
 * @param expected - when set, the exact milliseconds that must be on the message
 */
function assertRoseInvokeTimeoutPresent(timeout: number | undefined, expected?: number): void {
	assert.equal(typeof timeout, "number");
	assert.notEqual(timeout, undefined);
	if (expected === undefined)
		assert.ok(timeout !== undefined && timeout > 0);
	else
		assert.equal(timeout, expected);
}

/** Transport whose stub default was cleared, so an omitted caller timeout stays off the wire. */
class EmptyDefaultTransport extends CapturingClientTransport {
	public constructor() {
		super();
		this.defaultTimeout = 0;
	}
}

/**
 * Encodes one outbound invoke or event and checks ROSEInvoke.timeout on the invoke and the decoded payload.
 * The timeout tests call this for JSON and BER. A false includeContext is a call with no invoke context.
 * "stub" expects the transport default on the wire and on the context.
 * "omit" expects the optional field and the context timeout to stay unset.
 * "local-only" expects the wire field to stay off and the context to keep the local wait.
 *
 * @param options.encoding - JSON or BER for this call
 * @param options.transport - capturing transport; a new JSON client is created when omitted
 * @param options.context - caller context, passed through unchanged when includeContext is true
 * @param options.includeContext - false omits the context argument
 * @param options.expected - milliseconds, the stub default, an omitted field, or a local-only timeout
 * @param options.event - true sends an event (invoke id 99999), which never carries a timeout
 * @param options.sendTimeoutInInvokes - when true, BER may write ROSEInvoke.timeout
 */
async function assertEncodedTimeout(options: {
	encoding: EASN1TransportEncoding;
	transport?: CapturingClientTransport;
	context?: object;
	includeContext: boolean;
	expected: number | "stub" | "omit" | "local-only";
	event?: boolean;
	sendTimeoutInInvokes?: boolean;
}): Promise<void> {
	const transport = options.transport ?? new CapturingClientTransport();
	transport.setEncoding(options.encoding);
	if (options.sendTimeoutInInvokes)
		transport.setSendTimeoutInInvokes(true);
	const operationID = options.event ? 4150 : 4100;
	const operationName = options.event ? "asnTestEvent" : "asnTestInvoke";
	transport.registerOperation(
		noopInvokeHandler,
		noopInvokeHandler as never,
		operationID,
		operationName,
		"TestModule",
		100,
		0,
		0,
		options.event === true,
	);
	const rose = new TestRose(transport);
	const argument = new ENetUC_Common.AsnOptionalParamChoice({ integerdata: 1 });
	const converter = ENetUC_Common_Converter.AsnOptionalParamChoice_Converter;

	if (options.event) {
		rose.handleEvent(argument, operationID, operationName, converter, options.includeContext ? options.context : undefined);
		await new Promise<void>((resolve) => setImmediate(resolve));
	} else if (options.includeContext) {
		await rose.handleInvoke(argument, ENetUC_Common.AsnOptionalParamChoice, operationID, operationName, converter, converter, options.context);
	} else {
		await rose.handleInvoke(argument, ENetUC_Common.AsnOptionalParamChoice, operationID, operationName, converter, converter);
	}

	assert.ok(transport.lastInvokeData);
	const callerTimeout = localTimeoutFromCaller(options.includeContext ? options.context : undefined);
	const stubTimeout = transport.getDefaultInvokeTimeout();
	const expectedMs = options.expected === "stub"
		? stubTimeout
		: options.expected === "local-only"
			? (callerTimeout !== undefined ? callerTimeout : (stubTimeout > 0 ? stubTimeout : undefined))
			: options.expected;
	if (expectedMs === "omit" || options.expected === "local-only")
		assert.equal(transport.lastInvokeData.invoke.timeout, undefined);
	else
		assertRoseInvokeTimeoutPresent(transport.lastInvokeData.invoke.timeout, expectedWireMs(expectedMs));
	if (!options.event) {
		if (expectedMs === "omit")
			assert.equal(transport.lastInvokeData.invokeContext.invokeTimeout(), undefined);
		else
			assert.equal(transport.lastInvokeData.invokeContext.invokeTimeout(), expectedMs);
	}
	if (options.event)
		assert.equal(transport.lastInvokeData.invoke.invokeID, 99999);

	const errors = new ConverterErrors();
	const payLoad = transport.lastInvokeData.payLoad;
	const decoded = (options.encoding === EASN1TransportEncoding.BER
		? ROSEMessage_Converter.fromBER(new Uint8Array((payLoad as { toBER(sizeOnly: boolean): ArrayBuffer }).toBER(false)), errors)
		: ROSEMessage_Converter.fromJSON(payLoad as object, errors)) as ROSEMessage;
	assert.equal(errors.length, 0, errors.getDiagnostic());
	assert.ok(decoded.invoke);
	if (expectedMs === "omit" || options.expected === "local-only")
		assert.equal(decoded.invoke.timeout, undefined);
	else
		assertRoseInvokeTimeoutPresent(decoded.invoke.timeout, expectedWireMs(expectedMs));
}

/**
 * Narrows a resolved expectation to the milliseconds that must appear on the wire.
 * Callers use this only after they have rejected the omit and local-only cases.
 *
 * @param expectedMs - milliseconds, or the omit sentinel that must not reach this helper
 * @returns the millisecond value
 */
function expectedWireMs(expectedMs: number | "omit" | undefined): number {
	if (typeof expectedMs !== "number")
		throw new Error("expected a numeric ROSEInvoke.timeout");
	return expectedMs;
}

/**
 * Reads a caller timeout from a plain params object or a live invoke context.
 * A cleared context returns undefined so the stub default can fill the local wait.
 *
 * @param context - the object passed as the invoke context, when one was passed
 * @returns the caller timeout, including 0, or undefined when the caller left it unset
 */
function localTimeoutFromCaller(context: object | undefined): number | undefined {
	if (!context)
		return undefined;
	const live = context as { invokeTimeout?: () => number | undefined; invokeTimeoutMs?: number };
	if (typeof live.invokeTimeout === "function")
		return live.invokeTimeout();
	if (typeof live.invokeTimeoutMs === "number")
		return live.invokeTimeoutMs;
	return undefined;
}

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

test("InboundInitCopiesZeroWireTimeout", () => {
	const ctx = ReceiveInvokeContext.create(roseInvoke({
		invokeID: 1,
		operationID: 43210,
		timeout: 0,
	}));
	assert.equal(ctx.invokeTimeout(), 0);
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
		ENetUC_Common.AsnOptionalParamChoice,
		4100,
		"asnTestInvoke",
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		{ invokeTimeoutMs: 500, encoding: EASN1TransportEncoding.JSON },
	);

	assert.ok(transport.lastInvokeData);
	assertRoseInvokeTimeoutPresent(transport.lastInvokeData.invoke.timeout);
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
	assertRoseInvokeTimeoutPresent(decoded.invoke.timeout);
});

test("OutboundDefaultInvokeTimeoutUsesStubDefault", async () => {
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
		ENetUC_Common.AsnOptionalParamChoice,
		4100,
		"asnTestInvoke",
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		{ encoding: EASN1TransportEncoding.JSON },
	);

	assert.ok(transport.lastInvokeData);
	const stubTimeout = transport.getDefaultInvokeTimeout();
	assertRoseInvokeTimeoutPresent(transport.lastInvokeData.invoke.timeout);
	assert.equal(transport.lastInvokeData.invoke.timeout, stubTimeout);
	assert.equal(transport.lastInvokeData.invokeContext.invokeTimeout(), stubTimeout);

	const errors = new ConverterErrors();
	const decoded = ROSEMessage_Converter.fromJSON(
		transport.lastInvokeData.payLoad as object,
		errors,
	) as ROSEMessage;
	assert.equal(errors.length, 0);
	assert.ok(decoded.invoke);
	assertRoseInvokeTimeoutPresent(decoded.invoke.timeout);
	assert.equal(decoded.invoke.timeout, stubTimeout);
});

test("OutboundWithoutInvokeContextUsesStubDefault", async () => {
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
		ENetUC_Common.AsnOptionalParamChoice,
		4100,
		"asnTestInvoke",
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
	);

	assert.ok(transport.lastInvokeData);
	const stubTimeout = transport.getDefaultInvokeTimeout();
	assertRoseInvokeTimeoutPresent(transport.lastInvokeData.invoke.timeout);
	assert.equal(transport.lastInvokeData.invoke.timeout, stubTimeout);
	assert.equal(transport.lastInvokeData.invokeContext.invokeTimeout(), stubTimeout);

	const errors = new ConverterErrors();
	const decoded = ROSEMessage_Converter.fromJSON(
		transport.lastInvokeData.payLoad as object,
		errors,
	) as ROSEMessage;
	assert.equal(errors.length, 0);
	assert.ok(decoded.invoke);
	assertRoseInvokeTimeoutPresent(decoded.invoke.timeout);
	assert.equal(decoded.invoke.timeout, stubTimeout);
});

test("OutboundUndefinedInvokeTimeoutUsesStubDefault", async () => {
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
		ENetUC_Common.AsnOptionalParamChoice,
		4100,
		"asnTestInvoke",
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		{ invokeTimeoutMs: undefined, encoding: EASN1TransportEncoding.JSON },
	);

	assert.ok(transport.lastInvokeData);
	const stubTimeout = transport.getDefaultInvokeTimeout();
	assertRoseInvokeTimeoutPresent(transport.lastInvokeData.invoke.timeout);
	assert.equal(transport.lastInvokeData.invoke.timeout, stubTimeout);
	assert.equal(transport.lastInvokeData.invokeContext.invokeTimeout(), stubTimeout);

	const errors = new ConverterErrors();
	const decoded = ROSEMessage_Converter.fromJSON(
		transport.lastInvokeData.payLoad as object,
		errors,
	) as ROSEMessage;
	assert.equal(errors.length, 0);
	assert.ok(decoded.invoke);
	assertRoseInvokeTimeoutPresent(decoded.invoke.timeout);
	assert.equal(decoded.invoke.timeout, stubTimeout);
});

test("OutboundClearedInvokeContextUsesStubDefault", async () => {
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
	const callerContext = new SendInvokeContext({ invokeTimeoutMs: 500, encoding: EASN1TransportEncoding.JSON });
	callerContext.clearInvokeTimeout();

	await rose.handleInvoke(
		argument,
		ENetUC_Common.AsnOptionalParamChoice,
		4100,
		"asnTestInvoke",
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		callerContext,
	);

	assert.ok(transport.lastInvokeData);
	const stubTimeout = transport.getDefaultInvokeTimeout();
	assertRoseInvokeTimeoutPresent(transport.lastInvokeData.invoke.timeout);
	assert.equal(transport.lastInvokeData.invoke.timeout, stubTimeout);
	assert.equal(transport.lastInvokeData.invokeContext.invokeTimeout(), stubTimeout);

	const errors = new ConverterErrors();
	const decoded = ROSEMessage_Converter.fromJSON(
		transport.lastInvokeData.payLoad as object,
		errors,
	) as ROSEMessage;
	assert.equal(errors.length, 0);
	assert.ok(decoded.invoke);
	assertRoseInvokeTimeoutPresent(decoded.invoke.timeout);
	assert.equal(decoded.invoke.timeout, stubTimeout);
});

test("OutboundInvokeContextTimeoutEncodesOnInvoke", async () => {
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
	const callerContext = new SendInvokeContext({ invokeTimeoutMs: 750, encoding: EASN1TransportEncoding.JSON });

	await rose.handleInvoke(
		argument,
		ENetUC_Common.AsnOptionalParamChoice,
		4100,
		"asnTestInvoke",
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		callerContext,
	);

	assert.ok(transport.lastInvokeData);
	assertRoseInvokeTimeoutPresent(transport.lastInvokeData.invoke.timeout, 750);
	assert.equal(transport.lastInvokeData.invokeContext.invokeTimeout(), 750);

	const errors = new ConverterErrors();
	const decoded = ROSEMessage_Converter.fromJSON(
		transport.lastInvokeData.payLoad as object,
		errors,
	) as ROSEMessage;
	assert.equal(errors.length, 0);
	assert.ok(decoded.invoke);
	assertRoseInvokeTimeoutPresent(decoded.invoke.timeout, 750);
});

test("OutboundSuppressedStubDefaultOmitsOptionalTimeout", async () => {
	const transport = new EmptyDefaultTransport();
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
		ENetUC_Common.AsnOptionalParamChoice,
		4100,
		"asnTestInvoke",
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		{ encoding: EASN1TransportEncoding.JSON },
	);

	assert.ok(transport.lastInvokeData);
	assert.equal(transport.lastInvokeData.invoke.timeout, undefined);
	assert.equal(transport.lastInvokeData.invokeContext.invokeTimeout(), undefined);

	const errors = new ConverterErrors();
	const decoded = ROSEMessage_Converter.fromJSON(
		transport.lastInvokeData.payLoad as object,
		errors,
	) as ROSEMessage;
	assert.equal(errors.length, 0);
	assert.ok(decoded.invoke);
	assert.equal(decoded.invoke.timeout, undefined);
});

test("OutboundFireAndForgetEncodesZeroTimeout", async () => {
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
		ENetUC_Common.AsnOptionalParamChoice,
		4100,
		"asnTestInvoke",
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		ENetUC_Common_Converter.AsnOptionalParamChoice_Converter,
		{ invokeTimeoutMs: 0, encoding: EASN1TransportEncoding.JSON },
	);

	assert.ok(transport.lastInvokeData);
	assertRoseInvokeTimeoutPresent(transport.lastInvokeData.invoke.timeout, 0);
	assert.equal(transport.lastInvokeData.invokeContext.invokeTimeout(), 0);

	const errors = new ConverterErrors();
	const decoded = ROSEMessage_Converter.fromJSON(
		transport.lastInvokeData.payLoad as object,
		errors,
	) as ROSEMessage;
	assert.equal(errors.length, 0);
	assert.ok(decoded.invoke);
	assertRoseInvokeTimeoutPresent(decoded.invoke.timeout, 0);
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

test("OutboundWithoutInvokeContextUsesStubDefaultBer", async () => {
	await assertEncodedTimeout({ encoding: EASN1TransportEncoding.BER, includeContext: false, expected: "local-only" });
});

test("OutboundDefaultInvokeTimeoutUsesStubDefaultBer", async () => {
	await assertEncodedTimeout({
		encoding: EASN1TransportEncoding.BER,
		includeContext: true,
		context: { encoding: EASN1TransportEncoding.BER },
		expected: "local-only",
	});
});

test("OutboundUndefinedInvokeTimeoutUsesStubDefaultBer", async () => {
	await assertEncodedTimeout({
		encoding: EASN1TransportEncoding.BER,
		includeContext: true,
		context: { invokeTimeoutMs: undefined, encoding: EASN1TransportEncoding.BER },
		expected: "local-only",
	});
});

test("OutboundClearedInvokeContextUsesStubDefaultBer", async () => {
	const callerContext = new SendInvokeContext({ invokeTimeoutMs: 500, encoding: EASN1TransportEncoding.BER });
	callerContext.clearInvokeTimeout();
	await assertEncodedTimeout({
		encoding: EASN1TransportEncoding.BER,
		includeContext: true,
		context: callerContext,
		expected: "local-only",
	});
});

test("OutboundWireInvokeTimeoutEncodesOnInvokeBer", async () => {
	await assertEncodedTimeout({
		encoding: EASN1TransportEncoding.BER,
		includeContext: true,
		context: { invokeTimeoutMs: 500, encoding: EASN1TransportEncoding.BER },
		expected: "local-only",
	});
});

test("OutboundInvokeContextTimeoutEncodesOnInvokeBer", async () => {
	const callerContext = new SendInvokeContext({ invokeTimeoutMs: 750, encoding: EASN1TransportEncoding.BER });
	await assertEncodedTimeout({
		encoding: EASN1TransportEncoding.BER,
		includeContext: true,
		context: callerContext,
		expected: "local-only",
	});
});

test("OutboundFireAndForgetEncodesZeroTimeoutBer", async () => {
	await assertEncodedTimeout({
		encoding: EASN1TransportEncoding.BER,
		includeContext: true,
		context: { invokeTimeoutMs: 0, encoding: EASN1TransportEncoding.BER },
		expected: "local-only",
	});
});

test("OutboundWithoutInvokeContextWritesTimeoutWhenEnabledBer", async () => {
	await assertEncodedTimeout({
		encoding: EASN1TransportEncoding.BER,
		includeContext: false,
		expected: "stub",
		sendTimeoutInInvokes: true,
	});
});

test("OutboundWireInvokeTimeoutEncodesWhenEnabledBer", async () => {
	await assertEncodedTimeout({
		encoding: EASN1TransportEncoding.BER,
		includeContext: true,
		context: { invokeTimeoutMs: 500, encoding: EASN1TransportEncoding.BER },
		expected: 500,
		sendTimeoutInInvokes: true,
	});
});

test("OutboundFireAndForgetEncodesZeroTimeoutWhenEnabledBer", async () => {
	await assertEncodedTimeout({
		encoding: EASN1TransportEncoding.BER,
		includeContext: true,
		context: { invokeTimeoutMs: 0, encoding: EASN1TransportEncoding.BER },
		expected: 0,
		sendTimeoutInInvokes: true,
	});
});

test("OutboundInvokeContextZeroTimeoutEncodesOnInvoke", async () => {
	const callerContext = new SendInvokeContext({ invokeTimeoutMs: 0, encoding: EASN1TransportEncoding.JSON });
	await assertEncodedTimeout({
		encoding: EASN1TransportEncoding.JSON,
		includeContext: true,
		context: callerContext,
		expected: 0,
	});
});

test("OutboundInvokeContextZeroTimeoutEncodesOnInvokeBer", async () => {
	const callerContext = new SendInvokeContext({ invokeTimeoutMs: 0, encoding: EASN1TransportEncoding.BER });
	await assertEncodedTimeout({
		encoding: EASN1TransportEncoding.BER,
		includeContext: true,
		context: callerContext,
		expected: "local-only",
	});
});

test("OutboundEventOmitsWireTimeoutEvenWhenSetBer", async () => {
	await assertEncodedTimeout({
		encoding: EASN1TransportEncoding.BER,
		includeContext: true,
		context: { invokeTimeoutMs: 500, encoding: EASN1TransportEncoding.BER },
		expected: "omit",
		event: true,
	});
});

test("OutboundSuppressedStubDefaultOmitsOptionalTimeoutBer", async () => {
	await assertEncodedTimeout({
		encoding: EASN1TransportEncoding.BER,
		transport: new EmptyDefaultTransport(),
		includeContext: true,
		context: { encoding: EASN1TransportEncoding.BER },
		expected: "omit",
	});
});
