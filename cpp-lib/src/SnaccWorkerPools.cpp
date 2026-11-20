#include "../include/SnaccWorkerPools.h"
#include "../include/SNACCROSE.h"
#include "../include/SnaccROSEBase.h"
#include "../include/SnaccTelemetry.h"
#include "snacc-assert.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
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
	// When the item entered the queue. Wait is measured from here until the item leaves.
	// Unset when this item was not counted. CountWait must not run on an unset stamp.
	std::chrono::steady_clock::time_point enqueuedAt{};
	// True when enqueue counted this item. Completion counts the outcome only for that generation.
	bool bTelemetryCounted{false};
	// RoseWorkerPoolTelemetryEpoch at the moment incoming was incremented. A later edge makes this stale.
	std::uint64_t uiTelemetryEpoch{};
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
	// Every message offered to this pool, including door refusals.
	std::uint64_t incoming{};
	// The handler sent a result. Each one has one m_run observation.
	std::uint64_t success{};
	// The handler sent an error. Each one has one m_run observation.
	std::uint64_t handlerError{};
	// A check or the handler rejected. A check before OnInvoke has no m_run observation.
	std::uint64_t handlerReject{};
	// The queue was full. The message never entered.
	std::uint64_t rejectedFull{};
	// Stopping. At the door the message never entered. From the queue it already had.
	std::uint64_t rejectedShuttingDown{};
	// This connection closed. The pool keeps serving the others.
	std::uint64_t rejectedConnectionClosed{};
	// The deadline passed before the handler began.
	std::uint64_t timedOut{};
	// The handler finished and the reply was dropped. One m_run observation, and not also a success.
	std::uint64_t replyDiscarded{};
	// How long the handler ran.
	LatencyHistogram run{};
	// How long a request waited, for every request that left the queue.
	LatencyHistogram wait{};
	// Start of each request a thread is inside now. The oldest is the minimum.
	std::vector<std::chrono::steady_clock::time_point> runningSince{};
	// Floor thread, burst threads, and replacements after RetireWorker.
	std::uint64_t threadsStarted{};
	// Idle exit, RetireWorker, and the exit during Shutdown.
	std::uint64_t threadsStopped{};
	// Last RoseWorkerPoolTelemetryEpoch applied under this pool's mutex. A mismatch clears the sums.
	std::uint64_t telemetryEpoch{};
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
	// Pools Shutdown has joined. Health keeps them until the telemetry generation changes or Configure reuses the name.
	std::vector<ROSEWorkerPoolHealth> retired{};
	// Epoch of the retired rows. Health drops them when the generation changes, so an old drain does not return.
	// Shutdown sets this to the current epoch before it stores its final rows. Otherwise the next Health treats that drain as stale.
	std::uint64_t telemetryEpoch{};
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

	// Leading token on every worker so a debugger filter finds the pools. The configured pool name follows it.
	constexpr std::string_view kWorkerThreadNamePrefix = "wp::";

	// Publishes `wp::<pool>` when the worker starts. A pool named `sql::ModuleName` is `wp::sql::ModuleName`.
	// The debugger already shows the thread id, so the name does not repeat it.
	// C++20 has no thread-naming function. Windows uses SetThreadDescription. Other platforms leave the OS name unset.
	void NameWorkerThread(const std::string& poolName)
	{
		const std::string name = std::string(kWorkerThreadNamePrefix) + poolName;
#ifdef _WIN32
		// SetThreadDescription is looked up because older Windows does not export it. A missing export leaves the thread unnamed.
		std::wstring wide(name.begin(), name.end());
		using SetThreadDescriptionFn = HRESULT(WINAPI*)(HANDLE, PCWSTR);
		auto setThreadDescription = reinterpret_cast<SetThreadDescriptionFn>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetThreadDescription"));
		if (setThreadDescription)
			setThreadDescription(GetCurrentThread(), wide.c_str());
#else
		// std::jthread has no name. Do not call pthread_setname_np.
		(void)name;
#endif
	}

	// How an item leaves the pool. Run is refined by the callback report. The others are known before the callback.
	enum class PoolLeave
	{
		Success,
		HandlerError,
		HandlerReject,
		RejectedFull,
		RejectedShuttingDown,
		RejectedConnectionClosed,
		TimedOut,
		ReplyDiscarded
	};

	// Microseconds since start. A clock step backwards counts as zero.
	std::uint64_t ElapsedUs(std::chrono::steady_clock::time_point start)
	{
		const auto elapsed = std::chrono::steady_clock::now() - start;
		if (elapsed <= std::chrono::steady_clock::duration::zero())
			return 0;
		return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
	}

	// One observation into a cumulative histogram. Every bucket whose bound the value meets is incremented.
	// The last bound is the maximum uint64, so that bucket stays equal to the count.
	void ObserveUs(LatencyHistogram& histogram, std::uint64_t ui64Us)
	{
		for (std::size_t i = 0; i < kLatencyBucketsUs.size(); ++i)
			if (ui64Us <= kLatencyBucketsUs[i])
				++histogram.m_buckets[i];
		histogram.m_ui64SumUs += ui64Us;
		++histogram.m_ui64Count;
	}

	// Adds one outcome. The caller holds the pool mutex. Incoming was already counted.
	void CountOutcome(SnaccWorkerPoolState& pool, PoolLeave leave)
	{
		switch (leave)
		{
			case PoolLeave::Success:
				++pool.success;
				break;
			case PoolLeave::HandlerError:
				++pool.handlerError;
				break;
			case PoolLeave::HandlerReject:
				++pool.handlerReject;
				break;
			case PoolLeave::RejectedFull:
				++pool.rejectedFull;
				break;
			case PoolLeave::RejectedShuttingDown:
				++pool.rejectedShuttingDown;
				break;
			case PoolLeave::RejectedConnectionClosed:
				++pool.rejectedConnectionClosed;
				break;
			case PoolLeave::TimedOut:
				++pool.timedOut;
				break;
			case PoolLeave::ReplyDiscarded:
				++pool.replyDiscarded;
				break;
		}
	}

	// Records one handler duration when this outcome is one the handler produced.
	// A pre-check reject passes bHandlerRan false and adds no observation.
	void CountRun(SnaccWorkerPoolState& pool, PoolLeave leave, bool bHandlerRan, std::uint64_t ui64RunUs)
	{
		CountOutcome(pool, leave);
		const bool bObserve = leave == PoolLeave::Success || leave == PoolLeave::HandlerError || leave == PoolLeave::ReplyDiscarded || (leave == PoolLeave::HandlerReject && bHandlerRan);
		if (bObserve)
			ObserveUs(pool.run, ui64RunUs);
	}

	// Wait of one item that has just left the queue. Rejected-full and a door shutdown never call this.
	void CountWait(SnaccWorkerPoolState& pool, const WorkItem& item)
	{
		ObserveUs(pool.wait, ElapsedUs(item.enqueuedAt));
	}

	// Zeros the sums. Caller holds the pool mutex.
	// threadsStarted becomes the threads still inside the pool so started minus stopped stays equal to liveWorkers.
	// Queue depth, idle threads, and running threads are left as they are.
	void ClearPoolSums(SnaccWorkerPoolState& pool)
	{
		pool.incoming = 0;
		pool.success = 0;
		pool.handlerError = 0;
		pool.handlerReject = 0;
		pool.rejectedFull = 0;
		pool.rejectedShuttingDown = 0;
		pool.rejectedConnectionClosed = 0;
		pool.timedOut = 0;
		pool.replyDiscarded = 0;
		pool.run = {};
		pool.wait = {};
		pool.threadsStopped = 0;
		pool.threadsStarted = pool.liveWorkers;
	}

	// Applies a ConfigureTelemetry edge. Returns true when the sums were cleared.
	// Caller holds the pool mutex.
	bool ApplyWorkerPoolTelemetryEpoch(SnaccWorkerPoolState& pool)
	{
		const std::uint64_t uiEpoch = RoseWorkerPoolTelemetryEpoch();
		if (pool.telemetryEpoch == uiEpoch)
			return false;
		ClearPoolSums(pool);
		pool.telemetryEpoch = uiEpoch;
		return true;
	}

	// True when this pool should count. Applies a pending edge first, under the caller's pool mutex.
	bool WorkerPoolTelemetryActive(SnaccWorkerPoolState& pool)
	{
		ApplyWorkerPoolTelemetryEpoch(pool);
		return IsRoseWorkerPoolTelemetryEnabled();
	}

	// True when this item was counted and that generation is still the one stored on the pool.
	// Caller holds the pool mutex and has already applied the epoch.
	bool WorkItemStillCounted(const SnaccWorkerPoolState& pool, const WorkItem& item)
	{
		if (!item.bTelemetryCounted)
			return false;
		if (item.uiTelemetryEpoch != pool.telemetryEpoch)
			return false;
		return IsRoseWorkerPoolTelemetryEnabled();
	}

	// Counts a thread start only while telemetry is on. A reset in this call already includes the new live thread.
	// Caller holds the pool mutex and has already incremented liveWorkers.
	void NoteThreadStarted(SnaccWorkerPoolState& pool)
	{
		const bool bReset = ApplyWorkerPoolTelemetryEpoch(pool);
		if (!bReset && IsRoseWorkerPoolTelemetryEnabled())
			++pool.threadsStarted;
	}

	// Counts a thread exit only while telemetry is on. A reset in this call already matches the reduced live count.
	// Caller holds the pool mutex and has already decremented liveWorkers.
	void NoteThreadStopped(SnaccWorkerPoolState& pool)
	{
		const bool bReset = ApplyWorkerPoolTelemetryEpoch(pool);
		if (!bReset && IsRoseWorkerPoolTelemetryEnabled())
			++pool.threadsStopped;
	}

	// Maps the stub report onto the stored outcome. ReplyDiscarded is not also a success.
	PoolLeave LeaveFromReport(const SnaccPooledRunReport& report)
	{
		switch (report.outcome)
		{
			case SnaccPooledRunOutcome::Success:
				return PoolLeave::Success;
			case SnaccPooledRunOutcome::HandlerError:
				return PoolLeave::HandlerError;
			case SnaccPooledRunOutcome::ReplyDiscarded:
				return PoolLeave::ReplyDiscarded;
			case SnaccPooledRunOutcome::HandlerReject:
				return PoolLeave::HandlerReject;
		}
		return PoolLeave::HandlerReject;
	}

	// Hands the queued message to the stub callback. complete takes ownership of the released pointer.
	// The report matters for a Run action. The caller ignores it for every other action.
	SnaccPooledRunReport DispatchQueuedItem(WorkItem& item, SnaccPooledInboundAction action)
	{
		// No callback means the stub did not install one. The message dies with the work item. This is not a normal enqueue.
		if (!item.callbacks.complete)
			return {};
		// The public callback takes a raw pointer. Release here and the stub adopts it as its first act, so the message is not freed twice.
		SNACC::ROSEMessage* raw = item.message.release();
		return item.callbacks.complete(raw, item.size, action);
	}

	// Enqueue-time deadline from a positive ROSEInvoke.timeout. Unset when the field is absent or not positive.
	std::optional<std::chrono::steady_clock::time_point> DeadlineFor(const SNACC::ROSEInvoke& invoke)
	{
		// 0 is fire-and-forget, not "already expired". now+0 would drop the item before the handler runs.
		// Absent or non-positive timeout means there is no deadline. The worker always runs the handler.
		if (!invoke.timeout || static_cast<int>(*invoke.timeout) <= 0)
			return std::nullopt;
		// Captured at enqueue, not when the worker starts, so time spent queued counts against the client timeout.
		return std::chrono::steady_clock::now() + std::chrono::milliseconds(static_cast<int>(*invoke.timeout));
	}

	// Copies one pool under the caller's lock. Debug checks the two sums that hold at this moment.
	// started minus stopped is the live threads. Idle plus running can be lower until a new thread reaches the wait.
	// Incoming equals requests still here plus every outcome. The wait sum does not, because shutdown mixes the door and the queue.
	ROSEWorkerPoolHealth CopyPoolHealth(const SnaccWorkerPoolState& pool)
	{
		ASSERT(pool.threadsStarted >= pool.threadsStopped, "threadsStarted is below threadsStopped");
		ASSERT(pool.threadsStarted - pool.threadsStopped == pool.liveWorkers, "started minus stopped is not the live threads");
		ASSERT(pool.threadsStarted - pool.threadsStopped >= static_cast<std::uint64_t>(pool.idleWorkers) + pool.running, "started minus stopped is below idle plus running");
		// The sum stays inside the assert. A Release build drops the assert, so a named variable here would be unused.
		ASSERT(pool.incoming == static_cast<std::uint64_t>(pool.queue.size()) + pool.running + pool.success + pool.handlerError + pool.handlerReject + pool.rejectedFull + pool.rejectedShuttingDown + pool.rejectedConnectionClosed + pool.timedOut + pool.replyDiscarded, "incoming is not waiting plus running plus outcomes");

		ROSEWorkerPoolHealth row;
		row.m_strName = pool.name;
		row.m_uiMaxThreads = pool.maxThreads;
		row.m_counters.m_ui64Incoming = pool.incoming;
		row.m_counters.m_ui64Success = pool.success;
		row.m_counters.m_ui64HandlerError = pool.handlerError;
		row.m_counters.m_ui64HandlerReject = pool.handlerReject;
		row.m_counters.m_ui64RejectedFull = pool.rejectedFull;
		row.m_counters.m_ui64RejectedShuttingDown = pool.rejectedShuttingDown;
		row.m_counters.m_ui64RejectedConnectionClosed = pool.rejectedConnectionClosed;
		row.m_counters.m_ui64TimedOut = pool.timedOut;
		row.m_counters.m_ui64ReplyDiscarded = pool.replyDiscarded;
		row.m_counters.m_run = pool.run;
		row.m_state.m_uiIdleThreads = pool.idleWorkers;
		row.m_state.m_uiRunning = pool.running;
		if (!pool.runningSince.empty())
		{
			const auto oldest = *std::min_element(pool.runningSince.begin(), pool.runningSince.end());
			row.m_state.m_ui64OldestRunningUs = ElapsedUs(oldest);
		}
		if (pool.maxElements > 0)
			row.m_uiMaxElements = pool.maxElements;
		row.m_ui64ThreadsStarted = pool.threadsStarted;
		row.m_ui64ThreadsStopped = pool.threadsStopped;
		row.m_uiWaiting = static_cast<unsigned int>(pool.queue.size());
		if (!pool.queue.empty())
		{
			auto oldest = pool.queue.front().enqueuedAt;
			for (const WorkItem& item : pool.queue)
				if (item.enqueuedAt < oldest)
					oldest = item.enqueuedAt;
			row.m_ui64OldestWaitingUs = ElapsedUs(oldest);
		}
		row.m_wait = pool.wait;
		return row;
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
	NoteThreadStarted(*pool);
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
	NoteThreadStarted(*pool);
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
	NoteThreadStopped(*pool);
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
		std::chrono::steady_clock::time_point startedAt{};
		bool bCountRun = false;
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
				NoteThreadStopped(*pool);
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
			// The request has left the queue. Wait is counted only for an item this generation still owns.
			ApplyWorkerPoolTelemetryEpoch(*pool);
			bCountRun = WorkItemStillCounted(*pool, item);
			if (bCountRun)
				CountWait(*pool, item);
			if (bCountRun)
			{
				startedAt = std::chrono::steady_clock::now();
				pool->runningSince.push_back(startedAt);
			}
			// Count the stub before releasing the queue lock. Pause and stub destruction
			// scan the queue and then wait on this count, so a popped item must already be in it.
			NoteInflight(item.stub, 1);
		}
		bool retire = false;
		// Stopping wins over a closed session, and both win over a deadline. Exactly one outcome.
		const bool deadlinePassed = item.deadline.has_value() && std::chrono::steady_clock::now() >= *item.deadline;
		const bool allowed = item.callbacks.processingAllowed && item.callbacks.processingAllowed();
		const PoolLeave decided = pool->stopping ? PoolLeave::RejectedShuttingDown : !allowed ? PoolLeave::RejectedConnectionClosed : deadlinePassed ? PoolLeave::TimedOut : PoolLeave::Success;
		SnaccPooledRunReport report{};
		bool bThrew = false;
		const auto runStarted = bCountRun ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
		try
		{
			// Stopping and a closed session both reject without the handler.
			// A deadline that passed while the item sat in the queue drops the reply instead.
			if (decided == PoolLeave::RejectedShuttingDown || decided == PoolLeave::RejectedConnectionClosed)
				DispatchQueuedItem(item, SnaccPooledInboundAction::ShutdownReject);
			else if (decided == PoolLeave::TimedOut)
				DispatchQueuedItem(item, SnaccPooledInboundAction::DropExpired);
			else
				report = DispatchQueuedItem(item, SnaccPooledInboundAction::Run);
		}
		catch (...)
		{
			// SnaccException is turned into a mistyped-argument reject inside the stub and does not reach here.
			bThrew = true;
			retire = pool->faultPolicy == SnaccWorkerFaultPolicy::RetireWorker;
		}
		const std::uint64_t ui64RunUs = bCountRun ? ElapsedUs(runStarted) : 0;
		NoteInflight(item.stub, -1);

		{
			std::lock_guard<std::mutex> lock(pool->mutex);
			--pool->running;
			if (bCountRun)
			{
				const auto runningIt = std::find(pool->runningSince.begin(), pool->runningSince.end(), startedAt);
				if (runningIt != pool->runningSince.end())
					pool->runningSince.erase(runningIt);
			}
			// A throw past OnInvoke ran the handler and sent neither a result nor a reject. That is the only outcome.
			// An edge since enqueue drops this outcome. The sums for that generation were already cleared.
			ApplyWorkerPoolTelemetryEpoch(*pool);
			if (WorkItemStillCounted(*pool, item))
			{
				if (decided == PoolLeave::Success && bThrew)
					CountRun(*pool, PoolLeave::HandlerError, true, ui64RunUs);
				else if (decided == PoolLeave::Success)
					CountRun(*pool, LeaveFromReport(report), report.bHandlerRan, ui64RunUs);
				else
					CountOutcome(*pool, decided);
			}
			// This thread is done. Start a replacement when the queue still has work, then leave the loop.
			if (retire)
			{
				--pool->liveWorkers;
				NoteThreadStopped(*pool);
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
		// These items leave the queue here. Each one waited, and the outcome is shutdown. Count them before the reject runs outside the lock.
		ApplyWorkerPoolTelemetryEpoch(*pool);
		for (const WorkItem& item : queued)
		{
			if (!WorkItemStillCounted(*pool, item))
				continue;
			CountWait(*pool, item);
			CountOutcome(*pool, PoolLeave::RejectedShuttingDown);
		}
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
		// A new life of this name starts at zero. The previous life's final sample is no longer returned.
		m_state->retired.erase(std::remove_if(m_state->retired.begin(), m_state->retired.end(), [&](const ROSEWorkerPoolHealth& row) { return row.m_strName == pool->name; }), m_state->retired.end());
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

// Copies the counters a product health timer reads. Workers do not call into that timer.
std::vector<ROSEWorkerPoolHealth> SnaccWorkerPools::Health() const
{
	std::vector<std::shared_ptr<SnaccWorkerPoolState>> pools;
	std::vector<ROSEWorkerPoolHealth> retired;
	bool bEnabled = false;
	{
		// Same lock order as Enqueue: registry lock only long enough to copy the pool pointers.
		std::lock_guard<std::mutex> guard(m_state->registryMutex);
		const std::uint64_t uiEpoch = RoseWorkerPoolTelemetryEpoch();
		if (m_state->telemetryEpoch != uiEpoch)
		{
			m_state->retired.clear();
			m_state->telemetryEpoch = uiEpoch;
		}
		bEnabled = IsRoseWorkerPoolTelemetryEnabled();
		pools = m_state->pools;
		if (bEnabled)
			retired = m_state->retired;
	}
	if (!bEnabled)
	{
		// The edge clears under the pool lock even when the caller only asked for a copy.
		for (const std::shared_ptr<SnaccWorkerPoolState>& pool : pools)
		{
			std::lock_guard<std::mutex> lock(pool->mutex);
			ApplyWorkerPoolTelemetryEpoch(*pool);
		}
		return {};
	}
	std::vector<ROSEWorkerPoolHealth> health;
	health.reserve(pools.size() + retired.size());
	std::unordered_set<std::string> liveNames;
	for (const std::shared_ptr<SnaccWorkerPoolState>& pool : pools)
	{
		std::lock_guard<std::mutex> lock(pool->mutex);
		ApplyWorkerPoolTelemetryEpoch(*pool);
		if (!IsRoseWorkerPoolTelemetryEnabled())
			continue;
		health.push_back(CopyPoolHealth(*pool));
		liveNames.insert(pool->name);
	}
	// A shut-down pool stays visible until Configure reuses the name. Its counters are already final.
	for (ROSEWorkerPoolHealth& row : retired)
		if (liveNames.find(row.m_strName) == liveNames.end())
			health.push_back(std::move(row));
	return health;
}

// Drops the registry maps, then joins every pool. A concurrent enqueue sees no pools and runs on the caller.
void SnaccWorkerPools::Shutdown()
{
	// State is created in the constructor and lives as long as the registry. An empty pointer means there is nothing to stop.
	if (!m_state)
		return;
	std::vector<std::shared_ptr<SnaccWorkerPoolState>> pools;
	{
		// Keep the pools registered while they drain, so a sample during the join still sees the shutdown outcomes.
		std::lock_guard<std::mutex> guard(m_state->registryMutex);
		pools = m_state->pools;
	}
	// Join outside the registry lock. A handler still running may call back into the stub, and that must not need this lock.
	for (const auto& pool : pools)
		JoinPool(pool);
	// The drain is in the counters now. Drop the pools from the live set and keep the final copy for the next sample.
	std::lock_guard<std::mutex> guard(m_state->registryMutex);
	// These rows belong to the generation that just finished. Remember that epoch before storing them.
	// Health drops retired rows when the remembered epoch differs, which would discard this drain.
	const std::uint64_t uiEpoch = RoseWorkerPoolTelemetryEpoch();
	if (m_state->telemetryEpoch != uiEpoch)
	{
		m_state->retired.clear();
		m_state->telemetryEpoch = uiEpoch;
	}
	for (const auto& pool : pools)
	{
		std::lock_guard<std::mutex> lock(pool->mutex);
		ROSEWorkerPoolHealth finalRow = CopyPoolHealth(*pool);
		m_state->retired.erase(std::remove_if(m_state->retired.begin(), m_state->retired.end(), [&](const ROSEWorkerPoolHealth& row) { return row.m_strName == finalRow.m_strName; }), m_state->retired.end());
		m_state->retired.push_back(std::move(finalRow));
		m_state->poolsByName.erase(pool->name);
		m_state->pools.erase(std::remove(m_state->pools.begin(), m_state->pools.end(), pool), m_state->pools.end());
	}
	m_state->operationPools.clear();
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
	// Count the arrival before the outcome, so a sample cannot see an outcome without its incoming.
	const bool bCount = WorkerPoolTelemetryActive(*pool);
	if (bCount)
		++pool->incoming;
	// JoinPool has marked the pool, and it is still registered until the join finishes. The caller sends the shutdown reject.
	if (pool->stopping)
	{
		if (bCount)
			CountOutcome(*pool, PoolLeave::RejectedShuttingDown);
		return SnaccWorkerEnqueueResult::PoolStopped;
	}
	// Depth is the queue plus handlers already running, so one long SQL call still occupies a slot.
	if (pool->maxElements > 0 && pool->queue.size() + pool->running >= pool->maxElements)
	{
		// The message never enters the queue. Invoke and event share this one counter.
		if (bCount)
			CountOutcome(*pool, PoolLeave::RejectedFull);
		return SnaccWorkerEnqueueResult::RejectedFull;
	}

	// Ownership moves only after the depth check. The deadline is fixed now, so queue time counts against the client timeout.
	WorkItem item;
	item.stub = &stub;
	item.size = ulMessageSize;
	item.deadline = DeadlineFor(*pMessage->invoke);
	if (bCount)
	{
		item.bTelemetryCounted = true;
		item.uiTelemetryEpoch = pool->telemetryEpoch;
		item.enqueuedAt = std::chrono::steady_clock::now();
	}
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
				{
					// The item leaves this queue without a worker. It waited, and the connection is closed.
					ApplyWorkerPoolTelemetryEpoch(*pool);
					if (WorkItemStillCounted(*pool, item))
					{
						CountWait(*pool, item);
						CountOutcome(*pool, PoolLeave::RejectedConnectionClosed);
					}
					queued.push_back(std::move(item));
				}
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
