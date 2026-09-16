// Shared stub transports for glue-level runtime tests (parity with cpp-lib sample_runtime_harness patterns).
import type { ROSEError, ROSEReject, ROSEResult } from "../stub/SNACCROSE.js";
import { ASN1ClassInstanceType, TSASN1Base } from "../stub/TSASN1Base.js";
import { EASN1TransportEncoding } from "../stub/TSInvokeContext.js";
import type { IASN1InvokeData } from "../stub/TSROSEBase.js";
import { ReceiveInvokeContext, ROSEBase } from "../stub/TSROSEBase.js";

/** Records the last outbound invoke passed to sendInvoke. */
export class CapturingClientTransport extends TSASN1Base {
	public lastInvokeData?: IASN1InvokeData;

	public constructor() {
		super(EASN1TransportEncoding.JSON, ASN1ClassInstanceType.TSASN1NodeClient);
	}

	public async sendInvoke(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		this.lastInvokeData = data;
		return undefined;
	}

	public sendEvent(data: IASN1InvokeData): undefined | boolean {
		this.lastInvokeData = data;
		void this.sendInvoke(data);
		return undefined;
	}

	public sendEventSync(_data: IASN1InvokeData): boolean {
		return true;
	}

	public getSessionID(): string | undefined {
		return undefined;
	}
}

/** Minimal ROSE module for encode-path tests. */
export class TestRose extends ROSEBase {
	public readonly logFilter: string[] = [];

	public constructor(transport: CapturingClientTransport) {
		super(transport, false);
	}

	public getLogData(): { className: string } {
		return { className: "TestRose" };
	}

	public getNameForOperationID(_id: number): string | undefined {
		return undefined;
	}

	public getIDForOperationName(_name: string): number | undefined {
		return undefined;
	}

	public async onInvoke(): Promise<undefined> {
		return undefined;
	}
}

/** Server transport that records the invoke context seen by the registered handler. */
export class RecordingServerTransport extends TSASN1Base {
	public handlerContext?: ReceiveInvokeContext;

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

export const noopInvokeHandler = {
	getNameForOperationID: (): string | undefined => undefined,
	getIDForOperationName: (): number | undefined => undefined,
	onInvoke: async () => undefined,
};
