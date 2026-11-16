#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

// Upper bounds in microseconds. Each bound is inclusive.
// The last bound takes every observation. Its bucket count equals m_ui64Count.
inline constexpr std::array<std::uint64_t, 12> kLatencyBucketsUs{100, 500, 1'000, 5'000, 10'000, 50'000, 100'000, 500'000, 1'000'000, 5'000'000, 30'000'000, UINT64_MAX};

/*! Cumulative latency in microseconds.
	m_buckets[i] is how many observations were at or below kLatencyBucketsUs[i].
	One observation increments every bucket whose bound it meets, then adds to m_ui64SumUs and m_ui64Count.
	The last bucket therefore stays equal to m_ui64Count.
	The mean is the growth of m_ui64SumUs divided by the growth of m_ui64Count. */
class LatencyHistogram
{
public:
	/*! Cumulative counts, one per kLatencyBucketsUs bound. */
	std::array<std::uint64_t, kLatencyBucketsUs.size()> m_buckets{};
	/*! Sum of the observations, in microseconds. */
	std::uint64_t m_ui64SumUs{};
	/*! How many observations are in m_ui64SumUs and in the last bucket. */
	std::uint64_t m_ui64Count{};
};

/*! Counters. They only rise.
	A message that has left the pool is in exactly one outcome.
	A pool that cannot produce an outcome leaves that counter at 0. That 0 means the outcome cannot occur there, not that it happened to be absent.
	The socket pool cannot produce m_ui64TimedOut or m_ui64RejectedShuttingDown.
	The ROSE queue can produce every outcome. */
class ThreadPoolCounters
{
public:
	/*! Every message that arrives. */
	std::uint64_t m_ui64Incoming{};
	/*! The handler ran and sent a result, neither an error nor a reject. Each one adds exactly one m_run observation. */
	std::uint64_t m_ui64Success{};
	/*! The handler ran and sent an error, not a reject. Each one adds exactly one m_run observation. */
	std::uint64_t m_ui64HandlerError{};
	/*! A check or the handler rejected. Argument, operation, authentication, or an unreadable argument.
		A check before the handler adds no m_run observation. A handler that ran and then rejected adds one.
		Requests the handler rejected are m_run.m_ui64Count minus m_ui64Success minus m_ui64HandlerError minus m_ui64ReplyDiscarded.
		The rest of this counter are the pre-check rejects. */
	std::uint64_t m_ui64HandlerReject{};
	/*! The queue or the last socket thread was full. The message never entered the queue. */
	std::uint64_t m_ui64RejectedFull{};
	/*! The pool is stopping. At the door the message never entered the queue. From the queue it already had.
		The socket pool leaves this at 0. */
	std::uint64_t m_ui64RejectedShuttingDown{};
	/*! This connection closed. The pool keeps serving the others. */
	std::uint64_t m_ui64RejectedConnectionClosed{};
	/*! The deadline passed before the handler began. The socket pool leaves this at 0. */
	std::uint64_t m_ui64TimedOut{};
	/*! The handler had finished and the pool then stopped. The reply is dropped.
		This is the only outcome for that message. It still adds one m_run observation. */
	std::uint64_t m_ui64ReplyDiscarded{};

	/*! How long the handler ran.
		Each m_ui64Success, each m_ui64HandlerError, and each m_ui64ReplyDiscarded adds one observation.
		A m_ui64HandlerReject adds one only when the handler ran. */
	LatencyHistogram m_run{};
};

/*! State at this moment. These values can fall. */
class ThreadPoolState
{
public:
	/*! Threads waiting for the next request. */
	unsigned int m_uiIdleThreads{};
	/*! A thread is inside a request right now. */
	unsigned int m_uiRunning{};
	/*! How long the longest running request has been on a thread, in microseconds. Empty when none is running. */
	std::optional<std::uint64_t> m_ui64OldestRunningUs{};
};

/*! Health sample shared by the socket pool and the ROSE queue.
	The caller owns the copy. A later sample does not change this one.
	m_state.m_uiIdleThreads plus m_state.m_uiRunning is the threads inside the request loop now, at most m_uiMaxThreads.
	On the ROSE queue, m_counters.m_ui64Incoming equals requests still here plus every outcome. */
class ThreadPoolHealth
{
public:
	/*! Name of this pool. */
	std::string m_strName{};
	/*! Upper bound on threads. */
	unsigned int m_uiMaxThreads{};
	/*! Counters. They only rise. */
	ThreadPoolCounters m_counters{};
	/*! State at this moment. */
	ThreadPoolState m_state{};
};
