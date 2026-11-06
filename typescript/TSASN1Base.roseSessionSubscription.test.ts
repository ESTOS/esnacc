// Run: npx tsx typescript/TSASN1Base.roseSessionSubscription.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import { RoseSessionSubscriptionStore } from "./stub/RoseSessionSubscriptionStore.js";
import type { ROSEError, ROSEInvoke, ROSEReject, ROSEResult } from "./stub/SNACCROSE.js";
import { ASN1ClassInstanceType, TSASN1Base } from "./stub/TSASN1Base.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";
import { ROSEBase, ROSE_NOERROR, type ERoseResult } from "./stub/TSROSEBase.js";
import type { IASN1InvokeData, IASN1LogData, IReceiveInvokeContext } from "./stub/TSROSEBase.js";

class SubscriptionTransport extends TSASN1Base {
	private readonly store = new RoseSessionSubscriptionStore();
	public sentEvents = 0;

	public constructor() {
		super(EASN1TransportEncoding.JSON, ASN1ClassInstanceType.TSASN1Server);
	}

	public clearAllSubscriptions(): void {
		this.store.clearAllSubscriptions();
	}

	public clearSubscribedEvents(moduleIid: number): void {
		this.store.clearSubscribedEvents(moduleIid);
	}

	public clearSupportedInvokes(moduleIid: number): void {
		this.store.clearSupportedInvokes(moduleIid);
	}

	public setSubscribedEvents(moduleIid: number, eventOpIds: readonly number[]): void {
		this.store.setSubscribedEvents(moduleIid, eventOpIds);
	}

	public addSubscribedEvent(moduleIid: number, eventOpId: number): void {
		this.store.addSubscribedEvent(moduleIid, eventOpId);
	}

	public setSupportedInvokes(moduleIid: number, invokeOpIds: readonly number[]): void {
		this.store.setSupportedInvokes(moduleIid, invokeOpIds);
	}

	public addSupportedInvoke(moduleIid: number, invokeOpId: number): void {
		this.store.addSupportedInvoke(moduleIid, invokeOpId);
	}

	public isSubscribedEvent(eventOpId: number): boolean {
		return this.store.isSubscribedEvent(eventOpId);
	}

	public isSupportedInvoke(invokeOpId: number): boolean {
		return this.store.isSupportedInvoke(invokeOpId);
	}

	public sendInvoke(_data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		return Promise.resolve(undefined);
	}

	public sendEvent(_data: IASN1InvokeData): ERoseResult {
		this.sentEvents++;
		return ROSE_NOERROR;
	}

	public getSessionID(): string | undefined {
		return undefined;
	}
}

class TestRoseComponent extends ROSEBase {
	public readonly logFilter: string[] = [];

	public constructor(transport: SubscriptionTransport) {
		super(transport, true);
	}

	public getLogData(): IASN1LogData {
		return { className: "TestRoseComponent" };
	}

	public getNameForOperationID(_id: number): string | undefined {
		return undefined;
	}

	public getIDForOperationName(_name: string): number | undefined {
		return undefined;
	}

	public async onInvoke(
		_invoke: ROSEInvoke,
		_invokeContext: IReceiveInvokeContext,
		_handler: unknown,
	): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		return undefined;
	}
}

test("RoseSessionSubscriptionStore tracks subscribed events", () => {
	const store = new RoseSessionSubscriptionStore();
	assert.equal(store.isSubscribedEvent(2109), false);
	store.addSubscribedEvent(2104, 2109);
	assert.equal(store.isSubscribedEvent(2109), true);
	assert.equal(store.isSubscribedEvent(2170), false);
	store.clearSubscribedEvents(2104);
	assert.equal(store.isSubscribedEvent(2109), false);
});

test("RoseSessionSubscriptionStore setSubscribedEvents replaces that module", () => {
	const store = new RoseSessionSubscriptionStore();
	store.setSubscribedEvents(2104, [2109, 2170]);
	store.addSubscribedEvent(2104, 2110);

	store.setSubscribedEvents(2104, [2170]);

	assert.equal(store.isSubscribedEvent(2109), false);
	assert.equal(store.isSubscribedEvent(2110), false);
	assert.equal(store.isSubscribedEvent(2170), true);
});

test("RoseSessionSubscriptionStore setSubscribedEvents leaves other modules", () => {
	const store = new RoseSessionSubscriptionStore();
	store.setSubscribedEvents(2104, [2109]);
	store.setSubscribedEvents(2200, [2170]);

	store.setSubscribedEvents(2104, [2110]);

	assert.equal(store.isSubscribedEvent(2109), false);
	assert.equal(store.isSubscribedEvent(2110), true);
	assert.equal(store.isSubscribedEvent(2170), true);
});

test("RoseSessionSubscriptionStore empty setSubscribedEvents clears that module only", () => {
	const store = new RoseSessionSubscriptionStore();
	store.setSubscribedEvents(2104, [2109]);
	store.setSubscribedEvents(2200, [2170]);

	store.setSubscribedEvents(2104, []);

	assert.equal(store.isSubscribedEvent(2109), false);
	assert.equal(store.isSubscribedEvent(2170), true);
});

test("RoseSessionSubscriptionStore clearSubscribedEvents leaves supported invokes", () => {
	const store = new RoseSessionSubscriptionStore();
	store.setSubscribedEvents(2104, [2109]);
	store.setSupportedInvokes(2104, [4100]);
	store.setSupportedInvokes(2200, [4200]);

	store.clearSubscribedEvents(2104);

	assert.equal(store.isSubscribedEvent(2109), false);
	assert.equal(store.isSupportedInvoke(4100), true);
	assert.equal(store.isSupportedInvoke(4200), true);
});

test("RoseSessionSubscriptionStore clearSupportedInvokes leaves subscribed events", () => {
	const store = new RoseSessionSubscriptionStore();
	store.setSubscribedEvents(2104, [2109]);
	store.setSupportedInvokes(2104, [4100]);

	store.clearSupportedInvokes(2104);

	assert.equal(store.isSubscribedEvent(2109), true);
	assert.equal(store.isSupportedInvoke(4100), false);
});

test("RoseSessionSubscriptionStore clearAllSubscriptions clears events and invokes", () => {
	const store = new RoseSessionSubscriptionStore();
	store.setSubscribedEvents(2104, [2109]);
	store.setSupportedInvokes(2200, [4100]);

	store.clearAllSubscriptions();

	assert.equal(store.isSubscribedEvent(2109), false);
	assert.equal(store.isSupportedInvoke(4100), false);
});

test("RoseSessionSubscriptionStore addSubscribedEvent keeps other modules", () => {
	const store = new RoseSessionSubscriptionStore();
	store.setSubscribedEvents(2104, [2109]);
	store.addSubscribedEvent(2200, 2170);

	store.addSubscribedEvent(2104, 2110);

	assert.equal(store.isSubscribedEvent(2109), true);
	assert.equal(store.isSubscribedEvent(2110), true);
	assert.equal(store.isSubscribedEvent(2170), true);
});

test("RoseSessionSubscriptionStore setSupportedInvokes replaces that module and leaves events", () => {
	const store = new RoseSessionSubscriptionStore();
	store.setSubscribedEvents(2104, [2109]);
	store.setSupportedInvokes(2104, [4100, 4101]);
	store.setSupportedInvokes(2200, [4200]);

	store.setSupportedInvokes(2104, [4101]);

	assert.equal(store.isSubscribedEvent(2109), true);
	assert.equal(store.isSupportedInvoke(4100), false);
	assert.equal(store.isSupportedInvoke(4101), true);
	assert.equal(store.isSupportedInvoke(4200), true);
});

test("RoseSessionSubscriptionStore empty setSupportedInvokes clears that module only", () => {
	const store = new RoseSessionSubscriptionStore();
	store.setSupportedInvokes(2104, [4100]);
	store.setSupportedInvokes(2200, [4200]);

	store.setSupportedInvokes(2104, []);

	assert.equal(store.isSupportedInvoke(4100), false);
	assert.equal(store.isSupportedInvoke(4200), true);
});

test("RoseSessionSubscriptionStore addSupportedInvoke keeps other modules", () => {
	const store = new RoseSessionSubscriptionStore();
	store.setSupportedInvokes(2104, [4100]);
	store.addSupportedInvoke(2200, 4200);

	store.addSupportedInvoke(2104, 4101);

	assert.equal(store.isSupportedInvoke(4100), true);
	assert.equal(store.isSupportedInvoke(4101), true);
	assert.equal(store.isSupportedInvoke(4200), true);
});

test("RoseSessionSubscriptionStore clear of an unknown module leaves the session", () => {
	const store = new RoseSessionSubscriptionStore();
	store.setSubscribedEvents(2104, [2109]);
	store.setSupportedInvokes(2104, [4100]);

	store.clearSubscribedEvents(9999);
	store.clearSupportedInvokes(9998);

	assert.equal(store.isSubscribedEvent(2109), true);
	assert.equal(store.isSupportedInvoke(4100), true);
});

test("ROSEBase forwards subscription queries to transport", () => {
	const transport = new SubscriptionTransport();
	const component = new TestRoseComponent(transport);

	assert.equal(component.isSubscribedEvent(2109), false);
	component.addSubscribedEvent(2104, 2109);
	assert.equal(component.isSubscribedEvent(2109), true);
	assert.equal(component.isSupportedInvoke(4100), false);
	component.addSupportedInvoke(2104, 4100);
	assert.equal(component.isSupportedInvoke(4100), true);
});
