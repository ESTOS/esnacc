// Small typing helpers for glue tests (generated ASN.1 stubs use strict ctor signatures).
import type { ROSEInvoke } from "../stub/SNACCROSE.js";
import { ROSEInvoke as ROSEInvokeClass } from "../stub/SNACCROSE.js";

/** Builds a ROSEInvoke from a partial wire payload (generated ctor requires ROSEInvoke). */
export function roseInvoke(fields: Partial<ROSEInvoke> & Pick<ROSEInvoke, "invokeID" | "operationID">): ROSEInvoke {
	return new ROSEInvokeClass(fields as ROSEInvoke);
}

/** Normalizes encoded transport payload for TSASN1Base.receive (JSON string or bytes). */
export function toWireReceivePayload(payLoad: string | Uint8Array | object): object | Uint8Array {
	if (typeof payLoad === "string") {
		return JSON.parse(payLoad) as object;
	}
	return payLoad;
}
