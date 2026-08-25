/*
 * Per-session ROSE subscription contract (parity with C++ ISnaccRoseSessionSubscription).
 *
 * Naming map for future backends:
 *   C++  clearAllSubscriptions / setSubscribedEvents / isSubscribedEvent
 *   TS   clearAllSubscriptions / setSubscribedEvents / isSubscribedEvent
 *
 * Server-side transports must override all methods. TSASN1Base defaults call snaccAssertFail.
 * When OperationBlockPolicy is BlockUnsupportedOperations and session state is marked,
 * gluecode blocks outbound traffic via isOperationBlocked at handleEvent / handleInvoke.
 */
export interface IRoseSessionSubscription {
	clearAllSubscriptions(): void;
	clearSubscribedEvents(moduleIid: number): void;
	clearSupportedInvokes(moduleIid: number): void;
	setSubscribedEvents(moduleIid: number, eventOpIds: readonly number[]): void;
	addSubscribedEvent(moduleIid: number, eventOpId: number): void;
	setSupportedInvokes(moduleIid: number, invokeOpIds: readonly number[]): void;
	addSupportedInvoke(moduleIid: number, invokeOpId: number): void;
	isSubscribedEvent(eventOpId: number): boolean;
	isSupportedInvoke(invokeOpId: number): boolean;
}
