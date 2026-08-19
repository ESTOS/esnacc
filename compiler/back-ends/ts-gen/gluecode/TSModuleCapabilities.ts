// Run: npx tsx compiler/back-ends/ts-gen/gluecode/TSModuleCapabilities.ts
import type { ILoadedModuleInfo, IOpVersionInfo } from "./TSROSEBase.js";

/**
 * One module entry extracted from asnNegotiateInterface (or equivalent).
 */
export interface IRemoteModuleDetailInput {
	moduleName: string;
	version: string;
	invokeOpIds?: readonly number[];
	eventOpIds?: readonly number[];
}

/**
 * ASN.1 module detail shape used by buildRemoteModuleCapabilitiesFromAsn().
 */
export interface IAsnModuleDetailLike {
	u8sName: string;
	u8sASN1ModuleVersion: string;
	iOperations?: readonly number[];
	iEvents?: readonly number[];
}

function applyOpIds(opIds: readonly number[] | undefined, target: Map<number, IOpVersionInfo>): void {
	if (!opIds)
		return;
	for (const opId of opIds) {
		if (opId === 0)
			continue;
		target.set(opId, { addedUnix: 0, deprecatedUnix: 0 });
	}
}

/**
 * Merges one module detail into the remote capability map (creates the module entry when missing).
 */
export function applyModuleDetailToRemoteCapabilities(
	moduleName: string,
	version: string,
	invokeOpIds: readonly number[] | undefined,
	eventOpIds: readonly number[] | undefined,
	inOutRemote: Map<string, ILoadedModuleInfo>,
): void {
	let module = inOutRemote.get(moduleName);
	if (!module) {
		module = {
			moduleName,
			version,
			invokes: new Map<number, IOpVersionInfo>(),
			events: new Map<number, IOpVersionInfo>(),
		};
		inOutRemote.set(moduleName, module);
	} else {
		module.version = version;
	}

	applyOpIds(invokeOpIds, module.invokes);
	applyOpIds(eventOpIds, module.events);
}

/**
 * Builds a remote capability map from module detail inputs.
 */
export function buildRemoteModuleCapabilities(
	details: readonly IRemoteModuleDetailInput[],
): Map<string, ILoadedModuleInfo> {
	const remote = new Map<string, ILoadedModuleInfo>();
	for (const detail of details) {
		applyModuleDetailToRemoteCapabilities(
			detail.moduleName,
			detail.version,
			detail.invokeOpIds,
			detail.eventOpIds,
			remote,
		);
	}
	return remote;
}

/**
 * Builds a remote capability map from asnNegotiateInterface module-details payloads.
 */
export function buildRemoteModuleCapabilitiesFromAsn(
	moduleDetails: readonly IAsnModuleDetailLike[],
): Map<string, ILoadedModuleInfo> {
	return buildRemoteModuleCapabilities(
		moduleDetails.map((detail) => ({
			moduleName: detail.u8sName,
			version: detail.u8sASN1ModuleVersion,
			invokeOpIds: detail.iOperations,
			eventOpIds: detail.iEvents,
		})),
	);
}
