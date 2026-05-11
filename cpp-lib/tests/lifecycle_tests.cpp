#include "test_support/sample_runtime_harness.h"

namespace sample_runtime_tests
{

// Covers runtime lifecycle edge cases such as shutdown while work is pending.
class LifecycleRuntimeTest : public RuntimeTestBase
{
protected:
	// Verifies that stopping the client runtime completes a blocked invoke with shutdown.
	void AssertStopProcessingCompletesPendingInvokeWithShutdown(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.Transport().EnqueueAction(TransportAction::Queue());
		auto pendingInvoke = InvokeGetSettingsAsync(1000);

		std::this_thread::sleep_for(std::chrono::milliseconds(25));
		m_client.StopProcessing(true);

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
TEST_F(LifecycleRuntimeTest, StopProcessingCompletesPendingInvokeWithShutdownBer)
{
	AssertStopProcessingCompletesPendingInvokeWithShutdown(TransportEncoding::BER);
}

// Runs the shutdown-during-pending-invoke scenario over JSON.
TEST_F(LifecycleRuntimeTest, StopProcessingCompletesPendingInvokeWithShutdownJson)
{
	AssertStopProcessingCompletesPendingInvokeWithShutdown(TransportEncoding::JSON);
}

// Runs the fresh-fixture recovery scenario over BER.
TEST_F(LifecycleRuntimeTest, PendingInvokeCanRecoverAfterShutdownOnNextFixtureSetupBer)
{
	AssertPendingInvokeCanRecoverAfterShutdownOnNextFixtureSetup(TransportEncoding::BER);
}

// Runs the fresh-fixture recovery scenario over JSON.
TEST_F(LifecycleRuntimeTest, PendingInvokeCanRecoverAfterShutdownOnNextFixtureSetupJson)
{
	AssertPendingInvokeCanRecoverAfterShutdownOnNextFixtureSetup(TransportEncoding::JSON);
}

} // namespace sample_runtime_tests
