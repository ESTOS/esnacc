#include "test_support/sample_runtime_harness.h"

namespace snacclib
{

	// Specifies the named worker-pool contract from runtime_correctness_notes.md section 12.
	class snacclib_WorkerPoolRuntimeTest : public RuntimeTestBase
	{
	protected:
		void TearDown() override
		{
			if (m_gate)
				m_gate->Release();
			m_server.SetWorkerPools(nullptr);
			m_client.SetWorkerPools(nullptr);
			RuntimeTestBase::TearDown();
		}

		void ArmBlockingGetSettings()
		{
			m_gate = std::make_shared<HandlerBlockGate>();
			HandlerModes modes;
			modes.blockGetSettings = m_gate;
			ConfigureServerHandlers(modes);
		}

		// Sends asnGetSettings through one client endpoint. The latch receives the completion.
		long SendAsyncGetSettingsFrom(RuntimeEndpoint& client, AsyncInvokeLatch& latch, AsnGetSettingsResult& result, AsnRequestError& error, int timeoutMs)
		{
			AsnGetSettingsArgument argument;
			SnaccScopedInvokeMessage invokeMsg(client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
			auto pCtx = client.CreateSessionInvokeContext(invokeMsg.GetPtr());
			pCtx->SetInvokeTimeout(static_cast<unsigned int>(timeoutMs));
			pCtx->SetAsyncCompletion(latch.Callback(), &result, &error);
			return client.SendInvokeAsync(invokeMsg.GetPtr(), &result, &error, "asnGetSettings", std::move(pCtx));
		}

		long SendAsyncGetSettings(AsyncInvokeLatch& latch, AsnGetSettingsResult& result, AsnRequestError& error, int timeoutMs)
		{
			return SendAsyncGetSettingsFrom(m_client, latch, result, error, timeoutMs);
		}

		void ConfigureGetSettingsPool(unsigned int maxThreads, unsigned int maxElements, SnaccWorkerFaultPolicy faultPolicy = SnaccWorkerFaultPolicy::KeepWorker, unsigned int idleTimeoutMs = 30000)
		{
			SnaccWorkerPoolConfig config;
			config.name = "settings";
			config.maxThreads = maxThreads;
			config.maxElements = maxElements;
			config.faultPolicy = faultPolicy;
			config.idleTimeoutMs = idleTimeoutMs;
			config.operationIds.push_back(OPID_asnGetSettings);
			ASSERT_TRUE(m_pools.Configure({config}));
			m_server.SetWorkerPools(&m_pools);
		}

		// Unblocks the settings gate on destruction so a drained stub's handler can return.
		struct ReleaseGate
		{
			std::shared_ptr<HandlerBlockGate> gate{};
			// Releases the gate when the test left a handler blocked.
			~ReleaseGate()
			{
				if (gate)
					gate->Release();
			}
		};

		// Detaches the borrowed registry and waits until that stub's handlers return.
		struct DrainStub
		{
			SnaccROSEBase* stub{};
			// Drops this stub's queued items. The settings module must still be alive.
			~DrainStub()
			{
				if (stub)
					stub->SetWorkerPools(nullptr);
			}
		};

		std::shared_ptr<HandlerBlockGate> m_gate{};
		// Shared registry borrowed by the stubs under test. Cleared in TearDown before this is destroyed.
		SnaccWorkerPools m_pools{};
	};

	TEST_F(snacclib_WorkerPoolRuntimeTest, UnassignedOperationStaysOnCallingThread)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		m_gate->Release();

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		EXPECT_EQ(ROSE_NOERROR, m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error));
		EXPECT_EQ(1, m_gate->Entered());
		EXPECT_EQ(std::this_thread::get_id(), m_gate->ThreadAt(0));
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, AssignedOperationRunsOffCallingThread)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(1, 4);

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(latch, result, error, 2000));
		m_gate->WaitEntered(1);
		EXPECT_EQ(1, m_gate->Entered());
		EXPECT_NE(std::this_thread::get_id(), m_gate->ThreadAt(0));
		m_gate->Release();
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_NOERROR, latch.RoseResult());
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, SingleWorkerKeepsFifoOrder)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(1, 4);

		AsyncInvokeLatch first;
		AsyncInvokeLatch second;
		AsnGetSettingsResult firstResult;
		AsnGetSettingsResult secondResult;
		AsnRequestError firstError;
		AsnRequestError secondError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(first, firstResult, firstError, 2000));
		m_gate->WaitEntered(1);
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(second, secondResult, secondError, 2000));
		EXPECT_EQ(1, m_gate->Entered());
		m_gate->Release();
		ASSERT_TRUE(first.WaitFor(std::chrono::milliseconds(2000)));
		ASSERT_TRUE(second.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_NOERROR, first.RoseResult());
		EXPECT_EQ(ROSE_NOERROR, second.RoseResult());
		EXPECT_EQ(2, m_gate->Entered());
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, FullQueueRejectsInvokeWithoutHandler)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(1, 1);

		AsyncInvokeLatch running;
		AsnGetSettingsResult runningResult;
		AsnRequestError runningError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(running, runningResult, runningError, 2000));
		m_gate->WaitEntered(1);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		EXPECT_EQ(ROSE_REJECT_QUEUE_FULL, m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 2000));
		EXPECT_EQ(1, m_gate->Entered());

		m_gate->Release();
		ASSERT_TRUE(running.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_NOERROR, running.RoseResult());
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, FullQueueDropsEventWithoutReject)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(1, 1);

		AsyncInvokeLatch running;
		AsnGetSettingsResult runningResult;
		AsnRequestError runningError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(running, runningResult, runningError, 2000));
		m_gate->WaitEntered(1);

		AsnGetSettingsArgument argument;
		SnaccScopedInvokeMessage invokeMsg(99999, OPID_asnGetSettings, &argument);
		EXPECT_EQ(ROSE_NOERROR, m_client.SendEvent(invokeMsg.GetPtr(), "asnGetSettings"));
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		EXPECT_EQ(1, m_gate->Entered());

		m_gate->Release();
		ASSERT_TRUE(running.WaitFor(std::chrono::milliseconds(2000)));
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, PauseRejectsQueuedWorkAndDropsInflightReply)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(1, 4);

		AsyncInvokeLatch running;
		AsyncInvokeLatch queued;
		AsnGetSettingsResult runningResult;
		AsnGetSettingsResult queuedResult;
		AsnRequestError runningError;
		AsnRequestError queuedError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(running, runningResult, runningError, 400));
		m_gate->WaitEntered(1);
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(queued, queuedResult, queuedError, 2000));

		std::thread releaser([this] {
			std::this_thread::sleep_for(std::chrono::milliseconds(150));
			m_gate->Release();
		});
		m_server.PauseRoseProcessing();
		releaser.join();

		ASSERT_TRUE(queued.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_REJECT_UNKNOWNOPERATION, queued.RoseResult());
		ASSERT_TRUE(running.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_TE_TIMEOUT, running.RoseResult());
		EXPECT_EQ(1, m_gate->Entered());
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, ExpiredQueuedInvokeDoesNotRunHandler)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(1, 4);

		AsyncInvokeLatch running;
		AsyncInvokeLatch expired;
		AsnGetSettingsResult runningResult;
		AsnGetSettingsResult expiredResult;
		AsnRequestError runningError;
		AsnRequestError expiredError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(running, runningResult, runningError, 500));
		m_gate->WaitEntered(1);
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(expired, expiredResult, expiredError, 30));
		std::this_thread::sleep_for(std::chrono::milliseconds(80));
		m_gate->Release();

		ASSERT_TRUE(running.WaitFor(std::chrono::milliseconds(1000)));
		EXPECT_EQ(ROSE_NOERROR, running.RoseResult());
		ASSERT_TRUE(expired.WaitFor(std::chrono::milliseconds(1000)));
		EXPECT_EQ(ROSE_TE_TIMEOUT, expired.RoseResult());
		EXPECT_EQ(1, m_gate->Entered());
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, StubWithoutBorrowedPoolsStaysOnCallingThread)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		m_gate->Release();

		SnaccWorkerPoolConfig config;
		config.name = "client-settings";
		config.maxThreads = 1;
		config.maxElements = 1;
		config.operationIds.push_back(OPID_asnGetSettings);
		ASSERT_TRUE(m_pools.Configure({config}));
		m_client.SetWorkerPools(&m_pools);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		EXPECT_EQ(ROSE_NOERROR, m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error));
		EXPECT_EQ(1, m_gate->Entered());
		EXPECT_EQ(std::this_thread::get_id(), m_gate->ThreadAt(0));
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, SharedRegistryRunsBothStubsOnOneWorker)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(1, 4);

		SnaccRoseOperationLookup otherServerLookup;
		SnaccRoseOperationLookup otherClientLookup;
		ObservedRuntimeEndpoint otherServer(L"LoopbackServer2", "server-session-2", otherServerLookup);
		ObservedRuntimeEndpoint otherClient(L"LoopbackClient2", "client-session-2", otherClientLookup);
		SettingsServiceModule otherServerModule(otherServer);
		SettingsClientModule otherClientModule(otherClient);
		otherServer.ConnectTo(otherClient);
		otherClient.ConnectTo(otherServer);
		otherServer.SetEncoding(TransportEncoding::JSON);
		otherClient.SetEncoding(TransportEncoding::JSON);
		HandlerModes modes;
		modes.blockGetSettings = m_gate;
		otherServerModule.ConfigureHandlers(modes);
		otherServer.SetWorkerPools(&m_pools);
		DrainStub drainOtherServer{&otherServer};
		ReleaseGate releaseGate{m_gate};

		AsyncInvokeLatch first;
		AsyncInvokeLatch second;
		AsnGetSettingsResult firstResult;
		AsnGetSettingsResult secondResult;
		AsnRequestError firstError;
		AsnRequestError secondError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(first, firstResult, firstError, 2000));
		m_gate->WaitEntered(1);
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettingsFrom(otherClient, second, secondResult, secondError, 2000));
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		EXPECT_EQ(1, m_gate->Entered());

		m_gate->Release();
		ASSERT_TRUE(first.WaitFor(std::chrono::milliseconds(2000)));
		ASSERT_TRUE(second.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_NOERROR, first.RoseResult());
		EXPECT_EQ(ROSE_NOERROR, second.RoseResult());
		EXPECT_EQ(2, m_gate->Entered());
		EXPECT_EQ(m_gate->ThreadAt(0), m_gate->ThreadAt(1));
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, PauseDropsOnlyThePausingStubsQueuedInvoke)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(1, 8);

		SnaccRoseOperationLookup otherServerLookup;
		SnaccRoseOperationLookup otherClientLookup;
		ObservedRuntimeEndpoint otherServer(L"LoopbackServer2", "server-session-2", otherServerLookup);
		ObservedRuntimeEndpoint otherClient(L"LoopbackClient2", "client-session-2", otherClientLookup);
		SettingsServiceModule otherServerModule(otherServer);
		SettingsClientModule otherClientModule(otherClient);
		otherServer.ConnectTo(otherClient);
		otherClient.ConnectTo(otherServer);
		otherServer.SetEncoding(TransportEncoding::JSON);
		otherClient.SetEncoding(TransportEncoding::JSON);
		HandlerModes modes;
		modes.blockGetSettings = m_gate;
		otherServerModule.ConfigureHandlers(modes);
		otherServer.SetWorkerPools(&m_pools);
		DrainStub drainOtherServer{&otherServer};
		ReleaseGate releaseGate{m_gate};

		AsyncInvokeLatch running;
		AsyncInvokeLatch queuedOnPaused;
		AsyncInvokeLatch queuedOnOther;
		AsnGetSettingsResult runningResult;
		AsnGetSettingsResult queuedOnPausedResult;
		AsnGetSettingsResult queuedOnOtherResult;
		AsnRequestError runningError;
		AsnRequestError queuedOnPausedError;
		AsnRequestError queuedOnOtherError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(running, runningResult, runningError, 400));
		m_gate->WaitEntered(1);
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(queuedOnPaused, queuedOnPausedResult, queuedOnPausedError, 2000));
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettingsFrom(otherClient, queuedOnOther, queuedOnOtherResult, queuedOnOtherError, 2000));

		std::thread releaser([this] {
			std::this_thread::sleep_for(std::chrono::milliseconds(150));
			m_gate->Release();
		});
		m_server.PauseRoseProcessing();
		releaser.join();

		ASSERT_TRUE(queuedOnPaused.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_REJECT_UNKNOWNOPERATION, queuedOnPaused.RoseResult());
		ASSERT_TRUE(running.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_TE_TIMEOUT, running.RoseResult());
		ASSERT_TRUE(queuedOnOther.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_NOERROR, queuedOnOther.RoseResult());
		EXPECT_EQ(2, m_gate->Entered());
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, TwoWorkersRunHandlersConcurrently)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(2, 4);

		AsyncInvokeLatch first;
		AsyncInvokeLatch second;
		AsnGetSettingsResult firstResult;
		AsnGetSettingsResult secondResult;
		AsnRequestError firstError;
		AsnRequestError secondError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(first, firstResult, firstError, 2000));
		m_gate->WaitEntered(1);
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(second, secondResult, secondError, 2000));
		m_gate->WaitEntered(2);

		EXPECT_NE(m_gate->ThreadAt(0), m_gate->ThreadAt(1));
		EXPECT_NE(std::this_thread::get_id(), m_gate->ThreadAt(0));
		EXPECT_NE(std::this_thread::get_id(), m_gate->ThreadAt(1));

		m_gate->Release();
		ASSERT_TRUE(first.WaitFor(std::chrono::milliseconds(2000)));
		ASSERT_TRUE(second.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_NOERROR, first.RoseResult());
		EXPECT_EQ(ROSE_NOERROR, second.RoseResult());
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, FullDepthCountsEveryRunningWorker)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(2, 2);

		AsyncInvokeLatch first;
		AsyncInvokeLatch second;
		AsnGetSettingsResult firstResult;
		AsnGetSettingsResult secondResult;
		AsnRequestError firstError;
		AsnRequestError secondError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(first, firstResult, firstError, 2000));
		m_gate->WaitEntered(1);
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(second, secondResult, secondError, 2000));
		m_gate->WaitEntered(2);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		EXPECT_EQ(ROSE_REJECT_QUEUE_FULL, m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 2000));
		EXPECT_EQ(2, m_gate->Entered());

		m_gate->Release();
		ASSERT_TRUE(first.WaitFor(std::chrono::milliseconds(2000)));
		ASSERT_TRUE(second.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_NOERROR, first.RoseResult());
		EXPECT_EQ(ROSE_NOERROR, second.RoseResult());
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, DuplicateOperationIdIsRejectedAndPreviousSetStays)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(1, 4);
		m_gate->Release();

		SnaccWorkerPoolConfig repeated;
		repeated.name = "again";
		repeated.maxThreads = 1;
		repeated.operationIds.push_back(OPID_asnGetSettings);
		repeated.operationIds.push_back(OPID_asnGetSettings);
		EXPECT_FALSE(m_pools.Configure({repeated}));

		SnaccWorkerPoolConfig first;
		first.name = "settings-a";
		first.operationIds.push_back(OPID_asnGetSettings);
		SnaccWorkerPoolConfig second;
		second.name = "settings-b";
		second.operationIds.push_back(OPID_asnGetSettings);
		EXPECT_FALSE(m_pools.Configure({first, second}));

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		EXPECT_EQ(ROSE_NOERROR, m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error));
		EXPECT_EQ(1, m_gate->Entered());
		EXPECT_NE(std::this_thread::get_id(), m_gate->ThreadAt(0));
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, KeepWorkerHandlesNextInvokeAfterHandlerException)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(1, 4, SnaccWorkerFaultPolicy::KeepWorker);
		HandlerModes modes;
		modes.blockGetSettings = m_gate;
		modes.throwRuntimeErrorOnGetSettings = 1;
		ConfigureServerHandlers(modes);

		AsyncInvokeLatch failed;
		AsnGetSettingsResult failedResult;
		AsnRequestError failedError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(failed, failedResult, failedError, 400));
		m_gate->WaitEntered(1);
		m_gate->Release();
		ASSERT_TRUE(failed.WaitFor(std::chrono::milliseconds(1000)));
		EXPECT_EQ(ROSE_TE_TIMEOUT, failed.RoseResult());

		AsyncInvokeLatch next;
		AsnGetSettingsResult nextResult;
		AsnRequestError nextError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(next, nextResult, nextError, 2000));
		ASSERT_TRUE(next.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_NOERROR, next.RoseResult());
		EXPECT_EQ(2, m_gate->Entered());
		EXPECT_EQ(m_gate->ThreadAt(0), m_gate->ThreadAt(1));
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, RetireWorkerStartsReplacementAfterHandlerException)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(1, 4, SnaccWorkerFaultPolicy::RetireWorker);
		HandlerModes modes;
		modes.blockGetSettings = m_gate;
		modes.throwRuntimeErrorOnGetSettings = 1;
		ConfigureServerHandlers(modes);

		AsyncInvokeLatch failed;
		AsnGetSettingsResult failedResult;
		AsnRequestError failedError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(failed, failedResult, failedError, 400));
		m_gate->WaitEntered(1);
		m_gate->Release();
		ASSERT_TRUE(failed.WaitFor(std::chrono::milliseconds(1000)));
		EXPECT_EQ(ROSE_TE_TIMEOUT, failed.RoseResult());

		AsyncInvokeLatch next;
		AsnGetSettingsResult nextResult;
		AsnRequestError nextError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(next, nextResult, nextError, 2000));
		ASSERT_TRUE(next.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_NOERROR, next.RoseResult());
		EXPECT_EQ(2, m_gate->Entered());
		EXPECT_NE(m_gate->ThreadAt(0), m_gate->ThreadAt(1));
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, ConfigureStartsOneThreadBeforeAnyInvoke)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ConfigureGetSettingsPool(4, 8);
		EXPECT_EQ(1u, m_pools.LiveThreads("settings"));
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, ExtraThreadsLeaveAfterIdleAndOneRemains)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(2, 4, SnaccWorkerFaultPolicy::KeepWorker, 80);

		AsyncInvokeLatch first;
		AsyncInvokeLatch second;
		AsnGetSettingsResult firstResult;
		AsnGetSettingsResult secondResult;
		AsnRequestError firstError;
		AsnRequestError secondError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(first, firstResult, firstError, 2000));
		m_gate->WaitEntered(1);
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(second, secondResult, secondError, 2000));
		m_gate->WaitEntered(2);
		const std::thread::id burstA = m_gate->ThreadAt(0);
		const std::thread::id burstB = m_gate->ThreadAt(1);
		EXPECT_NE(burstA, burstB);
		m_gate->Release();
		ASSERT_TRUE(first.WaitFor(std::chrono::milliseconds(2000)));
		ASSERT_TRUE(second.WaitFor(std::chrono::milliseconds(2000)));

		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		while (m_pools.LiveThreads("settings") > 1 && std::chrono::steady_clock::now() < deadline)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		EXPECT_EQ(1u, m_pools.LiveThreads("settings"));

		ArmBlockingGetSettings();
		AsyncInvokeLatch again;
		AsyncInvokeLatch spawned;
		AsnGetSettingsResult againResult;
		AsnGetSettingsResult spawnedResult;
		AsnRequestError againError;
		AsnRequestError spawnedError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(again, againResult, againError, 2000));
		m_gate->WaitEntered(1);
		const std::thread::id survivor = m_gate->ThreadAt(0);
		EXPECT_TRUE(survivor == burstA || survivor == burstB);
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(spawned, spawnedResult, spawnedError, 2000));
		m_gate->WaitEntered(2);
		const std::thread::id replacement = m_gate->ThreadAt(1);
		EXPECT_NE(survivor, replacement);
		EXPECT_EQ(2u, m_pools.LiveThreads("settings"));

		m_gate->Release();
		ASSERT_TRUE(again.WaitFor(std::chrono::milliseconds(2000)));
		ASSERT_TRUE(spawned.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_NOERROR, again.RoseResult());
		EXPECT_EQ(ROSE_NOERROR, spawned.RoseResult());
	}

} // namespace snacclib
