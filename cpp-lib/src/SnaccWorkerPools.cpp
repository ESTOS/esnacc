#include "../include/SnaccWorkerPools.h"
#include "../include/SNACCROSE.h"
#include "../include/SnaccROSEBase.h"
#include "snacc-assert.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// One inbound ROSE message waiting for or owned by a pool worker.
struct WorkItem
{
	// Stub that queued the item. Pause matches on this pointer. The stub must live until the callback returns.
	SnaccROSEBase* stub{};
	std::unique_ptr<SNACC::ROSEMessage> message{};
	unsigned long size{};
	std::optional<std::chrono::steady_clock::time_point> deadline{};
	// Session gate and completion, installed by the stub when it queued this message.
	SnaccPooledInboundCallbacks callbacks{};
};

// Mutable state of one named pool. Shared by the process-wide registry and its workers.
struct SnaccWorkerPoolState
{
	std::string name{};
	unsigned int maxThreads{1};
	// Waiting items plus items inside a handler. 0 means the queue is not capped.
	unsigned int maxElements{};
	std::mutex mutex{};
	std::condition_variable cv{};
	std::deque<WorkItem> queue{};
	// Handlers currently inside DispatchQueuedItem. Counts toward maxElements.
	unsigned int running{};
	// Workers blocked in the wait, available to take the next item without starting a thread.
	unsigned int idleWorkers{};
	// Waiting workers plus workers inside a handler. The floor thread is included. Idle-exited threads are not.
	unsigned int liveWorkers{};
	std::chrono::milliseconds idleTimeout{30000};
	SnaccWorkerFaultPolicy faultPolicy{SnaccWorkerFaultPolicy::KeepWorker};
	// Set by JoinPool. Workers finish the item they already popped, then leave. New enqueues are refused.
	std::atomic<bool> stopping{};
	// C++20 workers. An idle exit detaches, because a jthread cannot join itself. Shutdown joins the ones that remain.
	std::vector<std::jthread> threads{};
};

// Registry-wide maps. Pool queues and counters live on SnaccWorkerPoolState, under that pool's mutex.
struct SnaccWorkerPools::State
{
	std::mutex registryMutex{};
	std::vector<std::shared_ptr<SnaccWorkerPoolState>> pools{};
	std::unordered_map<std::string, std::shared_ptr<SnaccWorkerPoolState>> poolsByName{};
	// operationID to the one pool allowed to run it. Empty means the stub runs that invoke itself.
	std::unordered_map<unsigned int, std::shared_ptr<SnaccWorkerPoolState>> operationPools{};
	std::mutex inflightMutex{};
	std::condition_variable inflightCv{};
	// Handlers that have left the queue and not yet returned, per stub. Pause waits until the entry is gone.
	std::unordered_map<SnaccROSEBase*, unsigned int> inflight{};
};

namespace
{
	// Set for the duration of WorkerMain. Null on every thread that is not inside a pool worker.
	thread_local SnaccWorkerPoolState* t_currentPool{};

	// One thread stays for the life of the pool so a crash dump still shows the pool name.
	constexpr unsigned int kMinLiveWorkers = 1;

	// Publishes `<pool>-<thread id>` for a crash dump when the worker starts.
	// C++20 has no thread-naming function. Windows uses SetThreadDescription. Other platforms leave the OS name unset.
	void NameWorkerThread(const std::string& poolName)
	{
#ifdef _WIN32
		// The suffix is the C++ thread id, assigned when the worker runs, not a slot reused from 0.
		// SetThreadDescription is looked up because older Windows does not export it. A missing export leaves the thread unnamed.
		std::ostringstream oss;
		oss << poolName << "-" << std::this_thread::get_id();
		const std::string name = oss.str();
		std::wstring wide(name.begin(), name.end());
		using SetThreadDescriptionFn = HRESULT(WINAPI*)(HANDLE, PCWSTR);
		auto setThreadDescription = reinterpret_cast<SetThreadDescriptionFn>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetThreadDescription"));
		if (setThreadDescription)
			setThreadDescription(GetCurrentThread(), wide.c_str());
#else
		// std::jthread has no name. Do not call pthread_setname_np.
		(void)poolName;
#endif
	}

	// Hands the queued message to the stub callback. complete takes ownership of the released pointer.
	void DispatchQueuedItem(WorkItem& item, SnaccPooledInboundAction action)
	{
		// No callback means the stub did not install one. The message dies with the work item. This is not a normal enqueue.
		if (!item.callbacks.complete)
			return;
		// The public callback takes a raw pointer. Release here and the stub adopts it as its first act, so the message is not freed twice.
		SNACC::ROSEMessage* raw = item.message.release();
		item.callbacks.complete(raw, item.size, action);
	}

	// Enqueue-time deadline from a positive ROSEInvoke.timeout. Unset when the field is absent or not positive.
	std::optional<std::chrono::steady_clock::time_point> DeadlineFor(const SNACC::ROSEInvoke& invoke)
	{
		// Absent or non-positive timeout means the client did not set a deadline. The worker then always runs the handler.
		if (!invoke.timeout || static_cast<int>(*invoke.timeout) <= 0)
			return std::nullopt;
		// Captured at enqueue, not when the worker starts, so time spent queued counts against the client timeout.
		return std::chrono::steady_clock::now() + std::chrono::milliseconds(static_cast<int>(*invoke.timeout));
	}
} // namespace

// Shared registry. The process owner constructs one and lends it to every stub that should pool.
SnaccWorkerPools::SnaccWorkerPools()
	: m_state(std::make_unique<State>())
{
}

// Joins workers before the registry is destroyed. Idle threads are still blocked in the wait.
SnaccWorkerPools::~SnaccWorkerPools()
{
	Shutdown();
}

// Records a handler that has been popped for this stub, or records that it returned.
// RejectQueuedAndWait scans the queues first and then waits until this map no longer contains the stub,
// so the increment must happen before the queue lock is released.
void SnaccWorkerPools::NoteInflight(SnaccROSEBase* stub, int delta)
{
	std::lock_guard<std::mutex> guard(m_state->inflightMutex);
	unsigned int& count = m_state->inflight[stub];
	// Positive when a worker has popped an item. Negative when that handler returns. The count does not go below zero.
	if (delta > 0)
		count += static_cast<unsigned int>(delta);
	else if (count > 0)
		--count;
	// Erase at zero so a waiter treats "absent" as finished. A stored zero would look like a stub that is still tracked.
	if (count == 0)
		m_state->inflight.erase(stub);
	// More than one stub can be draining at the same time, so every waiter is woken.
	m_state->inflightCv.notify_all();
}

// Starts the one thread that stays for the life of the pool, before any invoke has arrived.
void SnaccWorkerPools::StartFloorWorker(const std::shared_ptr<SnaccWorkerPoolState>& pool)
{
	// Configure calls this while holding the registry lock. This takes the pool lock. Do not call it when the pool lock is already held.
	std::lock_guard<std::mutex> lock(pool->mutex);
	// A second Configure of the same pool object, or a pool already stopping, must not start another floor thread.
	if (pool->stopping || pool->liveWorkers > 0)
		return;
	// Count the thread before it runs, so LiveThreads does not report zero in the gap between start and the thread entering WorkerMain.
	++pool->liveWorkers;
	pool->threads.emplace_back([this, pool] { WorkerMain(pool); });
}

// Starts one more worker when every live thread is busy and the queue still has work.
// Caller holds the pool mutex. An idle worker is left to wake instead of creating a thread.
void SnaccWorkerPools::EnsureWorker(const std::shared_ptr<SnaccWorkerPoolState>& pool)
{
	// Called from Enqueue and from a retiring worker, both already holding the pool mutex.
	// Skip when a waiter exists, the cap is reached, the queue is empty, or the pool is stopping.
	if (pool->stopping || pool->liveWorkers >= pool->maxThreads || pool->idleWorkers > 0 || pool->queue.empty())
		return;
	// The new thread may find the item already taken and then idle-exit. That is cheaper than blocking the enqueue on a busy pool.
	++pool->liveWorkers;
	pool->threads.emplace_back([this, pool] { WorkerMain(pool); });
}

// Leaves the pool after an idle wait when a floor thread is still present. Detaches because this is that thread.
bool SnaccWorkerPools::ExitIdleWorker(const std::shared_ptr<SnaccWorkerPoolState>& pool)
{
	// Called on the idle thread after its wait found an empty queue, while that thread holds the pool mutex.
	// Stay when Shutdown must still join us, when we are the floor thread, or when work arrived before we left.
	if (pool->stopping || pool->liveWorkers <= kMinLiveWorkers || !pool->queue.empty())
		return false;
	--pool->liveWorkers;
	// This jthread cannot join itself. Detach and drop it so JoinPool will not wait on it.
	const auto self = std::this_thread::get_id();
	const auto it = std::find_if(pool->threads.begin(), pool->threads.end(), [&](const std::jthread& thread) { return thread.get_id() == self; });
	if (it != pool->threads.end())
	{
		it->detach();
		pool->threads.erase(it);
	}
	return true;
}

// Pool thread body. Waits for one item, runs the stub callback, then waits again.
// The floor thread stays in this loop until Shutdown. Extra threads leave it after an idle timeout.
void SnaccWorkerPools::WorkerMain(const std::shared_ptr<SnaccWorkerPoolState>& pool)
{
	NameWorkerThread(pool->name);
	// Published for CurrentPoolIsStopping so a handler on this thread can drop its reply during Shutdown.
	t_currentPool = pool.get();
	while (true)
	{
		WorkItem item;
		{
			std::unique_lock<std::mutex> lock(pool->mutex);
			++pool->idleWorkers;
			// The floor thread waits until work or shutdown. Threads above it leave after idleTimeout.
			const bool shrinkWhenIdle = pool->liveWorkers > kMinLiveWorkers && pool->idleTimeout.count() > 0;
			if (shrinkWhenIdle)
				pool->cv.wait_for(lock, pool->idleTimeout, [&] { return pool->stopping || !pool->queue.empty(); });
			else
				pool->cv.wait(lock, [&] { return pool->stopping || !pool->queue.empty(); });
			--pool->idleWorkers;
			// Shutdown with nothing left to run. Do not detach: JoinPool still has to join this thread.
			if (pool->stopping && pool->queue.empty())
			{
				--pool->liveWorkers;
				pool->cv.notify_all();
				break;
			}
			// Woke from the idle timeout, or became the floor while waiting. Leave only when another thread remains.
			if (pool->queue.empty())
			{
				if (ExitIdleWorker(pool))
					break;
				continue;
			}
			item = std::move(pool->queue.front());
			pool->queue.pop_front();
			++pool->running;
			// Count the stub before releasing the queue lock. Pause and stub destruction
			// scan the queue and then wait on this count, so a popped item must already be in it.
			NoteInflight(item.stub, 1);
		}
		bool retire = false;
		try
		{
			// Stopping and a closed session both reject without the handler.
			// A deadline that passed while the item sat in the queue drops the reply instead.
			const bool expired = item.deadline.has_value() && std::chrono::steady_clock::now() >= *item.deadline;
			const bool allowed = item.callbacks.processingAllowed && item.callbacks.processingAllowed();
			if (pool->stopping || !allowed)
				DispatchQueuedItem(item, SnaccPooledInboundAction::ShutdownReject);
			else if (expired)
				DispatchQueuedItem(item, SnaccPooledInboundAction::DropExpired);
			else
				DispatchQueuedItem(item, SnaccPooledInboundAction::Run);
		}
		catch (...)
		{
			// SnaccException is turned into a mistyped-argument reject inside the stub and does not reach here.
			retire = pool->faultPolicy == SnaccWorkerFaultPolicy::RetireWorker;
		}
		NoteInflight(item.stub, -1);

		{
			std::lock_guard<std::mutex> lock(pool->mutex);
			--pool->running;
			// This thread is done. Start a replacement when the queue still has work, then leave the loop.
			if (retire)
			{
				--pool->liveWorkers;
				EnsureWorker(pool);
				break;
			}
		}
	}
	// This thread is no longer a pool worker. CurrentPoolIsStopping must be false for whatever it runs next.
	t_currentPool = nullptr;
}

// Stops one pool. Queued items are rejected on this thread. Handlers already running finish, then the workers are joined.
void SnaccWorkerPools::JoinPool(const std::shared_ptr<SnaccWorkerPoolState>& pool)
{
	std::vector<WorkItem> queued;
	{
		// Wake every waiter with an empty queue. Items already popped stay with the worker that took them.
		std::lock_guard<std::mutex> lock(pool->mutex);
		pool->stopping = true;
		queued.assign(std::make_move_iterator(pool->queue.begin()), std::make_move_iterator(pool->queue.end()));
		pool->queue.clear();
		pool->cv.notify_all();
	}
	// Reject outside the pool mutex. The stub callback may send on the transport.
	for (auto& item : queued)
		DispatchQueuedItem(item, SnaccPooledInboundAction::ShutdownReject);

	// Take the thread handles out from under the lock, then join. A thread that idle-exited already detached itself.
	std::vector<std::jthread> threads;
	{
		std::lock_guard<std::mutex> lock(pool->mutex);
		threads.swap(pool->threads);
	}
	for (auto& thread : threads)
		if (thread.joinable())
			thread.join();
}

// Pool name for one operation id. Empty when this registry does not pool that id, including after Shutdown.
std::string SnaccWorkerPools::PoolForOperation(unsigned int uiOperationId) const
{
	// Held only for the map lookup. Callers use the returned name and then lock the pool separately.
	std::lock_guard<std::mutex> guard(m_state->registryMutex);
	const auto it = m_state->operationPools.find(uiOperationId);
	if (it != m_state->operationPools.end())
		return it->second->name;
	else
		return {};
}

// Installs a new pool set. The previous set is left running until the new set has been checked.
bool SnaccWorkerPools::Configure(const std::vector<SnaccWorkerPoolConfig>& pools)
{
	// One operation id belongs to one pool. A duplicate refuses the call before any thread is stopped.
	std::unordered_set<unsigned int> operationIds;
	for (const SnaccWorkerPoolConfig& config : pools)
	{
		for (const unsigned int operationId : config.operationIds)
		{
			if (!operationIds.insert(operationId).second)
			{
				ASSERT(false, "operationID is already assigned to a worker pool in this registry");
				return false;
			}
		}
	}

	// The new set is valid. Reject whatever the old pools still hold and join those threads.
	Shutdown();
	// Hold the registry lock across insert and the floor-thread start, so an enqueue cannot see a pool with no worker.
	std::lock_guard<std::mutex> guard(m_state->registryMutex);
	for (const SnaccWorkerPoolConfig& config : pools)
	{
		auto pool = std::make_shared<SnaccWorkerPoolState>();
		pool->name = config.name;
		pool->maxThreads = std::max(1u, config.maxThreads);
		pool->maxElements = config.maxElements;
		pool->idleTimeout = std::chrono::milliseconds(config.idleTimeoutMs);
		pool->faultPolicy = config.faultPolicy;
		// A second config with the same name is ignored. Its operation ids are not registered.
		if (!m_state->poolsByName.emplace(pool->name, pool).second)
			continue;
		m_state->pools.push_back(pool);
		for (const unsigned int operationId : config.operationIds)
			m_state->operationPools.emplace(operationId, pool);
		// One thread now, named for crash dumps. Further threads start on the next enqueue that finds everyone busy.
		StartFloorWorker(pool);
	}
	return true;
}

// Reports threads that are waiting or inside a handler. Idle threads that already exited are not counted.
unsigned int SnaccWorkerPools::LiveThreads(const std::string& poolName) const
{
	std::shared_ptr<SnaccWorkerPoolState> pool;
	{
		// Copy the pool out and drop the registry lock before taking the pool lock, same order as Enqueue.
		std::lock_guard<std::mutex> guard(m_state->registryMutex);
		const auto it = m_state->poolsByName.find(poolName);
		if (it == m_state->poolsByName.end())
			return 0;
		pool = it->second;
	}
	// liveWorkers includes the floor thread waiting on an empty queue. It drops when an extra thread idle-exits.
	std::lock_guard<std::mutex> lock(pool->mutex);
	return pool->liveWorkers;
}

// Drops the registry maps, then joins every pool. A concurrent enqueue sees no pools and runs on the caller.
void SnaccWorkerPools::Shutdown()
{
	// State is created in the constructor and lives as long as the registry. An empty pointer means there is nothing to stop.
	if (!m_state)
		return;
	std::vector<std::shared_ptr<SnaccWorkerPoolState>> pools;
	{
		// Remove the pools from the maps before joining. An enqueue that arrives during the join finds nothing and runs on the caller.
		std::lock_guard<std::mutex> guard(m_state->registryMutex);
		pools.swap(m_state->pools);
		m_state->poolsByName.clear();
		m_state->operationPools.clear();
	}
	// Join outside the registry lock. A handler still running may call back into the stub, and that must not need this lock.
	for (const auto& pool : pools)
		JoinPool(pool);
}

// Offers one decoded invoke to a named pool. On any result other than Queued the message stays with the caller.
SnaccWorkerEnqueueResult SnaccWorkerPools::Enqueue(const std::string& poolName, SnaccROSEBase& stub, std::unique_ptr<SNACC::ROSEMessage>& pMessage, unsigned long ulMessageSize, SnaccPooledInboundCallbacks callbacks)
{
	std::shared_ptr<SnaccWorkerPoolState> pool;
	{
		// Copy the pool pointer out, then drop the registry lock before taking the pool lock.
		std::lock_guard<std::mutex> guard(m_state->registryMutex);
		const auto it = m_state->poolsByName.find(poolName);
		if (it == m_state->poolsByName.end())
			return SnaccWorkerEnqueueResult::NotPooled;
		pool = it->second;
	}

	std::lock_guard<std::mutex> lock(pool->mutex);
	// Shutdown already swapped this pool out of the maps, or JoinPool has marked it. The caller sends the shutdown reject.
	if (pool->stopping)
		return SnaccWorkerEnqueueResult::PoolStopped;
	// Depth is the queue plus handlers already running, so one long SQL call still occupies a slot.
	if (pool->maxElements > 0 && pool->queue.size() + pool->running >= pool->maxElements)
		return SnaccWorkerEnqueueResult::RejectedFull;

	// Ownership moves only after the depth check. The deadline is fixed now, so queue time counts against the client timeout.
	WorkItem item;
	item.stub = &stub;
	item.size = ulMessageSize;
	item.deadline = DeadlineFor(*pMessage->invoke);
	item.callbacks = std::move(callbacks);
	item.message = std::move(pMessage);
	pool->queue.push_back(std::move(item));
	// Wake an idle waiter when one exists. Start a thread only when every live worker is busy and the cap allows it.
	EnsureWorker(pool);
	pool->cv.notify_one();
	return SnaccWorkerEnqueueResult::Queued;
}

// Drops one stub from every pool queue and waits until that stub's running handlers return.
// Other stubs on the same pools stay queued. Used by pause, destruction, and SetWorkerPools.
void SnaccWorkerPools::RejectQueuedAndWait(SnaccROSEBase& stub)
{
	std::vector<WorkItem> queued;
	{
		std::lock_guard<std::mutex> guard(m_state->registryMutex);
		for (const auto& pool : m_state->pools)
		{
			// Split this stub's items out. The kept deque is what the workers still drain.
			std::lock_guard<std::mutex> lock(pool->mutex);
			std::deque<WorkItem> kept;
			for (auto& item : pool->queue)
				if (item.stub == &stub)
					queued.push_back(std::move(item));
				else
					kept.push_back(std::move(item));
			pool->queue.swap(kept);
		}
	}
	for (auto& item : queued)
		DispatchQueuedItem(item, SnaccPooledInboundAction::ShutdownReject);

	// Items already popped are not in the queue. Wait until NoteInflight clears this stub.
	std::unique_lock<std::mutex> lock(m_state->inflightMutex);
	m_state->inflightCv.wait(lock, [&] { return m_state->inflight.find(&stub) == m_state->inflight.end(); });
}

// Read by the stub after a handler returns, on the worker that ran it. False on the socket thread.
bool SnaccWorkerPools::CurrentPoolIsStopping()
{
	return t_currentPool != nullptr && t_currentPool->stopping;
}
