// Small typing helpers for glue tests (generated ASN.1 stubs use strict ctor signatures).
import * as ENetUC_Settings_Manager from "../stub/ENetUC_Settings_Manager.js";
import type { ROSEInvoke } from "../stub/SNACCROSE.js";
import { ROSEInvoke as ROSEInvokeClass } from "../stub/SNACCROSE.js";

/** Matches handleInvoke's result/argument template param (instance `type` used in decode diagnostics). */
export type RoseHandleInvokeTemplate = object & { readonly type: string };

/** Builds a ROSEInvoke from a partial wire payload (generated ctor requires ROSEInvoke). */
export function roseInvoke(fields: Partial<ROSEInvoke> & Pick<ROSEInvoke, "invokeID" | "operationID">): ROSEInvoke {
	return new ROSEInvokeClass(fields as ROSEInvoke);
}

/** Attaches instance `type` from the generated class static (handleInvoke logs `resultObj.type`). */
export function roseHandleInvokeTemplate<T extends object>(obj: T, typeName?: string): RoseHandleInvokeTemplate {
	const ctor = obj.constructor as { type?: string };
	const name = typeName ?? ctor.type ?? obj.constructor.name;
	return Object.assign(obj, { type: name });
}

/** Empty result template for handleInvoke decode path (generated ctor requires AsnGetSettingsResult). */
export function asnGetSettingsResultTemplate(): RoseHandleInvokeTemplate {
	return roseHandleInvokeTemplate(
		new ENetUC_Settings_Manager.AsnGetSettingsResult({
			settings: new ENetUC_Settings_Manager.AsnSomeSettings(),
		} as ENetUC_Settings_Manager.AsnGetSettingsResult),
	);
}

/** Normalizes encoded transport payload for TSASN1Base.receive (JSON string or bytes). */
export function toWireReceivePayload(payLoad: string | Uint8Array | object): object | Uint8Array {
	if (typeof payLoad === "string") {
		return JSON.parse(payLoad) as object;
	}
	return payLoad;
}
