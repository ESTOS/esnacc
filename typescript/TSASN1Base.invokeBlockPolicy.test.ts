// Run: npx tsx typescript/TSASN1Base.invokeBlockPolicy.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import { RoseSessionSubscriptionStore } from "./stub/RoseSessionSubscriptionStore.js";
import type { ROSEError, ROSEInvoke, ROSEResult } from "./stub/SNACCROSE.js";
import { ROSEReject } from "./stub/SNACCROSE.js";
import { ASN1ClassInstanceType, TSASN1Base } from "./stub/TSASN1Base.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";
import { buildRemoteModuleCapabilities } from "./stub/TSModuleCapabilities.js";
import { CustomInvokeProblemEnum, ERoseResult, OperationBlockPolicy, ROSEBase, ROSE_NOERROR, ROSE_REJECT_REMOTENOTCAPABLE } from "./stub/TSROSEBase.js";
import type { IASN1InvokeData } from "./stub/TSROSEBase.js";

class ClientTestTransport extends TSASN1Base {
	public sendInvokeCount = 0;

	public constructor() {
		super(EASN1TransportEncoding.JSON, ASN1ClassInstanceType.TSASN1NodeClient);
	}

	public async sendInvoke(_data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		++this.sendInvokeCount;
		return undefined;
	}

	/** Accepts every encoded event. These tests drive sendInvoke, not the socket. */
	public sendEvent(_data: IASN1InvokeData): ERoseResult {
		return ROSE_NOERROR;
	}

	public getSessionID(): string | undefined {
		return undefined;
	}
}

class SubscriptionTransport extends TSASN1Base {
	private readonly store = new RoseSessionSubscriptionStore();
	public sentEvents = 0;

	public constructor() {
		super(EASN1TransportEncoding.JSON, ASN1ClassInstanceType.TSASN1Server);
	}

	public setSubscribedEvents(moduleIid: number, eventOpIds: readonly number[]): void {
		this.store.setSubscribedEvents(moduleIid, eventOpIds);
	}

	public setSupportedInvokes(moduleIid: number, invokeOpIds: readonly number[]): void {
		this.store.setSupportedInvokes(moduleIid, invokeOpIds);
	}

	public isSubscribedEvent(eventOpId: number): boolean {
		return this.store.isSubscribedEvent(eventOpId);
	}

	public isSupportedInvoke(invokeOpId: number): boolean {
		return this.store.isSupportedInvoke(invokeOpId);
	}

	public sendInvoke(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		if (data.invoke.invokeID === 99999) {
			this.sentEvents++;
		}
		return Promise.resolve(undefined);
	}

	/** Counts events that passed the session filter. handleEvent calls this after the block check. */
	public sendEvent(_data: IASN1InvokeData): ERoseResult {
		this.sentEvents++;
		return ROSE_NOERROR;
	}

	public getSessionID(): string | undefined {
		return undefined;
	}
}

const noopHandler = {
	getNameForOperationID: () => undefined,
	getIDForOperationName: () => undefined,
	onInvoke: async () => undefined,
};

function createInvoke(operationID: number, operationName: string, invokeID = 1): ROSEInvoke {
	return {
		invokeID,
		operationID,
		operationName,
	} as ROSEInvoke;
}

test("negotiate path: blockUnsupportedOperations without snapshot does not block sendInvoke", async () => {
	const transport = new ClientTestTransport();
	transport.registerOperation(noopHandler, noopHandler as never, 100, "asnInvoke", "TestModule", 100, 0, 0, false);
	transport.setOperationBlockPolicy(OperationBlockPolicy.BlockUnsupportedOperations);
	assert.equal(transport.isOperationBlocked(100, false), false);
	assert.equal(transport.isOperationBlocked(2109, true), false);

	await transport.sendInvoke({
		invoke: createInvoke(100, "asnInvoke"),
		invokeContext: transport.getInvokeContextParams(undefined, 100, "asnInvoke", false),
		payLoad: new Uint8Array(),
	} as IASN1InvokeData);

	assert.equal(transport.sendInvokeCount, 1);
});

test("negotiate path: unsupported op id returns remoteNotCapable reject at stub gate", () => {
	const transport = new ClientTestTransport();
	transport.registerOperation(noopHandler, noopHandler as never, 100, "asnInvoke", "TestModule", 100, 0, 0, false);
	transport.setRemoteModuleCapabilities(buildRemoteModuleCapabilities([
		{ moduleName: "TestModule", version: "1.0.0", invokeOpIds: [200] },
	]));
	transport.setOperationBlockPolicy(OperationBlockPolicy.BlockUnsupportedOperations);

	const reject = transport.completeIfOperationBlocked(100, "asnInvoke", false);
	assert.ok(reject instanceof ROSEReject);
	assert.equal(reject.reject?.invokeProblem, CustomInvokeProblemEnum.remoteNotCapable);
	assert.equal(CustomInvokeProblemEnum.remoteNotCapable, ROSE_REJECT_REMOTENOTCAPABLE);
});

test("subscription path: blocks event and invoke absent from the store", () => {
	const transport = new SubscriptionTransport();
	transport.setOperationBlockPolicy(OperationBlockPolicy.BlockUnsupportedOperations);
	assert.equal(transport.isOperationBlocked(2109, true), true);
	assert.equal(transport.isOperationBlocked(2109, false), true);
});

test("subscription path: blocks unsubscribed event", () => {
	const transport = new SubscriptionTransport();
	transport.setOperationBlockPolicy(OperationBlockPolicy.BlockUnsupportedOperations);
	transport.setSubscribedEvents(100, [2170]);
	assert.equal(transport.isOperationBlocked(2109, true), true);
	assert.equal(transport.isOperationBlocked(2170, true), false);
});

test("subscription path: blocks unsupported invoke", () => {
	const transport = new SubscriptionTransport();
	transport.setOperationBlockPolicy(OperationBlockPolicy.BlockUnsupportedOperations);
	transport.setSupportedInvokes(100, [3001]);
	assert.equal(transport.isOperationBlocked(3002, false), true);
	assert.equal(transport.isOperationBlocked(3001, false), false);
});

test("completeIfOperationBlocked blocks unsubscribed event", () => {
	const transport = new SubscriptionTransport();
	transport.setOperationBlockPolicy(OperationBlockPolicy.BlockUnsupportedOperations);
	transport.setSubscribedEvents(100, [2170]);
	const blocked = transport.completeIfOperationBlocked(2109, "asnJournalEntryChanged", true);
	assert.ok(blocked instanceof ROSEReject);
	assert.equal(blocked.reject?.invokeProblem, CustomInvokeProblemEnum.remoteNotCapable);
	assert.equal(transport.completeIfOperationBlocked(2170, "asnJournalEntryChanged", true), undefined);
});

test("completeIfOperationBlocked returns remoteNotCapable for unsupported server invoke", () => {
	const transport = new SubscriptionTransport();
	transport.setOperationBlockPolicy(OperationBlockPolicy.BlockUnsupportedOperations);
	transport.setSupportedInvokes(100, [3001]);
	const reject = transport.completeIfOperationBlocked(3002, "asnExampleInvoke", false, 42);
	assert.ok(reject instanceof ROSEReject);
	assert.equal(reject.reject?.invokeProblem, CustomInvokeProblemEnum.remoteNotCapable);
});

test("clearRemoteModuleCapabilities stops blocking unsupported invoke", async () => {
	const transport = new ClientTestTransport();
	transport.registerOperation(noopHandler, noopHandler as never, 100, "asnInvoke", "TestModule", 100, 0, 0, false);
	transport.setRemoteModuleCapabilities(buildRemoteModuleCapabilities([
		{ moduleName: "TestModule", version: "1.0.0", invokeOpIds: [200] },
	]));
	transport.setOperationBlockPolicy(OperationBlockPolicy.BlockUnsupportedOperations);
	transport.clearRemoteModuleCapabilities();

	await transport.sendInvoke({
		invoke: createInvoke(100, "asnInvoke"),
		invokeContext: transport.getInvokeContextParams(undefined, 100, "asnInvoke", false),
		payLoad: new Uint8Array(),
	} as IASN1InvokeData);

	assert.equal(transport.sendInvokeCount, 1);
});

test("isSupportedOperation reflects applied snapshot", () => {
	const transport = new ClientTestTransport();
	transport.registerOperation(
		noopHandler,
		noopHandler as never,
		4100,
		"asnGetSettings",
		"TestModule",
		100,
		0,
		0,
		false,
	);
	transport.registerOperation(
		noopHandler,
		noopHandler as never,
		4101,
		"asnSetSettings",
		"TestModule",
		100,
		0,
		0,
		false,
	);
	transport.setRemoteModuleCapabilities(buildRemoteModuleCapabilities([
		{ moduleName: "TestModule", version: "1.0.0", invokeOpIds: [4100] },
	]));

	assert.equal(transport.hasRemoteModuleCapabilities(), true);
	assert.equal(transport.isSupportedOperation(4100), true);
	assert.equal(transport.isSupportedOperation(4101), false);
});

/**
 * Minimal ROSE stub so a test can call handleEvent, the path generated event_* methods use.
 * The abstract hooks are unused; the block check returns before encode.
 */
class EventSendRose extends ROSEBase {
	/** Builds a stub bound to the subscription transport under test. */
	public constructor(transport: SubscriptionTransport) {
		super(transport, false);
	}

	/** Log label only. handleEvent does not log when the session filter blocks the event. */
	public getLogData(): { className: string } {
		return { className: "EventSendRose" };
	}

	/** Unused. The block check returns before name lookup. */
	public getNameForOperationID(_id: number): string | undefined {
		return undefined;
	}

	/** Unused. The block check returns before name lookup. */
	public getIDForOperationName(_name: string): number | undefined {
		return undefined;
	}

	/** Unused. This fixture does not receive invokes. */
	public async onInvoke(): Promise<undefined> {
		return undefined;
	}
}

test("handleEvent returns remoteNotCapable and does not send a blocked event", () => {
	const transport = new SubscriptionTransport();
	transport.setOperationBlockPolicy(OperationBlockPolicy.BlockUnsupportedOperations);
	transport.setSubscribedEvents(100, [2170]);
	const rose = new EventSendRose(transport);

	const code = rose.handleEvent({}, 2109, "asnJournalEntryChanged", {} as never);

	assert.equal(code, ROSE_REJECT_REMOTENOTCAPABLE);
	assert.equal(transport.sentEvents, 0);
});
