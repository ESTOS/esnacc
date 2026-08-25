#ifndef _ISnaccRoseSessionSubscription_h_
#define _ISnaccRoseSessionSubscription_h_

#include <list>

/*
 * Per-session ROSE subscription contract for SnaccROSESender and SnaccROSEComponent.
 *
 * Server-side senders must override all methods. SnaccROSESender base implementations assert
 * in debug builds when called without an override.
 *
 * When SnaccOperationBlockPolicy is BlockUnsupportedOperations and session state is marked,
 * gluecode blocks outbound traffic via IsOperationBlocked.
 */
class ISnaccRoseSessionSubscription
{
public:
	virtual ~ISnaccRoseSessionSubscription() = default;

	/* Clears all subscribed events and supported invokes for this session (e.g. on disconnect). */
	virtual void ClearAllSubscriptions() = 0;

	/* Removes subscribed server-to-client event OPIDs for moduleIid only. */
	virtual void ClearSubscribedEvents(int moduleIid) = 0;

	/* Removes supported server-to-client invoke OPIDs for moduleIid only. */
	virtual void ClearSupportedInvokes(int moduleIid) = 0;

	/* Replaces the subscribed event OPID set for moduleIid and refreshes the flat dispatch lookup. */
	virtual void SetSubscribedEvents(int moduleIid, const std::list<int>& eventOpIds) = 0;

	/* Adds one subscribed event OPID for moduleIid without replacing other module entries. */
	virtual void AddSubscribedEvent(int moduleIid, unsigned int uiEventOpId) = 0;

	/* Replaces the supported server-to-client invoke OPID set for moduleIid. */
	virtual void SetSupportedInvokes(int moduleIid, const std::list<int>& invokeOpIds) = 0;

	/* Adds one supported invoke OPID for moduleIid without replacing other module entries. */
	virtual void AddSupportedInvoke(int moduleIid, unsigned int uiInvokeOpId) = 0;

	/* True when uiEventOpId is in the effective subscribed-event set for this session. */
	virtual bool IsSubscribedEvent(unsigned int uiEventOpId) const = 0;

	/* True when uiInvokeOpId is in the supported server-to-client invoke set for this session. */
	virtual bool IsSupportedInvoke(unsigned int uiInvokeOpId) const = 0;
};

#endif // _ISnaccRoseSessionSubscription_h_
