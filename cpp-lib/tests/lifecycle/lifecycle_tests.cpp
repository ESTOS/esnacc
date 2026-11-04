#include "test_support/sample_runtime_harness.h"

namespace snacclib
{

	// Covers runtime lifecycle edge cases such as shutdown while work is pending.
	class snacclib_LifecycleRuntimeTest : public RuntimeTestBase
	{
	protected:
		// Verifies that stopping the client runtime completes a blocked invoke with shutdown.
		void AssertPauseRoseProcessingCompletesPendingInvokeWithShutdown(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			m_server.Transport().EnqueueAction(TransportAction::Queue());
			auto pendingInvoke = InvokeGetSettingsAsync(1000);

			std::this_thread::sleep_for(std::chrono::milliseconds(25));
			m_client.PauseRoseProcessing();

			EXPECT_EQ(ROSE_TE_SHUTDOWN, pendingInvoke.get());
		}

		// Verifies that a fixture recreated after a failed/pending call starts cleanly.
		void AssertPendingInvokeCanRecoverAfterShutdownOnNextFixtureSetup(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			m_server.Transport().EnqueueAction(TransportAction::Drop());

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;

			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 25);
			EXPECT_EQ(ROSE_TE_TIMEOUT, roseResult);
		}
	};

	// Runs the shutdown-during-pending-invoke scenario over BER.
	TEST_F(snacclib_LifecycleRuntimeTest, PauseRoseProcessingCompletesPendingInvokeWithShutdownBer)
	{
		AssertPauseRoseProcessingCompletesPendingInvokeWithShutdown(TransportEncoding::BER);
	}

	// Runs the shutdown-during-pending-invoke scenario over JSON.
	TEST_F(snacclib_LifecycleRuntimeTest, PauseRoseProcessingCompletesPendingInvokeWithShutdownJson)
	{
		AssertPauseRoseProcessingCompletesPendingInvokeWithShutdown(TransportEncoding::JSON);
	}

	// Runs the fresh-fixture recovery scenario over BER.
	TEST_F(snacclib_LifecycleRuntimeTest, PendingInvokeCanRecoverAfterShutdownOnNextFixtureSetupBer)
	{
		AssertPendingInvokeCanRecoverAfterShutdownOnNextFixtureSetup(TransportEncoding::BER);
	}

	// Runs the fresh-fixture recovery scenario over JSON.
	TEST_F(snacclib_LifecycleRuntimeTest, PendingInvokeCanRecoverAfterShutdownOnNextFixtureSetupJson)
	{
		AssertPendingInvokeCanRecoverAfterShutdownOnNextFixtureSetup(TransportEncoding::JSON);
	}

	// ProCall calls ResumeRoseProcessing from OnConnect after PauseRoseProcessing on disconnect.
	// The next invoke and event on the same stub must leave the shutdown gate.
	TEST_F(snacclib_LifecycleRuntimeTest, ResumeRoseProcessingAllowsNextOutboundInvoke)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		m_client.PauseRoseProcessing();

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		EXPECT_EQ(ROSE_TE_SHUTDOWN, m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 250));

		m_client.ResumeRoseProcessing();
		EXPECT_EQ(ROSE_NOERROR, m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 250));

		ENetUC_Settings_ManagerROSE component(&m_client);
		AsnSettingsChangedArgument eventArgument;
		SetSettingsValues(eventArgument.settings, true, "after-resume");
		EXPECT_EQ(ROSE_NOERROR, component.Event_asnSettingsChanged(&eventArgument));
	}

	// A result that arrives after pause already completed the invoke stays ignored.
	// The following session, opened with ResumeRoseProcessing, still completes a new invoke.
	TEST_F(snacclib_LifecycleRuntimeTest, LateResultAfterPauseDoesNotResurrectPendingInvoke)
	{
		InitializeEndpoints(TransportEncoding::JSON);
		m_server.Transport().EnqueueAction(TransportAction::Queue());
		auto pendingInvoke = InvokeGetSettingsAsync(1000);

		for (int i = 0; i < 20 && m_server.Transport().QueuedMessageCount() == 0; ++i)
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		ASSERT_EQ(1u, m_server.Transport().QueuedMessageCount());

		m_client.PauseRoseProcessing();
		EXPECT_EQ(ROSE_TE_SHUTDOWN, pendingInvoke.get());

		m_server.Transport().FlushQueuedMessages();

		m_client.ResumeRoseProcessing();
		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		EXPECT_EQ(ROSE_NOERROR, m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 250));
	}

} // namespace snacclib
