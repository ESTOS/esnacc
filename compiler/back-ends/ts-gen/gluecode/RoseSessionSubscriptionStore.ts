import type { IRoseSessionSubscription } from "./IRoseSessionSubscription.js";

/*
 * In-memory per-session subscription store (reference implementation for TS servers).
 * Parity with UCServer ENetCtiSessionSubscriptionStore; reuse the same shape for Kotlin/Swift ports.
 */
export class RoseSessionSubscriptionStore implements IRoseSessionSubscription {
	private subscribedEventsByModule = new Map<number, Set<number>>();
	private subscribedEventOpIds = new Set<number>();
	private supportedInvokesByModule = new Map<number, Set<number>>();
	private supportedInvokeOpIds = new Set<number>();

	public clearAllSubscriptions(): void {
		this.subscribedEventsByModule.clear();
		this.subscribedEventOpIds.clear();
		this.supportedInvokesByModule.clear();
		this.supportedInvokeOpIds.clear();
	}

	public clearSubscribedEvents(moduleIid: number): void {
		this.clearModuleOpIds(this.subscribedEventsByModule, this.subscribedEventOpIds, moduleIid);
	}

	public clearSupportedInvokes(moduleIid: number): void {
		this.clearModuleOpIds(this.supportedInvokesByModule, this.supportedInvokeOpIds, moduleIid);
	}

	public setSubscribedEvents(moduleIid: number, eventOpIds: readonly number[]): void {
		this.replaceModuleOpIds(this.subscribedEventsByModule, this.subscribedEventOpIds, moduleIid, eventOpIds);
	}

	public addSubscribedEvent(moduleIid: number, eventOpId: number): void {
		this.subscribedEventsByModule.set(moduleIid, this.subscribedEventsByModule.get(moduleIid) ?? new Set<number>());
		this.subscribedEventsByModule.get(moduleIid)!.add(eventOpId);
		this.subscribedEventOpIds.add(eventOpId);
	}

	public setSupportedInvokes(moduleIid: number, invokeOpIds: readonly number[]): void {
		this.replaceModuleOpIds(this.supportedInvokesByModule, this.supportedInvokeOpIds, moduleIid, invokeOpIds);
	}

	public addSupportedInvoke(moduleIid: number, invokeOpId: number): void {
		this.supportedInvokesByModule.set(moduleIid, this.supportedInvokesByModule.get(moduleIid) ?? new Set<number>());
		this.supportedInvokesByModule.get(moduleIid)!.add(invokeOpId);
		this.supportedInvokeOpIds.add(invokeOpId);
	}

	public isSubscribedEvent(eventOpId: number): boolean {
		return this.subscribedEventOpIds.has(eventOpId);
	}

	public isSupportedInvoke(invokeOpId: number): boolean {
		return this.supportedInvokeOpIds.has(invokeOpId);
	}

	private replaceModuleOpIds(
		moduleMap: Map<number, Set<number>>,
		flatOpIds: Set<number>,
		moduleIid: number,
		opIds: readonly number[],
	): void {
		this.clearModuleOpIds(moduleMap, flatOpIds, moduleIid);
		if (opIds.length === 0)
			return;

		const moduleOpIds = new Set<number>();
		for (const opId of opIds) {
			moduleOpIds.add(opId);
			flatOpIds.add(opId);
		}
		moduleMap.set(moduleIid, moduleOpIds);
	}

	private clearModuleOpIds(moduleMap: Map<number, Set<number>>, flatOpIds: Set<number>, moduleIid: number): void {
		const previous = moduleMap.get(moduleIid);
		if (!previous)
			return;
		for (const opId of previous)
			flatOpIds.delete(opId);
		moduleMap.delete(moduleIid);
	}
}
