#include "test_support/sample_runtime_harness.h"

#include <algorithm>

namespace sample_runtime_tests
{
namespace
{
bool HasTelemetry(const std::vector<std::shared_ptr<const SnaccTelemetryData>>& entries, const SnaccTelemetryData::Direction direction, const SnaccTelemetryData::Outcome outcome,
	const SnaccTelemetryData::Stage stage, const SnaccTelemetryData::Reason reason, const std::optional<long> roseResult = std::nullopt)
{
	return std::any_of(entries.begin(), entries.end(), [&](const std::shared_ptr<const SnaccTelemetryData>& entry) {
		if (!entry || entry->m_Direction != direction || entry->m_Outcome != outcome || entry->m_Stage != stage || entry->m_Reason != reason)
			return false;
		if (!roseResult.has_value())
			return true;
		return entry->m_lRoseResult.has_value() && entry->m_lRoseResult.value() == roseResult.value();
	});
}

const SnaccTelemetryData* FindTelemetry(const std::vector<std::shared_ptr<const SnaccTelemetryData>>& entries, const SnaccTelemetryData::Direction direction, const SnaccTelemetryData::Stage stage,
	const SnaccTelemetryData::Reason reason)
{
	const auto it = std::find_if(entries.begin(), entries.end(), [&](const std::shared_ptr<const SnaccTelemetryData>& entry) {
		return entry && entry->m_Direction == direction && entry->m_Stage == stage && entry->m_Reason == reason;
	});
	return it == entries.end() ? nullptr : it->get();
}
} // namespace

// Verifies that the telemetry callback sees the major lifecycle outcomes emitted
// by the runtime for success, failure, event, and custom-context scenarios.
class TelemetryRuntimeTest : public RuntimeTestBase
{
protected:
	// Verifies the normal invoke/result telemetry flow on both client and server.
	void AssertHappyPathTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
		EXPECT_EQ(ROSE_NOERROR, roseResult);

		const auto entries = TelemetryEntries();
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Outcome::RESULT, SnaccTelemetryData::Stage::OUTBOUND_WAIT,
			SnaccTelemetryData::Reason::REMOTE_RESULT, ROSE_NOERROR));
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::INBOUND, SnaccTelemetryData::Outcome::RESULT, SnaccTelemetryData::Stage::INBOUND_INVOKE,
			SnaccTelemetryData::Reason::LOCAL_RESULT, ROSE_NOERROR));
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::INBOUND, SnaccTelemetryData::Outcome::RESULT, SnaccTelemetryData::Stage::INBOUND_RESPONSE,
			SnaccTelemetryData::Reason::REMOTE_RESULT, ROSE_NOERROR));
	}

	// Verifies that application errors are reflected as both local and remote error telemetry.
	void AssertApplicationErrorTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		HandlerModes handlerModes;
		handlerModes.getSettingsReturnsError = true;
		ConfigureServerHandlers(handlerModes);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
		EXPECT_EQ(ROSE_ERROR_VALUE, roseResult);

		const auto entries = TelemetryEntries();
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Outcome::ERR, SnaccTelemetryData::Stage::OUTBOUND_WAIT,
			SnaccTelemetryData::Reason::REMOTE_ERROR, ROSE_ERROR_VALUE));
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::INBOUND, SnaccTelemetryData::Outcome::ERR, SnaccTelemetryData::Stage::INBOUND_INVOKE,
			SnaccTelemetryData::Reason::LOCAL_ERROR, ROSE_NOERROR));
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::INBOUND, SnaccTelemetryData::Outcome::ERR, SnaccTelemetryData::Stage::INBOUND_RESPONSE,
			SnaccTelemetryData::Reason::REMOTE_ERROR, ROSE_ERROR_VALUE));
	}

	// Verifies that reject flows are surfaced as local and remote reject telemetry.
	void AssertRejectTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		HandlerModes handlerModes;
		handlerModes.implementGetSettings = false;
		ConfigureServerHandlers(handlerModes);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
		EXPECT_EQ(ROSE_REJECT_FUNCTIONMISSING, roseResult);

		const auto entries = TelemetryEntries();
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Outcome::REJECT, SnaccTelemetryData::Stage::OUTBOUND_WAIT,
			SnaccTelemetryData::Reason::REMOTE_REJECT, ROSE_REJECT_FUNCTIONMISSING));
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::INBOUND, SnaccTelemetryData::Outcome::REJECT, SnaccTelemetryData::Stage::INBOUND_RESPONSE,
			SnaccTelemetryData::Reason::REMOTE_REJECT, ROSE_REJECT_FUNCTIONMISSING));
	}

	// Verifies that decode failures are reported through inbound decode telemetry.
	void AssertDecodeFailureTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.ReceiveRaw(MalformedPayload(encoding));

		const auto entries = TelemetryEntries();
		EXPECT_TRUE(std::any_of(entries.begin(), entries.end(), [](const std::shared_ptr<const SnaccTelemetryData>& entry) {
			return entry && entry->m_Direction == SnaccTelemetryData::Direction::INBOUND && entry->m_Stage == SnaccTelemetryData::Stage::INBOUND_DECODE &&
				entry->m_Reason == SnaccTelemetryData::Reason::DECODE_FAILED;
		}));
	}

	// Verifies that timeouts are reported as outbound wait failures.
	void AssertTimeoutTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.Transport().EnqueueAction(TransportAction::Drop());

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 25);
		EXPECT_EQ(ROSE_TE_TIMEOUT, roseResult);

		const auto entries = TelemetryEntries();
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::OUTBOUND_WAIT,
			SnaccTelemetryData::Reason::TIMEOUT, ROSE_TE_TIMEOUT));
	}

	// Verifies that fire-and-forget invokes are treated as successful dispatches
	// instead of unhandled failures when the caller intentionally skips waiting.
	void AssertWaitSkippedTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.Transport().EnqueueAction(TransportAction::Queue());

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 0);
		EXPECT_EQ(ROSE_NOERROR, roseResult);
		EXPECT_EQ(1U, m_server.Transport().QueuedMessageCount());

		const auto entries = TelemetryEntries();
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Outcome::DISPATCHED, SnaccTelemetryData::Stage::OUTBOUND_WAIT,
			SnaccTelemetryData::Reason::WAIT_SKIPPED, ROSE_NOERROR));
	}

	// Verifies that a valid response envelope with an undecodable payload reports
	// the caller-visible decode failure instead of the envelope type.
	void AssertResponsePayloadDecodeFailureTelemetry(const TransportEncoding encoding, const bool bErrorEnvelope)
	{
		InitializeEndpoints(encoding);
		m_client.Transport().EnqueueAction(TransportAction::Queue());

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
		auto pCtx = CreateClientInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");

		auto responseFuture = std::async(std::launch::async, [&]() {
			return m_client.SendInvoke(invokeMsg.GetPtr(), &result, &error, "asnGetSettings", 250, pCtx);
		});

		for (int i = 0; i < 20 && m_client.Transport().QueuedMessageCount() == 0; ++i)
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		ASSERT_EQ(1U, m_client.Transport().QueuedMessageCount());
		m_client.Transport().TakeQueuedMessages();

		std::string responsePayload;
		AsnBool wrongPayload(true);
		if (bErrorEnvelope)
			ASSERT_EQ(ROSE_NOERROR, m_server.EncodeError(invokeMsg.GetPtr()->invokeID, &wrongPayload, responsePayload));
		else
			ASSERT_EQ(ROSE_NOERROR, m_server.EncodeResult(invokeMsg.GetPtr()->invokeID, &wrongPayload, responsePayload));

		m_client.ReceiveRaw(responsePayload);

		const long roseResult = responseFuture.get();
		EXPECT_EQ(ROSE_RE_DECODE_FAILED, roseResult);

		const auto entries = TelemetryEntries();
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::OUTBOUND_WAIT,
			SnaccTelemetryData::Reason::DECODE_FAILED, ROSE_RE_DECODE_FAILED));
		EXPECT_FALSE(HasTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, bErrorEnvelope ? SnaccTelemetryData::Outcome::ERR : SnaccTelemetryData::Outcome::RESULT,
			SnaccTelemetryData::Stage::OUTBOUND_WAIT, bErrorEnvelope ? SnaccTelemetryData::Reason::REMOTE_ERROR : SnaccTelemetryData::Reason::REMOTE_RESULT));
	}

	// Verifies that outbound events and inbound event handling produce event telemetry.
	void AssertEventTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);

		AsnCreateFancyEventsArgument argument;
		argument.iEventDelay = 0;
		argument.iEventCount = 2;
		AsnCreateFancyEventsResult result;
		AsnRequestError error;

		const long roseResult = m_clientEventModule.InvokeCreateFancyEvents(&argument, &result, &error);
		EXPECT_EQ(ROSE_NOERROR, roseResult);

		const auto entries = TelemetryEntries();
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Outcome::EVENT, SnaccTelemetryData::Stage::OUTBOUND_SEND,
			SnaccTelemetryData::Reason::LOCAL_EVENT, ROSE_NOERROR));
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::INBOUND, SnaccTelemetryData::Outcome::EVENT, SnaccTelemetryData::Stage::INBOUND_INVOKE,
			SnaccTelemetryData::Reason::LOCAL_EVENT, ROSE_NOERROR));
	}

	// Verifies that outbound telemetry receives a cloned custom context and that
	// PrepareForTelemetry only touches the retained clone.
	void AssertCustomContextIsClonedAndPrepared(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
		auto pCtx = CreateClientInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
		pCtx->SetTelemetryNote("custom");

		EXPECT_FALSE(pCtx->WasPreparedForTelemetry());
		const long roseResult = m_client.SendInvoke(invokeMsg.GetPtr(), &result, &error, "asnGetSettings", -1, pCtx);
		EXPECT_EQ(ROSE_NOERROR, roseResult);
		EXPECT_FALSE(pCtx->WasPreparedForTelemetry());

		const auto entries = TelemetryEntries();
		const SnaccTelemetryData* pTelemetry = FindTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Stage::OUTBOUND_WAIT, SnaccTelemetryData::Reason::REMOTE_RESULT);
		ASSERT_NE(nullptr, pTelemetry);
		ASSERT_TRUE(pTelemetry->m_pctx);

		const auto* pSessionCtx = dynamic_cast<const SessionInvokeContext*>(pTelemetry->m_pctx.get());
		ASSERT_NE(nullptr, pSessionCtx);
		EXPECT_EQ("client-session", pSessionCtx->m_strLocalSessionId);
		EXPECT_EQ("client-session", pSessionCtx->m_strInvokeSessionId);
		EXPECT_TRUE(pSessionCtx->WasPreparedForTelemetry());
		EXPECT_EQ("prepared:custom", pSessionCtx->TelemetryNote());
	}
};

TEST_F(TelemetryRuntimeTest, HappyPathTelemetryBer)
{
	AssertHappyPathTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, HappyPathTelemetryJson)
{
	AssertHappyPathTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, ApplicationErrorTelemetryBer)
{
	AssertApplicationErrorTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, ApplicationErrorTelemetryJson)
{
	AssertApplicationErrorTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, RejectTelemetryBer)
{
	AssertRejectTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, RejectTelemetryJson)
{
	AssertRejectTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, DecodeFailureTelemetryBer)
{
	AssertDecodeFailureTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, DecodeFailureTelemetryJson)
{
	AssertDecodeFailureTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, TimeoutTelemetryBer)
{
	AssertTimeoutTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, TimeoutTelemetryJson)
{
	AssertTimeoutTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, WaitSkippedTelemetryBer)
{
	AssertWaitSkippedTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, WaitSkippedTelemetryJson)
{
	AssertWaitSkippedTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, ResultPayloadDecodeFailureTelemetryBer)
{
	AssertResponsePayloadDecodeFailureTelemetry(TransportEncoding::BER, false);
}

TEST_F(TelemetryRuntimeTest, ResultPayloadDecodeFailureTelemetryJson)
{
	AssertResponsePayloadDecodeFailureTelemetry(TransportEncoding::JSON, false);
}

TEST_F(TelemetryRuntimeTest, ErrorPayloadDecodeFailureTelemetryBer)
{
	AssertResponsePayloadDecodeFailureTelemetry(TransportEncoding::BER, true);
}

TEST_F(TelemetryRuntimeTest, ErrorPayloadDecodeFailureTelemetryJson)
{
	AssertResponsePayloadDecodeFailureTelemetry(TransportEncoding::JSON, true);
}

TEST_F(TelemetryRuntimeTest, EventTelemetryBer)
{
	AssertEventTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, EventTelemetryJson)
{
	AssertEventTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, CustomContextIsClonedAndPreparedBer)
{
	AssertCustomContextIsClonedAndPrepared(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, CustomContextIsClonedAndPreparedJson)
{
	AssertCustomContextIsClonedAndPrepared(TransportEncoding::JSON);
}

} // namespace sample_runtime_tests
