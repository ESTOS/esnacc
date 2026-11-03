#ifndef _RoseSessionSubscriptionStore_h_
#define _RoseSessionSubscriptionStore_h_

#include "ISnaccRoseSessionSubscription.h"

#include <unordered_map>
#include <unordered_set>

/*
 * In-memory per-session subscription store.
 * Paired with TypeScript RoseSessionSubscriptionStore. A replace or clear updates
 * only that module's OPIDs in the flat lookup. Events and invokes stay separate sets.
 * Not thread-safe. Callers that share one store across threads must serialize access.
 */
class RoseSessionSubscriptionStore : public ISnaccRoseSessionSubscription
{
public:
	void ClearAllSubscriptions() override;
	void ClearSubscribedEvents(int moduleIid) override;
	void ClearSupportedInvokes(int moduleIid) override;
	void SetSubscribedEvents(int moduleIid, const std::list<int>& eventOpIds) override;
	void AddSubscribedEvent(int moduleIid, unsigned int uiEventOpId) override;
	void SetSupportedInvokes(int moduleIid, const std::list<int>& invokeOpIds) override;
	void AddSupportedInvoke(int moduleIid, unsigned int uiInvokeOpId) override;
	bool IsSubscribedEvent(unsigned int uiEventOpId) const override;
	bool IsSupportedInvoke(unsigned int uiInvokeOpId) const override;

private:
	/* Drops moduleIid from moduleOpIdsByModule and removes its previous OPIDs from flatOpIds. Unknown moduleIid is a no-op. */
	void ClearModuleOpIds(std::unordered_map<int, std::unordered_set<unsigned int>>& moduleOpIdsByModule, std::unordered_set<unsigned int>& flatOpIds, int moduleIid);

	/* Replaces moduleIid. An empty opIds list leaves the module absent. Other modules stay in both maps. */
	void ReplaceModuleOpIds(std::unordered_map<int, std::unordered_set<unsigned int>>& moduleOpIdsByModule, std::unordered_set<unsigned int>& flatOpIds, int moduleIid, const std::list<int>& opIds);

	std::unordered_map<int, std::unordered_set<unsigned int>> m_subscribedEventsByModule;
	std::unordered_set<unsigned int> m_subscribedEventOpIds;
	std::unordered_map<int, std::unordered_set<unsigned int>> m_supportedInvokesByModule;
	std::unordered_set<unsigned int> m_supportedInvokeOpIds;
};

#endif // _RoseSessionSubscriptionStore_h_
