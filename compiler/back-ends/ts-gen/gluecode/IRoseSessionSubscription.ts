/*
 * Per-session ROSE subscription contract (parity with C++ ISnaccRoseSessionSubscription).
 *
 * Naming map for future backends:
 *   C++  ClearAllSubscriptions / SetSubscribedEvents / IsSubscribedEvent
 *   TS   clearAllSubscriptions / setSubscribedEvents / isSubscribedEvent
 *
 * Server-side transports that own a store must override all methods.
 * TSASN1Base write defaults call snaccAssertFail. The two queries allow every OPID until a store is installed.
 * When OperationBlockPolicy is BlockUnsupportedOperations, gluecode blocks outbound
 * traffic that is not in this session set via isOperationBlocked. An empty set blocks.
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
