// Run: npx tsx compiler/back-ends/ts-gen/tests/TSASN1Base.invokeBlockPolicy.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import {
	ASN1ClassInstanceType,
	TSASN1Base,
} from "./workdir/TSASN1Base.js";
import { RoseSessionSubscriptionStore } from "./workdir/RoseSessionSubscriptionStore.js";
import { EASN1TransportEncoding } from "./workdir/TSInvokeContext.js";
import {
	CustomInvokeProblemEnum,
	OperationBlockPolicy,
	ROSE_REJECT_REMOTENOTCAPABLE,
} from "./workdir/TSROSEBase.js";
import { buildRemoteModuleCapabilities } from "./workdir/TSModuleCapabilities.js";
import type { IASN1InvokeData } from "./workdir/TSROSEBase.js";
import type { ROSEError, ROSEInvoke, ROSEResult } from "./workdir/SNACCROSE.js";
import { ROSEReject } from "./workdir/SNACCROSE.js";

class ClientTestTransport extends TSASN1Base {
	public sendInvokeCount = 0;

	public constructor() {
		super(EASN1TransportEncoding.JSON, ASN1ClassInstanceType.TSASN1NodeClient);
	}

	public async sendInvoke(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		++this.sendInvokeCount;
		return undefined;
	}

	public sendEventSync(_data: IASN1InvokeData): boolean {
		return true;
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
		this.markSessionSubscriptionStateSet();
		this.store.setSubscribedEvents(moduleIid, eventOpIds);
	}

	public setSupportedInvokes(moduleIid: number, invokeOpIds: readonly number[]): void {
		this.markSessionSubscriptionStateSet();
		this.store.setSupportedInvokes(moduleIid, invokeOpIds);
	}

	public isSubscribedEvent(eventOpId: number): boolean {
		return this.store.isSubscribedEvent(eventOpId);
	}

	public isSupportedInvoke(invokeOpId: number): boolean {
		return this.store.isSupportedInvoke(invokeOpId);
	}

	public sendInvoke(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		if (data.invoke.invokeID === 99999)
			this.sentEvents++;
		return Promise.resolve(undefined);
	}

	public sendEventSync(_data: IASN1InvokeData): boolean {
		this.sentEvents++;
		return true;
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
	assert.equal(reject.reject.invokeProblem, CustomInvokeProblemEnum.remoteNotCapable);
	assert.equal(CustomInvokeProblemEnum.remoteNotCapable, ROSE_REJECT_REMOTENOTCAPABLE);
});

test("subscription path: BlockUnsupportedOperations without state does not block", () => {
	const transport = new SubscriptionTransport();
	transport.setOperationBlockPolicy(OperationBlockPolicy.BlockUnsupportedOperations);
	assert.equal(transport.isOperationBlocked(2109, true), false);
	assert.equal(transport.isOperationBlocked(2109, false), false);
});

test("subscription path: blocks unsubscribed event after state is marked", () => {
	const transport = new SubscriptionTransport();
	transport.setOperationBlockPolicy(OperationBlockPolicy.BlockUnsupportedOperations);
	transport.setSubscribedEvents(100, [2170]);
	assert.equal(transport.isOperationBlocked(2109, true), true);
	assert.equal(transport.isOperationBlocked(2170, true), false);
});

test("completeIfOperationBlocked blocks unsubscribed event after state is marked", () => {
	const transport = new SubscriptionTransport();
	transport.setOperationBlockPolicy(OperationBlockPolicy.BlockUnsupportedOperations);
	transport.setSubscribedEvents(100, [2170]);
	const blocked = transport.completeIfOperationBlocked(2109, "asnJournalEntryChanged", true);
	assert.ok(blocked instanceof ROSEReject);
	assert.equal(blocked.reject.invokeProblem, CustomInvokeProblemEnum.remoteNotCapable);
	assert.equal(transport.completeIfOperationBlocked(2170, "asnJournalEntryChanged", true), undefined);
});

test("completeIfOperationBlocked returns remoteNotCapable for unsupported server invoke", () => {
	const transport = new SubscriptionTransport();
	transport.setOperationBlockPolicy(OperationBlockPolicy.BlockUnsupportedOperations);
	transport.setSupportedInvokes(100, [3001]);
	const reject = transport.completeIfOperationBlocked(3002, "asnExampleInvoke", false, 42);
	assert.ok(reject instanceof ROSEReject);
	assert.equal(reject.reject.invokeProblem, CustomInvokeProblemEnum.remoteNotCapable);
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
	transport.registerOperation(noopHandler, noopHandler as never, 4100, "asnGetSettings", "TestModule", 100, 0, 0, false);
	transport.registerOperation(noopHandler, noopHandler as never, 4101, "asnSetSettings", "TestModule", 100, 0, 0, false);
	transport.setRemoteModuleCapabilities(buildRemoteModuleCapabilities([
		{ moduleName: "TestModule", version: "1.0.0", invokeOpIds: [4100] },
	]));

	assert.equal(transport.hasRemoteModuleCapabilities(), true);
	assert.equal(transport.isSupportedOperation(4100), true);
	assert.equal(transport.isSupportedOperation(4101), false);
});
