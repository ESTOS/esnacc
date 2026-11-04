#ifndef _RoseSessionSubscriptionStore_h_
#define _RoseSessionSubscriptionStore_h_

#include "ISnaccRoseSessionSubscription.h"

#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

/*
 * In-memory per-session subscription store.
 * Paired with TypeScript RoseSessionSubscriptionStore. A replace or clear updates
 * only that module's OPIDs in the flat lookup. Events and invokes stay separate sets.
 * Many readers, one writer (std::shared_mutex). The TypeScript store has the same
 * set rules and no lock; its event loop is the serializer.
 * ClearModuleOpIds and ReplaceModuleOpIds must be called with the unique lock held.
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

	mutable std::shared_mutex m_lock;
	std::unordered_map<int, std::unordered_set<unsigned int>> m_subscribedEventsByModule;
	std::unordered_set<unsigned int> m_subscribedEventOpIds;
	std::unordered_map<int, std::unordered_set<unsigned int>> m_supportedInvokesByModule;
	std::unordered_set<unsigned int> m_supportedInvokeOpIds;
};

#endif // _RoseSessionSubscriptionStore_h_
