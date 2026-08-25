// Run: npx tsx compiler/back-ends/ts-gen/tests/TSASN1Base.roseSessionSubscription.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import {
	ASN1ClassInstanceType,
	TSASN1Base,
} from "./workdir/TSASN1Base.js";
import { RoseSessionSubscriptionStore } from "./workdir/RoseSessionSubscriptionStore.js";
import { ROSEBase } from "./workdir/TSROSEBase.js";
import { EASN1TransportEncoding } from "./workdir/TSInvokeContext.js";
import type { IASN1InvokeData, IASN1LogData, IReceiveInvokeContext } from "./workdir/TSROSEBase.js";
import type { ROSEError, ROSEInvoke, ROSEResult, ROSEReject } from "./workdir/SNACCROSE.js";

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

	public sendEvent(_data: IASN1InvokeData): undefined {
		this.sentEvents++;
		return undefined;
	}

	public sendEventSync(_data: IASN1InvokeData): boolean {
		this.sentEvents++;
		return true;
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
