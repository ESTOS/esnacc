#pragma once

#include "ThreadPoolHealth.h"

#include <cstdint>
#include <optional>

/*! ROSE queue health. The shared socket-pool fields, plus what only this pool records.
	The caller owns the copy. A later sample does not change this one.
	m_counters.m_ui64Incoming equals m_uiWaiting plus m_state.m_uiRunning plus every outcome in m_counters.
	m_ui64ThreadsStarted and m_ui64ThreadsStopped count threads, not requests.
	A thread is counted started before it reaches the wait. m_ui64ThreadsStarted minus m_ui64ThreadsStopped is at least idle plus running, and greater until that thread enters the loop.
	The copy is taken under the pool lock. */
class ROSEWorkerPoolHealth : public ThreadPoolHealth
{
public:
	/*! How many requests may wait and run at once. Empty when the queue has no cap. */
	std::optional<unsigned int> m_uiMaxElements{};
	/*! Threads this pool has started. A counter. */
	std::uint64_t m_ui64ThreadsStarted{};
	/*! Threads that have left this pool. A counter. */
	std::uint64_t m_ui64ThreadsStopped{};
	/*! Requests waiting for a thread. State. */
	unsigned int m_uiWaiting{};
	/*! How long the oldest waiting request has been waiting, in microseconds. Empty when none is waiting. */
	std::optional<std::uint64_t> m_ui64OldestWaitingUs{};
	/*! Wait of every request that left the queue, whichever outcome followed.
		m_counters.m_ui64RejectedFull never entered the queue, so it is absent here.
		A shutdown refusal at the door is the same. A shutdown that drains the queue is included.
		Because m_counters.m_ui64RejectedShuttingDown mixes both moments, wait count plus m_uiWaiting is not incoming minus the door refusals. */
	LatencyHistogram m_wait{};
};
