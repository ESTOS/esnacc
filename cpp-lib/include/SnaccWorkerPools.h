#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

class SnaccROSEBase;

namespace SNACC
{
	class ROSEMessage;
}

/*! What a worker does when a handler throws something other than SnaccException.
	SnaccException stays a mistyped-argument reject and does not retire the thread. */
enum class SnaccWorkerFaultPolicy
{
	/*! Swallow the exception and let this thread take the next item. */
	KeepWorker,
	/*! End this thread. A new thread is started when the queue still has work, or on the next enqueue. */
	RetireWorker
};

/*! One named worker pool inside a shared SnaccWorkerPools registry.
	operationIds are served by this pool. Each operationID may appear once in the whole registry.
	Every stub that borrows the registry sees the same assignment. */
struct SnaccWorkerPoolConfig
{
	std::string name{};
	/*! Upper bound on live worker threads. Values below 1 are treated as 1. */
	unsigned int maxThreads{1};
	/*! How long a thread above the floor waits on an empty queue before it exits.
		0 keeps every started thread until Shutdown. The floor thread ignores this and stays. */
	unsigned int idleTimeoutMs{30000};
	/*! Waiting items plus items in a handler. 0 means no depth limit. */
	unsigned int maxElements{};
	/*! Thread reaction when a handler throws past OnInvoke. Default keeps the worker. */
	SnaccWorkerFaultPolicy faultPolicy{SnaccWorkerFaultPolicy::KeepWorker};
	std::vector<unsigned int> operationIds{};
};

/*! How a pool worker finishes one queued inbound message.
	Run decodes and calls the handler. ShutdownReject and DropExpired do not. */
enum class SnaccPooledInboundAction
{
	Run,
	ShutdownReject,
	DropExpired
};

/*! Callbacks the stub installs when it queues an invoke.
	The pool calls them on a worker, or on the thread that cancels that stub's queue.
	complete takes ownership of the ROSEMessage pointer. processingAllowed is the session gate. */
struct SnaccPooledInboundCallbacks
{
	std::function<bool()> processingAllowed{};
	std::function<void(SNACC::ROSEMessage*, unsigned long, SnaccPooledInboundAction)> complete{};
};

/*! Result of offering one inbound invoke to a named worker pool. */
enum class SnaccWorkerEnqueueResult
{
	/*! Pool name is unknown. The caller runs the invoke on the calling thread. */
	NotPooled,
	/*! The pool owns the message and will run it on a worker. */
	Queued,
	/*! Depth is exhausted. The caller rejects or drops without a handler. */
	RejectedFull,
	/*! The pool is shutting down. The caller sends the shutdown reject. */
	PoolStopped
};

struct SnaccWorkerPoolState;

/*! Process-wide worker pools, owned once the way a listener owns SnaccRoseOperationLookup.
	The owner calls Configure at startup and keeps this object for the process lifetime.
	Each SnaccROSEBase borrows it through SetWorkerPools. Stubs share the threads and the
	operationID assignment. Pause or destruction of one stub rejects only that stub's queued
	items and waits for that stub's handlers. Shutdown rejects every stub and joins the workers. */
class SnaccWorkerPools
{
public:
	SnaccWorkerPools();
	~SnaccWorkerPools();

	SnaccWorkerPools(const SnaccWorkerPools&) = delete;
	SnaccWorkerPools& operator=(const SnaccWorkerPools&) = delete;

	/*! Replaces the shared pool set and starts one floor thread per pool.
		Returns false and keeps the previous set when an operationID is listed twice.
		A repeated pool name is skipped and its operation ids are not registered. */
	bool Configure(const std::vector<SnaccWorkerPoolConfig>& pools);
	/*! Rejects queued work for every stub, drops replies of handlers already running, and joins workers. */
	void Shutdown();
	/*! Pool name assigned to uiOperationId, or empty when the id is not pooled. */
	std::string PoolForOperation(unsigned int uiOperationId) const;
	/*! Live worker threads for poolName, including the one that waits while the queue is empty.
		0 when the name is not in the current set. */
	unsigned int LiveThreads(const std::string& poolName) const;

	/*! Queues pMessage on poolName. On Queued, pMessage and callbacks are taken. Otherwise both stay with the caller. */
	SnaccWorkerEnqueueResult Enqueue(const std::string& poolName, SnaccROSEBase& stub, std::unique_ptr<SNACC::ROSEMessage>& pMessage, unsigned long ulMessageSize, SnaccPooledInboundCallbacks callbacks);

	/*! Drops this stub's queued items and waits until its in-flight handlers return.
		Other stubs on this registry stay queued. The dropped items get the shutdown reject. */
	void RejectQueuedAndWait(SnaccROSEBase& stub);

	/*! True on a pool worker whose pool is stopping. The handler reply must not be sent. */
	static bool CurrentPoolIsStopping();

private:
	/*! Starts the floor thread for a pool that has just been configured. Caller does not hold the pool mutex. */
	void StartFloorWorker(const std::shared_ptr<SnaccWorkerPoolState>& pool);
	/*! Starts one worker when every live thread is busy and the queue is not empty. Caller holds the pool mutex. */
	void EnsureWorker(const std::shared_ptr<SnaccWorkerPoolState>& pool);
	/*! Detaches this worker after an idle timeout when another thread is still in the pool.
		Caller holds the pool mutex. Returns false when this thread is the floor and must stay. */
	bool ExitIdleWorker(const std::shared_ptr<SnaccWorkerPoolState>& pool);
	/*! Pool thread body. Invokes the callbacks stored with each queued message. */
	void WorkerMain(const std::shared_ptr<SnaccWorkerPoolState>& pool);
	/*! Rejects whatever is still queued, then joins the workers. */
	void JoinPool(const std::shared_ptr<SnaccWorkerPoolState>& pool);
	/*! Tracks how many handlers of one stub are inside a pool worker. delta is +1 or -1. */
	void NoteInflight(SnaccROSEBase* stub, int delta);

	struct State;
	std::unique_ptr<State> m_state{};
};
