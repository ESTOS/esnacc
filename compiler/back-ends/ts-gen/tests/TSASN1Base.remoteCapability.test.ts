// Run: npx tsx compiler/back-ends/ts-gen/tests/TSASN1Base.remoteCapability.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import {
	ASN1ClassInstanceType,
	TSASN1Base,
} from "./workdir/gluecode/TSASN1Base.js";
import { EASN1TransportEncoding } from "./workdir/gluecode/TSInvokeContext.js";
import {
	CustomInvokeProblemEnum,
	RemoteCapabilityMode,
	ROSE_REJECT_REMOTENOTCAPABLE,
} from "./workdir/gluecode/TSROSEBase.js";
import { buildRemoteModuleCapabilities } from "./workdir/gluecode/TSModuleCapabilities.js";
import type { IASN1InvokeData } from "./workdir/gluecode/TSROSEBase.js";
import type { ROSEError, ROSEInvoke, ROSEReject, ROSEResult } from "./workdir/gluecode/SNACCROSE.js";

class TestTransport extends TSASN1Base {
	public sendInvokeCount = 0;

	public constructor() {
		super(EASN1TransportEncoding.JSON, ASN1ClassInstanceType.TSASN1NodeClient);
	}

	public async sendInvoke(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		const localReject = this.tryRejectRemoteNotCapable(data.invoke);
		if (localReject)
			return localReject;
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

test("lookUpName and lookUpModuleName resolve registered handlers", () => {
	const transport = new TestTransport();
	transport.registerModuleVersion("TestModule", "1.0.0");
	transport.registerOperation(noopHandler, noopHandler as never, 100, "asnInvoke", "TestModule", 0, 0, false);

	assert.equal(transport.lookUpName(100), "asnInvoke");
	assert.equal(transport.lookUpID("asnInvoke"), 100);
	assert.equal(transport.lookUpModuleName(100), "TestModule");
});

test("enabled without snapshot does not gate sendInvoke", async () => {
	const transport = new TestTransport();
	transport.registerOperation(noopHandler, noopHandler as never, 100, "asnInvoke", "TestModule", 0, 0, false);
	transport.setRemoteCapabilityMode(RemoteCapabilityMode.Enabled);

	await transport.sendInvoke({
		invoke: createInvoke(100, "asnInvoke"),
		invokeContext: transport.getInvokeContextParams(undefined, 100, "asnInvoke", false),
		payLoad: new Uint8Array(),
	} as IASN1InvokeData);

	assert.equal(transport.sendInvokeCount, 1);
});

test("enabled with unsupported op id returns remoteNotCapable reject", async () => {
	const transport = new TestTransport();
	transport.registerOperation(noopHandler, noopHandler as never, 100, "asnInvoke", "TestModule", 0, 0, false);
	transport.applyRemoteModuleCapabilities(buildRemoteModuleCapabilities([
		{ moduleName: "TestModule", version: "1.0.0", invokeOpIds: [200] },
	]));
	transport.setRemoteCapabilityMode(RemoteCapabilityMode.Enabled);

	const result = await transport.sendInvoke({
		invoke: createInvoke(100, "asnInvoke"),
		invokeContext: transport.getInvokeContextParams(undefined, 100, "asnInvoke", false),
		payLoad: new Uint8Array(),
	} as IASN1InvokeData);

	assert.ok(result);
	assert.equal((result as ROSEReject).reject.invokeProblem, CustomInvokeProblemEnum.remoteNotCapable);
	assert.equal(CustomInvokeProblemEnum.remoteNotCapable, ROSE_REJECT_REMOTENOTCAPABLE);
	assert.equal(transport.sendInvokeCount, 0);
});

test("isSupportedOperation reflects applied snapshot", () => {
	const transport = new TestTransport();
	transport.registerOperation(noopHandler, noopHandler as never, 100, "asnInvoke", "TestModule", 0, 0, false);
	transport.applyRemoteModuleCapabilities(buildRemoteModuleCapabilities([
		{ moduleName: "TestModule", version: "1.0.0", invokeOpIds: [100] },
	]));

	assert.equal(transport.hasRemoteModuleCapabilities(), true);
	assert.equal(transport.isSupportedOperation(100), true);
	assert.equal(transport.isSupportedOperation(200), false);
});

test("clearRemoteModuleCapabilities stops gating", async () => {
	const transport = new TestTransport();
	transport.registerOperation(noopHandler, noopHandler as never, 100, "asnInvoke", "TestModule", 0, 0, false);
	transport.applyRemoteModuleCapabilities(buildRemoteModuleCapabilities([
		{ moduleName: "TestModule", version: "1.0.0", invokeOpIds: [200] },
	]));
	transport.setRemoteCapabilityMode(RemoteCapabilityMode.Enabled);
	transport.clearRemoteModuleCapabilities();

	await transport.sendInvoke({
		invoke: createInvoke(100, "asnInvoke"),
		invokeContext: transport.getInvokeContextParams(undefined, 100, "asnInvoke", false),
		payLoad: new Uint8Array(),
	} as IASN1InvokeData);

	assert.equal(transport.sendInvokeCount, 1);
});

test("disabled with snapshot does not gate sendInvoke", async () => {
	const transport = new TestTransport();
	transport.registerOperation(noopHandler, noopHandler as never, 100, "asnInvoke", "TestModule", 0, 0, false);
	transport.applyRemoteModuleCapabilities(buildRemoteModuleCapabilities([
		{ moduleName: "TestModule", version: "1.0.0", invokeOpIds: [200] },
	]));
	transport.setRemoteCapabilityMode(RemoteCapabilityMode.Disabled);

	await transport.sendInvoke({
		invoke: createInvoke(100, "asnInvoke"),
		invokeContext: transport.getInvokeContextParams(undefined, 100, "asnInvoke", false),
		payLoad: new Uint8Array(),
	} as IASN1InvokeData);

	assert.equal(transport.sendInvokeCount, 1);
});

test("enabled with supported op id sends invoke", async () => {
	const transport = new TestTransport();
	transport.registerOperation(noopHandler, noopHandler as never, 100, "asnInvoke", "TestModule", 0, 0, false);
	transport.applyRemoteModuleCapabilities(buildRemoteModuleCapabilities([
		{ moduleName: "TestModule", version: "1.0.0", invokeOpIds: [100] },
	]));
	transport.setRemoteCapabilityMode(RemoteCapabilityMode.Enabled);

	await transport.sendInvoke({
		invoke: createInvoke(100, "asnInvoke"),
		invokeContext: transport.getInvokeContextParams(undefined, 100, "asnInvoke", false),
		payLoad: new Uint8Array(),
	} as IASN1InvokeData);

	assert.equal(transport.sendInvokeCount, 1);
});
