#include "test_support/sample_runtime_harness.h"

namespace snacclib
{

	// Arms the server override until the test returns, including on failure.
	struct RefuseInlineScope
	{
		explicit RefuseInlineScope(ObservedRuntimeEndpoint& server)
			: m_server(server)
		{
			m_server.ArmRefuseInline();
		}

		// Clears the override after the assertions, including when the test fails.
		~RefuseInlineScope()
		{
			m_server.ClearRefuseInline();
		}

		// The endpoint whose DecideInlineExecution this scope arms.
		ObservedRuntimeEndpoint& m_server;
	};

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

		// The eight outcomes. Unset members are expected to stay 0.
		struct PoolOutcomeSample
		{
			std::uint64_t incoming{};
			std::uint64_t success{};
			std::uint64_t handlerError{};
			std::uint64_t handlerReject{};
			std::uint64_t rejectedFull{};
			std::uint64_t rejectedShuttingDown{};
			std::uint64_t rejectedConnectionClosed{};
			std::uint64_t timedOut{};
			std::uint64_t replyDiscarded{};
		};

		// Waits until the settings pool matches. The worker records an outcome after the client has already seen the reply.
		template <typename Ready> ROSEWorkerPoolHealth WaitForSettings(Ready ready)
		{
			const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
			std::vector<ROSEWorkerPoolHealth> rows = m_pools.Health();
			while (std::chrono::steady_clock::now() < deadline)
			{
				for (const ROSEWorkerPoolHealth& row : rows)
					if (row.m_strName == "settings" && ready(row))
						return row;
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				rows = m_pools.Health();
			}
			for (const ROSEWorkerPoolHealth& row : rows)
				if (row.m_strName == "settings")
					return row;
			return {};
		}

		// Checks every outcome counter and the ROSE incoming identity.
		void ExpectOutcomes(const ROSEWorkerPoolHealth& row, const PoolOutcomeSample& expected)
		{
			EXPECT_EQ(expected.incoming, row.m_counters.m_ui64Incoming);
			EXPECT_EQ(expected.success, row.m_counters.m_ui64Success);
			EXPECT_EQ(expected.handlerError, row.m_counters.m_ui64HandlerError);
			EXPECT_EQ(expected.handlerReject, row.m_counters.m_ui64HandlerReject);
			EXPECT_EQ(expected.rejectedFull, row.m_counters.m_ui64RejectedFull);
			EXPECT_EQ(expected.rejectedShuttingDown, row.m_counters.m_ui64RejectedShuttingDown);
			EXPECT_EQ(expected.rejectedConnectionClosed, row.m_counters.m_ui64RejectedConnectionClosed);
			EXPECT_EQ(expected.timedOut, row.m_counters.m_ui64TimedOut);
			EXPECT_EQ(expected.replyDiscarded, row.m_counters.m_ui64ReplyDiscarded);
			const std::uint64_t ui64Outcomes = row.m_counters.m_ui64Success + row.m_counters.m_ui64HandlerError + row.m_counters.m_ui64HandlerReject + row.m_counters.m_ui64RejectedFull + row.m_counters.m_ui64RejectedShuttingDown + row.m_counters.m_ui64RejectedConnectionClosed + row.m_counters.m_ui64TimedOut + row.m_counters.m_ui64ReplyDiscarded;
			EXPECT_EQ(row.m_counters.m_ui64Incoming, static_cast<std::uint64_t>(row.m_uiWaiting) + row.m_state.m_uiRunning + ui64Outcomes);
		}

		// A cumulative histogram: each wider bound holds at least the previous count, and the last bound equals the count.
		void ExpectHistogramFilled(const LatencyHistogram& histogram, std::uint64_t ui64Count, bool bExpectPositiveSum)
		{
			EXPECT_EQ(ui64Count, histogram.m_ui64Count);
			EXPECT_EQ(histogram.m_ui64Count, histogram.m_buckets.back());
			for (std::size_t i = 1; i < histogram.m_buckets.size(); ++i)
				EXPECT_LE(histogram.m_buckets[i - 1], histogram.m_buckets[i]);
			if (bExpectPositiveSum)
				EXPECT_GT(histogram.m_ui64SumUs, 0u);
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

	TEST_F(snacclib_WorkerPoolRuntimeTest, RefuseInlineExecutionRejectsUnpooledInvoke)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		m_gate->Release();
		RefuseInlineScope gate(m_server);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		EXPECT_EQ(ROSE_REJECT_QUEUE_FULL, m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error));
		EXPECT_EQ(0, m_gate->Entered());
		EXPECT_EQ(1, m_server.RefuseInlineCalls());
		EXPECT_EQ(0, m_server.RefuseInlineEvents());
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, RefuseInlineExecutionDropsUnpooledEvent)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		m_gate->Release();
		RefuseInlineScope gate(m_server);

		AsnGetSettingsArgument argument;
		SnaccScopedInvokeMessage invokeMsg(99999, OPID_asnGetSettings, &argument);
		EXPECT_EQ(ROSE_NOERROR, m_client.SendEvent(invokeMsg.GetPtr(), "asnGetSettings"));
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		EXPECT_EQ(0, m_gate->Entered());
		EXPECT_EQ(1, m_server.RefuseInlineCalls());
		EXPECT_EQ(1, m_server.RefuseInlineEvents());
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, PooledInvokeIsQueuedWhenInlineWouldBeRefused)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(1, 4);
		RefuseInlineScope gate(m_server);

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(latch, result, error, 2000));
		m_gate->WaitEntered(1);
		EXPECT_EQ(1, m_gate->Entered());
		EXPECT_NE(std::this_thread::get_id(), m_gate->ThreadAt(0));
		EXPECT_EQ(0, m_server.RefuseInlineCalls());
		m_gate->Release();
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_NOERROR, latch.RoseResult());
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
		const auto fullDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		std::vector<ROSEWorkerPoolHealth> full = m_pools.Health();
		while ((full.empty() || full[0].m_counters.m_ui64RejectedFull < 1) && std::chrono::steady_clock::now() < fullDeadline)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			full = m_pools.Health();
		}
		EXPECT_EQ(1, m_gate->Entered());
		ASSERT_EQ(1u, full.size());
		EXPECT_EQ(1u, full[0].m_counters.m_ui64RejectedFull);
		EXPECT_EQ(1u, full[0].m_wait.m_ui64Count);

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

	TEST_F(snacclib_WorkerPoolRuntimeTest, ZeroTimeoutQueuedInvokeStillRunsHandler)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ArmBlockingGetSettings();
		ConfigureGetSettingsPool(1, 4);

		AsyncInvokeLatch running;
		AsnGetSettingsResult runningResult;
		AsnRequestError runningError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(running, runningResult, runningError, 2000));
		m_gate->WaitEntered(1);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		ASSERT_EQ(ROSE_NOERROR, m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 0));
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		EXPECT_EQ(1, m_gate->Entered());

		m_gate->Release();
		ASSERT_TRUE(running.WaitFor(std::chrono::milliseconds(1000)));
		EXPECT_EQ(ROSE_NOERROR, running.RoseResult());

		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
		while (m_gate->Entered() < 2 && std::chrono::steady_clock::now() < deadline)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		EXPECT_EQ(2, m_gate->Entered());
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

	TEST_F(snacclib_WorkerPoolRuntimeTest, HealthSnapshotCountsThePoolNotEachThread)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ConfigureGetSettingsPool(2, 2);
		const auto idleDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		std::vector<ROSEWorkerPoolHealth> idle = m_pools.Health();
		while ((idle.empty() || idle[0].m_state.m_uiIdleThreads < 1) && std::chrono::steady_clock::now() < idleDeadline)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			idle = m_pools.Health();
		}
		ASSERT_EQ(1u, idle.size());
		EXPECT_EQ("settings", idle[0].m_strName);
		EXPECT_EQ(2u, idle[0].m_uiMaxThreads);
		EXPECT_EQ(0u, idle[0].m_state.m_uiRunning);
		EXPECT_EQ(1u, idle[0].m_state.m_uiIdleThreads);
		EXPECT_EQ(0u, idle[0].m_counters.m_ui64Incoming);
		EXPECT_EQ(0u, idle[0].m_counters.m_ui64RejectedFull);
		EXPECT_EQ(1u, idle[0].m_ui64ThreadsStarted);
		EXPECT_EQ(0u, idle[0].m_ui64ThreadsStopped);
		EXPECT_EQ(std::optional<unsigned int>(2), idle[0].m_uiMaxElements);
		EXPECT_EQ(0u, idle[0].m_uiWaiting);

		ArmBlockingGetSettings();
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

		const std::vector<ROSEWorkerPoolHealth> busy = m_pools.Health();
		ASSERT_EQ(1u, busy.size());
		EXPECT_EQ(2u, busy[0].m_state.m_uiRunning);
		EXPECT_EQ(0u, busy[0].m_state.m_uiIdleThreads);
		EXPECT_EQ(1u, busy[0].m_counters.m_ui64RejectedFull);
		EXPECT_EQ(2u, busy[0].m_ui64ThreadsStarted);
		EXPECT_EQ(0u, busy[0].m_uiWaiting);
		EXPECT_EQ(3u, busy[0].m_counters.m_ui64Incoming);
		EXPECT_EQ(0u, busy[0].m_counters.m_ui64Success);
		EXPECT_EQ(busy[0].m_counters.m_ui64Incoming, busy[0].m_uiWaiting + busy[0].m_state.m_uiRunning + busy[0].m_counters.m_ui64RejectedFull);

		m_gate->Release();
		ASSERT_TRUE(first.WaitFor(std::chrono::milliseconds(2000)));
		ASSERT_TRUE(second.WaitFor(std::chrono::milliseconds(2000)));

		// The completion latch fires inside the handler, before the worker records the outcome.
		const auto doneDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		std::vector<ROSEWorkerPoolHealth> done = m_pools.Health();
		while ((done.empty() || done[0].m_counters.m_ui64Success < 2) && std::chrono::steady_clock::now() < doneDeadline)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			done = m_pools.Health();
		}
		ASSERT_EQ(1u, done.size());
		EXPECT_EQ(3u, done[0].m_counters.m_ui64Incoming);
		EXPECT_EQ(2u, done[0].m_counters.m_ui64Success);
		EXPECT_EQ(2u, done[0].m_counters.m_run.m_ui64Count);
		EXPECT_EQ(2u, done[0].m_wait.m_ui64Count);
		EXPECT_EQ(0u, done[0].m_state.m_uiRunning);
		EXPECT_EQ(0u, done[0].m_uiWaiting);
		EXPECT_EQ(1u, done[0].m_counters.m_ui64RejectedFull);
		EXPECT_EQ(done[0].m_counters.m_ui64Incoming, done[0].m_uiWaiting + done[0].m_state.m_uiRunning + done[0].m_counters.m_ui64Success + done[0].m_counters.m_ui64RejectedFull);
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, HealthCountsHandlerError)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ConfigureGetSettingsPool(1, 4);
		HandlerModes modes;
		modes.getSettingsReturnsError = true;
		ConfigureServerHandlers(modes);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		EXPECT_EQ(ROSE_ERROR_VALUE, m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error));

		const ROSEWorkerPoolHealth row = WaitForSettings([](const ROSEWorkerPoolHealth& sample) { return sample.m_counters.m_ui64HandlerError >= 1; });
		ExpectOutcomes(row, PoolOutcomeSample{.incoming = 1, .handlerError = 1});
		ExpectHistogramFilled(row.m_counters.m_run, 1, false);
		ExpectHistogramFilled(row.m_wait, 1, false);
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, HealthCountsHandlerReject)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ConfigureGetSettingsPool(1, 4);
		HandlerModes modes;
		modes.implementGetSettings = false;
		ConfigureServerHandlers(modes);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		EXPECT_EQ(ROSE_REJECT_FUNCTIONMISSING, m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error));

		const ROSEWorkerPoolHealth row = WaitForSettings([](const ROSEWorkerPoolHealth& sample) { return sample.m_counters.m_ui64HandlerReject >= 1; });
		ExpectOutcomes(row, PoolOutcomeSample{.incoming = 1, .handlerReject = 1});
		// The handler was entered, so the reject adds one run observation.
		ExpectHistogramFilled(row.m_counters.m_run, 1, false);
		ExpectHistogramFilled(row.m_wait, 1, false);
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, HealthCountsUncaughtExceptionAsHandlerError)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		ConfigureGetSettingsPool(1, 4);
		HandlerModes modes;
		modes.throwRuntimeErrorOnGetSettings = 1;
		ConfigureServerHandlers(modes);

		AsyncInvokeLatch failed;
		AsnGetSettingsResult failedResult;
		AsnRequestError failedError;
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(failed, failedResult, failedError, 400));
		ASSERT_TRUE(failed.WaitFor(std::chrono::milliseconds(1000)));

		const ROSEWorkerPoolHealth row = WaitForSettings([](const ROSEWorkerPoolHealth& sample) { return sample.m_counters.m_ui64HandlerError >= 1; });
		ExpectOutcomes(row, PoolOutcomeSample{.incoming = 1, .handlerError = 1});
		ExpectHistogramFilled(row.m_counters.m_run, 1, false);
		ExpectHistogramFilled(row.m_wait, 1, false);
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, HealthCountsTimedOut)
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
		ASSERT_TRUE(expired.WaitFor(std::chrono::milliseconds(1000)));
		EXPECT_EQ(ROSE_NOERROR, running.RoseResult());
		EXPECT_EQ(ROSE_TE_TIMEOUT, expired.RoseResult());

		const ROSEWorkerPoolHealth row = WaitForSettings([](const ROSEWorkerPoolHealth& sample) { return sample.m_counters.m_ui64TimedOut >= 1 && sample.m_counters.m_ui64Success >= 1; });
		ExpectOutcomes(row, PoolOutcomeSample{.incoming = 2, .success = 1, .timedOut = 1});
		// The expired request left the queue and did not run the handler.
		ExpectHistogramFilled(row.m_counters.m_run, 1, false);
		ExpectHistogramFilled(row.m_wait, 2, false);
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, HealthCountsConnectionClosedAndDiscardedReply)
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
		ASSERT_TRUE(running.WaitFor(std::chrono::milliseconds(2000)));
		EXPECT_EQ(ROSE_REJECT_UNKNOWNOPERATION, queued.RoseResult());
		EXPECT_EQ(ROSE_TE_TIMEOUT, running.RoseResult());

		const ROSEWorkerPoolHealth row = WaitForSettings([](const ROSEWorkerPoolHealth& sample) { return sample.m_counters.m_ui64RejectedConnectionClosed >= 1 && sample.m_counters.m_ui64ReplyDiscarded >= 1; });
		ExpectOutcomes(row, PoolOutcomeSample{.incoming = 2, .rejectedConnectionClosed = 1, .replyDiscarded = 1});
		// The queued request never ran. The running handler finished and its reply was dropped, so it has one run observation.
		ExpectHistogramFilled(row.m_counters.m_run, 1, false);
		ExpectHistogramFilled(row.m_wait, 2, false);
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, HealthCountsRejectedShuttingDown)
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
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(running, runningResult, runningError, 500));
		m_gate->WaitEntered(1);
		ASSERT_EQ(ROSE_NOERROR, SendAsyncGetSettings(queued, queuedResult, queuedError, 2000));
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		EXPECT_EQ(1, m_gate->Entered());

		std::thread releaser([this] {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			m_gate->Release();
		});
		m_pools.Shutdown();
		releaser.join();
		ASSERT_TRUE(queued.WaitFor(std::chrono::milliseconds(2000)));
		ASSERT_TRUE(running.WaitFor(std::chrono::milliseconds(2000)));

		const std::vector<ROSEWorkerPoolHealth> rows = m_pools.Health();
		ASSERT_EQ(1u, rows.size());
		EXPECT_EQ("settings", rows[0].m_strName);
		ExpectOutcomes(rows[0], PoolOutcomeSample{.incoming = 2, .rejectedShuttingDown = 1, .replyDiscarded = 1});
		ExpectHistogramFilled(rows[0].m_counters.m_run, 1, false);
		ExpectHistogramFilled(rows[0].m_wait, 2, false);
		EXPECT_EQ(0u, rows[0].m_uiWaiting);
		EXPECT_EQ(0u, rows[0].m_state.m_uiRunning);
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, HealthCountsRunAndWaitHistograms)
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
		std::this_thread::sleep_for(std::chrono::milliseconds(40));

		const ROSEWorkerPoolHealth busy = WaitForSettings([](const ROSEWorkerPoolHealth& sample) { return sample.m_uiWaiting >= 1 && sample.m_state.m_uiRunning >= 1; });
		ExpectOutcomes(busy, PoolOutcomeSample{.incoming = 2});
		EXPECT_EQ(1u, busy.m_state.m_uiRunning);
		EXPECT_EQ(1u, busy.m_uiWaiting);
		ASSERT_TRUE(busy.m_state.m_ui64OldestRunningUs.has_value());
		ASSERT_TRUE(busy.m_ui64OldestWaitingUs.has_value());
		EXPECT_GT(*busy.m_state.m_ui64OldestRunningUs, 0u);
		EXPECT_GT(*busy.m_ui64OldestWaitingUs, 0u);

		m_gate->Release();
		ASSERT_TRUE(first.WaitFor(std::chrono::milliseconds(2000)));
		ASSERT_TRUE(second.WaitFor(std::chrono::milliseconds(2000)));

		const ROSEWorkerPoolHealth done = WaitForSettings([](const ROSEWorkerPoolHealth& sample) { return sample.m_counters.m_ui64Success >= 2; });
		ExpectOutcomes(done, PoolOutcomeSample{.incoming = 2, .success = 2});
		ExpectHistogramFilled(done.m_counters.m_run, 2, true);
		ExpectHistogramFilled(done.m_wait, 2, true);
		EXPECT_FALSE(done.m_state.m_ui64OldestRunningUs.has_value());
		EXPECT_FALSE(done.m_ui64OldestWaitingUs.has_value());
	}

	TEST_F(snacclib_WorkerPoolRuntimeTest, HealthCountsThreadsStopped)
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
		m_gate->Release();
		ASSERT_TRUE(first.WaitFor(std::chrono::milliseconds(2000)));
		ASSERT_TRUE(second.WaitFor(std::chrono::milliseconds(2000)));

		const ROSEWorkerPoolHealth row = WaitForSettings([](const ROSEWorkerPoolHealth& sample) { return sample.m_ui64ThreadsStopped >= 1 && sample.m_counters.m_ui64Success >= 2; });
		ExpectOutcomes(row, PoolOutcomeSample{.incoming = 2, .success = 2});
		EXPECT_EQ(2u, row.m_ui64ThreadsStarted);
		EXPECT_EQ(1u, row.m_ui64ThreadsStopped);
		ExpectHistogramFilled(row.m_counters.m_run, 2, false);
		ExpectHistogramFilled(row.m_wait, 2, false);
	}

} // namespace snacclib
