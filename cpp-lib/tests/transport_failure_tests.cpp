#include "test_support/sample_runtime_harness.h"

namespace sample_runtime_tests
{

// Covers failures caused by the transport layer rather than by business logic.
class TransportFailureRuntimeTest : public RuntimeTestBase
{
protected:
	// Verifies that an immediate send failure is surfaced to the caller.
	void AssertSendFailureIsReportedAsTransportFailure(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_client.Transport().EnqueueAction(TransportAction::Fail());

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 250);
		EXPECT_EQ(ROSE_TE_TRANSPORTFAILED, roseResult);
	}

	// Verifies that a lost response results in a caller-visible timeout.
	void AssertDroppedResponseTimesOut(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.Transport().EnqueueAction(TransportAction::Drop());

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 25);
		EXPECT_EQ(ROSE_TE_TIMEOUT, roseResult);
	}

	// Verifies that a stale delayed response does not poison the next invoke.
	void AssertDelayedResponseCanArriveAfterTimeoutWithoutBreakingNextCall(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.Transport().EnqueueAction(TransportAction::Queue());

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long firstRoseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 25);
		EXPECT_EQ(ROSE_TE_TIMEOUT, firstRoseResult);
		EXPECT_EQ(1u, m_server.Transport().QueuedMessageCount());

		FlushServerOutbound();

		AsnGetSettingsResult secondResult;
		const long secondRoseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &secondResult, &error, 250);
		EXPECT_EQ(ROSE_NOERROR, secondRoseResult);
		ASSERT_NE(nullptr, secondResult.settings.u8sUsername);
		EXPECT_EQ("initial-user", secondResult.settings.u8sUsername->getASCII());
	}

	// Verifies that malformed inbound data is ignored instead of corrupting later traffic.
	void AssertMalformedInboundPayloadDoesNotDispatchHandlers(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.ReceiveRaw(MalformedPayload(encoding));

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 250);
		EXPECT_EQ(ROSE_NOERROR, roseResult);
		ASSERT_NE(nullptr, result.settings.u8sUsername);
		EXPECT_EQ("initial-user", result.settings.u8sUsername->getASCII());
		EXPECT_EQ(0, ClientSettingsEventCount());
		EXPECT_EQ(0u, ClientFancyEventCount());
	}
};

// Runs the send-failure scenario over BER.
TEST_F(TransportFailureRuntimeTest, SendFailureIsReportedAsTransportFailureBer)
{
	AssertSendFailureIsReportedAsTransportFailure(TransportEncoding::BER);
}

// Runs the send-failure scenario over JSON.
TEST_F(TransportFailureRuntimeTest, SendFailureIsReportedAsTransportFailureJson)
{
	AssertSendFailureIsReportedAsTransportFailure(TransportEncoding::JSON);
}

// Runs the dropped-response timeout scenario over BER.
TEST_F(TransportFailureRuntimeTest, DroppedResponseTimesOutBer)
{
	AssertDroppedResponseTimesOut(TransportEncoding::BER);
}

// Runs the dropped-response timeout scenario over JSON.
TEST_F(TransportFailureRuntimeTest, DroppedResponseTimesOutJson)
{
	AssertDroppedResponseTimesOut(TransportEncoding::JSON);
}

// Runs the delayed-response recovery scenario over BER.
TEST_F(TransportFailureRuntimeTest, DelayedResponseCanArriveAfterTimeoutWithoutBreakingNextCallBer)
{
	AssertDelayedResponseCanArriveAfterTimeoutWithoutBreakingNextCall(TransportEncoding::BER);
}

// Runs the delayed-response recovery scenario over JSON.
TEST_F(TransportFailureRuntimeTest, DelayedResponseCanArriveAfterTimeoutWithoutBreakingNextCallJson)
{
	AssertDelayedResponseCanArriveAfterTimeoutWithoutBreakingNextCall(TransportEncoding::JSON);
}

// Runs the malformed-payload resilience scenario over BER.
TEST_F(TransportFailureRuntimeTest, MalformedInboundPayloadDoesNotDispatchHandlersBer)
{
	AssertMalformedInboundPayloadDoesNotDispatchHandlers(TransportEncoding::BER);
}

// Runs the malformed-payload resilience scenario over JSON.
TEST_F(TransportFailureRuntimeTest, MalformedInboundPayloadDoesNotDispatchHandlersJson)
{
	AssertMalformedInboundPayloadDoesNotDispatchHandlers(TransportEncoding::JSON);
}

} // namespace sample_runtime_tests
