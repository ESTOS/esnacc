// Run: npx tsx compiler/back-ends/ts-gen/gluecode/TSASN1Base.registry.test.ts
import assert from "node:assert/strict";
import test from "node:test";
import {
	ASN1ClassInstanceType,
	TSASN1Base,
} from "./TSASN1Base.js";
import { EASN1TransportEncoding } from "./TSInvokeContext.js";
import type { IASN1InvokeData } from "./TSROSEBase.js";
import type { ROSEError, ROSEReject, ROSEResult } from "./SNACCROSE.js";

class TestTransport extends TSASN1Base {
	public constructor() {
		super(EASN1TransportEncoding.JSON, ASN1ClassInstanceType.TSASN1NodeClient);
	}

	public sendInvoke(_data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		return Promise.resolve(undefined);
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

test("registerOperation metadata appears in getLoadedModules", () => {
	const transport = new TestTransport();
	transport.registerModuleVersion("TestModule", "20240101.0.20240506");
	transport.registerOperation(noopHandler, noopHandler as never, 100, "asnInvoke", "TestModule", 1714968000, 0, false);
	transport.registerOperation(noopHandler, noopHandler as never, 200, "asnEvent", "TestModule", 0, 1715054400, true);

	const modules = transport.getLoadedModules();
	assert.equal(modules.size, 1);
	const module = modules.get("TestModule");
	assert.ok(module);
	assert.equal(module.version, "20240101.0.20240506");
	assert.equal(module.invokes.get(100)?.addedUnix, 1714968000);
	assert.equal(module.invokes.get(100)?.deprecatedUnix, 0);
	assert.equal(module.invokes.get(100)?.opName, "asnInvoke");
	assert.equal(module.events.get(200)?.deprecatedUnix, 1715054400);
	assert.equal(module.events.get(200)?.opName, "asnEvent");

	transport.unregisterModule("TestModule");
	assert.equal(transport.getLoadedModules().size, 0);
});

test("separate stub instances keep separate registries", () => {
	const transportA = new TestTransport();
	const transportB = new TestTransport();

	transportA.registerModuleVersion("ModuleA", "1.0.1");
	transportA.registerOperation(noopHandler, noopHandler as never, 10, "opA", "ModuleA", 0, 0, false);

	transportB.registerModuleVersion("ModuleB", "2.0.2");
	transportB.registerOperation(noopHandler, noopHandler as never, 20, "opB", "ModuleB", 0, 0, true);

	assert.equal(transportA.getLoadedModules().size, 1);
	assert.equal(transportB.getLoadedModules().size, 1);
	assert.ok(transportA.getLoadedModules().has("ModuleA"));
	assert.ok(!transportA.getLoadedModules().has("ModuleB"));
});

test("lookUpName lookUpID and lookUpModuleName resolve registered operations", () => {
	const transport = new TestTransport();
	transport.registerModuleVersion("TestModule", "1.0.0");
	transport.registerOperation(noopHandler, noopHandler as never, 100, "asnInvoke", "TestModule", 0, 0, false);

	assert.equal(transport.lookUpName(100), "asnInvoke");
	assert.equal(transport.lookUpID("asnInvoke"), 100);
	assert.equal(transport.lookUpModuleName(100), "TestModule");
	assert.equal(transport.lookUpModuleName(999), undefined);
});
