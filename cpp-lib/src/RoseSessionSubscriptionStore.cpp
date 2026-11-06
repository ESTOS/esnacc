#include "../include/RoseSessionSubscriptionStore.h"

#include <mutex>
#include <shared_mutex>

/*
 * Removes one module from the per-module map and from the flat dispatch set.
 * Called by Clear* and by ReplaceModuleOpIds before the new OPID list is inserted.
 * Caller holds the unique lock.
 * An unknown moduleIid leaves both containers unchanged. OPIDs are not reference-counted:
 * the same OPID stored on a second module would disappear from the flat set with this module.
 */
void RoseSessionSubscriptionStore::ClearModuleOpIds(std::unordered_map<int, std::unordered_set<unsigned int>>& moduleOpIdsByModule, std::unordered_set<unsigned int>& flatOpIds, int moduleIid)
{
	const auto previousModuleIt = moduleOpIdsByModule.find(moduleIid);
	if (previousModuleIt == moduleOpIdsByModule.end())
		return;

	for (const unsigned int uiPreviousOpId : previousModuleIt->second)
		flatOpIds.erase(uiPreviousOpId);

	moduleOpIdsByModule.erase(previousModuleIt);
}

/*
 * Replaces the OPID set of one module and rebuilds its contribution to the flat lookup.
 * Called by SetSubscribedEvents and SetSupportedInvokes while the unique lock is held. An empty list clears the module.
 * Other modules in the same maps are left in place.
 */
void RoseSessionSubscriptionStore::ReplaceModuleOpIds(std::unordered_map<int, std::unordered_set<unsigned int>>& moduleOpIdsByModule, std::unordered_set<unsigned int>& flatOpIds, int moduleIid, const std::list<int>& opIds)
{
	ClearModuleOpIds(moduleOpIdsByModule, flatOpIds, moduleIid);
	if (opIds.empty())
		return;

	std::unordered_set<unsigned int> moduleOpIds;
	for (const int opId : opIds)
	{
		const unsigned int uiOpId = static_cast<unsigned int>(opId);
		moduleOpIds.insert(uiOpId);
		flatOpIds.insert(uiOpId);
	}

	moduleOpIdsByModule.emplace(moduleIid, std::move(moduleOpIds));
}

/* Clears events and invokes for the whole session. Used on disconnect. Takes the unique lock. */
void RoseSessionSubscriptionStore::ClearAllSubscriptions()
{
	const std::unique_lock lock(m_lock);
	m_subscribedEventsByModule.clear();
	m_subscribedEventOpIds.clear();
	m_supportedInvokesByModule.clear();
	m_supportedInvokeOpIds.clear();
}

/* Drops subscribed events for moduleIid. Supported invokes of that module stay. Takes the unique lock. */
void RoseSessionSubscriptionStore::ClearSubscribedEvents(int moduleIid)
{
	const std::unique_lock lock(m_lock);
	ClearModuleOpIds(m_subscribedEventsByModule, m_subscribedEventOpIds, moduleIid);
}

/* Drops supported invokes for moduleIid. Subscribed events of that module stay. Takes the unique lock. */
void RoseSessionSubscriptionStore::ClearSupportedInvokes(int moduleIid)
{
	const std::unique_lock lock(m_lock);
	ClearModuleOpIds(m_supportedInvokesByModule, m_supportedInvokeOpIds, moduleIid);
}

/* Replaces the subscribed event OPIDs of moduleIid. Empty eventOpIds clears that module only. Takes the unique lock. */
void RoseSessionSubscriptionStore::SetSubscribedEvents(int moduleIid, const std::list<int>& eventOpIds)
{
	const std::unique_lock lock(m_lock);
	ReplaceModuleOpIds(m_subscribedEventsByModule, m_subscribedEventOpIds, moduleIid, eventOpIds);
}

/* Adds one event OPID for moduleIid without removing other events of this module or of other modules. Takes the unique lock. */
void RoseSessionSubscriptionStore::AddSubscribedEvent(int moduleIid, unsigned int uiEventOpId)
{
	const std::unique_lock lock(m_lock);
	m_subscribedEventsByModule[moduleIid].insert(uiEventOpId);
	m_subscribedEventOpIds.insert(uiEventOpId);
}

/* Replaces the supported invoke OPIDs of moduleIid. Empty invokeOpIds clears that module only. Takes the unique lock. */
void RoseSessionSubscriptionStore::SetSupportedInvokes(int moduleIid, const std::list<int>& invokeOpIds)
{
	const std::unique_lock lock(m_lock);
	ReplaceModuleOpIds(m_supportedInvokesByModule, m_supportedInvokeOpIds, moduleIid, invokeOpIds);
}

/* Adds one invoke OPID for moduleIid without removing other invokes of this module or of other modules. Takes the unique lock. */
void RoseSessionSubscriptionStore::AddSupportedInvoke(int moduleIid, unsigned int uiInvokeOpId)
{
	const std::unique_lock lock(m_lock);
	m_supportedInvokesByModule[moduleIid].insert(uiInvokeOpId);
	m_supportedInvokeOpIds.insert(uiInvokeOpId);
}

/* True when uiEventOpId is in the flat event set, regardless of which module registered it. Takes a shared lock. */
bool RoseSessionSubscriptionStore::IsSubscribedEvent(unsigned int uiEventOpId) const
{
	const std::shared_lock lock(m_lock);
	return m_subscribedEventOpIds.find(uiEventOpId) != m_subscribedEventOpIds.end();
}

/* True when uiInvokeOpId is in the flat invoke set, regardless of which module registered it. Takes a shared lock. */
bool RoseSessionSubscriptionStore::IsSupportedInvoke(unsigned int uiInvokeOpId) const
{
	const std::shared_lock lock(m_lock);
	return m_supportedInvokeOpIds.find(uiInvokeOpId) != m_supportedInvokeOpIds.end();
}
