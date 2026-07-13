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

const SnaccTelemetryData* FindOutboundWaitTelemetry(const std::vector<std::shared_ptr<const SnaccTelemetryData>>& entries, const SnaccTelemetryData::Reason reason)
{
	return FindTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Stage::OUTBOUND_WAIT, reason);
}

void ExpectOutboundWaitTelemetry(const SnaccTelemetryData* pTelemetry, const SnaccTelemetryData::Outcome outcome, const SnaccTelemetryData::Reason reason, const std::optional<long> roseResult,
	const bool bExpectRequestData, const bool bExpectResponseData)
{
	ASSERT_NE(nullptr, pTelemetry);
	EXPECT_EQ(outcome, pTelemetry->m_Outcome);
	EXPECT_EQ(SnaccTelemetryData::Stage::OUTBOUND_WAIT, pTelemetry->m_Stage);
	EXPECT_EQ(reason, pTelemetry->m_Reason);
	if (roseResult.has_value())
	{
		ASSERT_TRUE(pTelemetry->m_lRoseResult.has_value());
		EXPECT_EQ(roseResult.value(), pTelemetry->m_lRoseResult.value());
	}
	if (bExpectRequestData)
		EXPECT_GT(pTelemetry->m_stRequestData, 0u);
	if (bExpectResponseData)
	{
		ASSERT_TRUE(pTelemetry->m_stResponseData.has_value());
		EXPECT_GT(pTelemetry->m_stResponseData.value(), 0u);
	}
	EXPECT_GE(pTelemetry->m_Duration.count(), 0);
}

long SendAsyncGetSettings(RuntimeEndpoint& client, AsyncInvokeLatch& latch, AsnGetSettingsResult& result, AsnRequestError& error, int timeoutMs)
{
	AsnGetSettingsArgument argument;
	SnaccScopedInvokeMessage invokeMsg(client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
	auto pCtx = client.CreateSessionInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
	pCtx->SetInvokeTimeout(timeoutMs);
	pCtx->SetAsyncCompletion(latch.Callback(), &result, &error);
	return client.SendInvokeAsync(invokeMsg.GetPtr(), &result, &error, "asnGetSettings", std::move(pCtx));
}

long SendFireAndForgetGetSettings(RuntimeEndpoint& client)
{
	AsnGetSettingsArgument argument;
	AsnGetSettingsResult result;
	AsnRequestError error;
	SnaccScopedInvokeMessage invokeMsg(client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
	auto pCtx = client.CreateSessionInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
	pCtx->SetInvokeTimeout(0);
	return client.SendInvokeAsync(invokeMsg.GetPtr(), &result, &error, "asnGetSettings", std::move(pCtx));
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
		ExpectOutboundWaitTelemetry(FindOutboundWaitTelemetry(entries, SnaccTelemetryData::Reason::REMOTE_RESULT), SnaccTelemetryData::Outcome::RESULT,
			SnaccTelemetryData::Reason::REMOTE_RESULT, ROSE_NOERROR, true, true);
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
		pCtx->SetInvokeTimeout(250);

		auto responseFuture = std::async(std::launch::async, [&]() {
			return m_client.SendInvoke(invokeMsg.GetPtr(), &result, &error, "asnGetSettings", pCtx);
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
		const long roseResult = m_client.SendInvoke(invokeMsg.GetPtr(), &result, &error, "asnGetSettings", pCtx);
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

	// Verifies that outbound transport failures are reported at OUTBOUND_SEND.
	void AssertTransportSendFailureTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_client.Transport().EnqueueAction(TransportAction::Fail());

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 250);
		EXPECT_EQ(ROSE_TE_TRANSPORTFAILED, roseResult);

		const auto entries = TelemetryEntries();
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::OUTBOUND_SEND,
			SnaccTelemetryData::Reason::SEND_FAILED, ROSE_TE_TRANSPORTFAILED));
	}

	// Verifies that shutdown completes pending sync invokes with shutdown telemetry.
	void AssertShutdownPendingInvokeTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.Transport().EnqueueAction(TransportAction::Queue());

		auto pendingInvoke = std::async(std::launch::async, [this]() {
			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			return m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 1000);
		});

		std::this_thread::sleep_for(std::chrono::milliseconds(25));
		m_client.StopProcessing(true);

		EXPECT_EQ(ROSE_TE_SHUTDOWN, pendingInvoke.get());

		const auto entries = TelemetryEntries();
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::OUTBOUND_WAIT,
			SnaccTelemetryData::Reason::SHUTDOWN, ROSE_TE_SHUTDOWN));
		const SnaccTelemetryData* pTelemetry = FindOutboundWaitTelemetry(entries, SnaccTelemetryData::Reason::SHUTDOWN);
		ASSERT_NE(nullptr, pTelemetry);
		EXPECT_GT(pTelemetry->m_stRequestData, 0u);
	}

	// Verifies inbound settings-changed event telemetry on the client.
	void AssertSettingsChangedEventTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);

		AsnSetSettingsArgument argument;
		SetSettingsValues(argument.settings, true, "telemetry-user");
		AsnSetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeSetSettings(&argument, &result, &error);
		EXPECT_EQ(ROSE_NOERROR, roseResult);

		const auto entries = TelemetryEntries();
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::INBOUND, SnaccTelemetryData::Outcome::EVENT, SnaccTelemetryData::Stage::INBOUND_INVOKE,
			SnaccTelemetryData::Reason::LOCAL_EVENT, ROSE_NOERROR));
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Outcome::EVENT, SnaccTelemetryData::Stage::OUTBOUND_SEND,
			SnaccTelemetryData::Reason::LOCAL_EVENT, ROSE_NOERROR));
	}

	// Verifies async happy-path telemetry including request/response payload sizes.
	void AssertAsyncHappyPathTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long sendResult = SendAsyncGetSettings(m_client, latch, result, error, 500);
		ASSERT_EQ(ROSE_NOERROR, sendResult);
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(500)));
		EXPECT_EQ(ROSE_NOERROR, latch.RoseResult());

		const auto entries = TelemetryEntries();
		ExpectOutboundWaitTelemetry(FindOutboundWaitTelemetry(entries, SnaccTelemetryData::Reason::REMOTE_RESULT), SnaccTelemetryData::Outcome::RESULT,
			SnaccTelemetryData::Reason::REMOTE_RESULT, ROSE_NOERROR, true, true);
	}

	// Verifies async application-error telemetry.
	void AssertAsyncApplicationErrorTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		HandlerModes handlerModes;
		handlerModes.getSettingsReturnsError = true;
		ConfigureServerHandlers(handlerModes);

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long sendResult = SendAsyncGetSettings(m_client, latch, result, error, 500);
		ASSERT_EQ(ROSE_NOERROR, sendResult);
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(500)));

		const auto entries = TelemetryEntries();
		ExpectOutboundWaitTelemetry(FindOutboundWaitTelemetry(entries, SnaccTelemetryData::Reason::REMOTE_ERROR), SnaccTelemetryData::Outcome::ERR,
			SnaccTelemetryData::Reason::REMOTE_ERROR, ROSE_ERROR_VALUE, true, true);
	}

	// Verifies async reject telemetry.
	void AssertAsyncRejectTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		HandlerModes handlerModes;
		handlerModes.implementGetSettings = false;
		ConfigureServerHandlers(handlerModes);

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long sendResult = SendAsyncGetSettings(m_client, latch, result, error, 500);
		ASSERT_EQ(ROSE_NOERROR, sendResult);
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(500)));

		const auto entries = TelemetryEntries();
		ExpectOutboundWaitTelemetry(FindOutboundWaitTelemetry(entries, SnaccTelemetryData::Reason::REMOTE_REJECT), SnaccTelemetryData::Outcome::REJECT,
			SnaccTelemetryData::Reason::REMOTE_REJECT, ROSE_REJECT_FUNCTIONMISSING, true, true);
	}

	// Verifies async timeout telemetry.
	void AssertAsyncTimeoutTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.Transport().EnqueueAction(TransportAction::Drop());

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long sendResult = SendAsyncGetSettings(m_client, latch, result, error, 25);
		ASSERT_EQ(ROSE_NOERROR, sendResult);
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(500)));

		const auto entries = TelemetryEntries();
		ExpectOutboundWaitTelemetry(FindOutboundWaitTelemetry(entries, SnaccTelemetryData::Reason::TIMEOUT), SnaccTelemetryData::Outcome::UNHANDLED,
			SnaccTelemetryData::Reason::TIMEOUT, ROSE_TE_TIMEOUT, true, false);
	}

	// Verifies async shutdown telemetry for a pending invoke.
	void AssertAsyncShutdownTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.Transport().EnqueueAction(TransportAction::Queue());

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long sendResult = SendAsyncGetSettings(m_client, latch, result, error, 1000);
		ASSERT_EQ(ROSE_NOERROR, sendResult);

		std::this_thread::sleep_for(std::chrono::milliseconds(25));
		m_client.StopProcessing(true);

		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(500)));

		const auto entries = TelemetryEntries();
		ExpectOutboundWaitTelemetry(FindOutboundWaitTelemetry(entries, SnaccTelemetryData::Reason::SHUTDOWN), SnaccTelemetryData::Outcome::UNHANDLED,
			SnaccTelemetryData::Reason::SHUTDOWN, ROSE_TE_SHUTDOWN, true, false);
	}

	// Verifies async transport-send failure telemetry.
	void AssertAsyncTransportSendFailureTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_client.Transport().EnqueueAction(TransportAction::Fail());

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long sendResult = SendAsyncGetSettings(m_client, latch, result, error, 250);
		EXPECT_EQ(ROSE_TE_TRANSPORTFAILED, sendResult);
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(250)));

		const auto entries = TelemetryEntries();
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::OUTBOUND_SEND,
			SnaccTelemetryData::Reason::SEND_FAILED, ROSE_TE_TRANSPORTFAILED));
	}

	// Verifies async outbound request size matches the sync invoke for the same operation.
	void AssertAsyncRequestDataMatchesSyncInvoke(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult syncResult;
		AsnRequestError syncError;
		const long syncRoseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &syncResult, &syncError);
		ASSERT_EQ(ROSE_NOERROR, syncRoseResult);
		const SnaccTelemetryData* pSyncTelemetry = FindOutboundWaitTelemetry(TelemetryEntries(), SnaccTelemetryData::Reason::REMOTE_RESULT);
		ASSERT_NE(nullptr, pSyncTelemetry);
		const size_t syncRequestData = pSyncTelemetry->m_stRequestData;

		m_telemetryRecorder.Clear();

		AsyncInvokeLatch latch;
		AsnGetSettingsResult asyncResult;
		AsnRequestError asyncError;
		const long asyncSendResult = SendAsyncGetSettings(m_client, latch, asyncResult, asyncError, 500);
		ASSERT_EQ(ROSE_NOERROR, asyncSendResult);
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(500)));

		const SnaccTelemetryData* pAsyncTelemetry = FindOutboundWaitTelemetry(TelemetryEntries(), SnaccTelemetryData::Reason::REMOTE_RESULT);
		ASSERT_NE(nullptr, pAsyncTelemetry);
		EXPECT_EQ(syncRequestData, pAsyncTelemetry->m_stRequestData);
		EXPECT_GT(pAsyncTelemetry->m_stRequestData, 0u);
	}

	// Verifies async iTimeout==0 matches sync fire-and-forget telemetry (DISPATCHED/WAIT_SKIPPED).
	void AssertAsyncWaitSkippedTelemetry(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.Transport().EnqueueAction(TransportAction::Queue());

		const long sendResult = SendFireAndForgetGetSettings(m_client);
		EXPECT_EQ(ROSE_NOERROR, sendResult);
		EXPECT_EQ(1u, m_server.Transport().QueuedMessageCount());

		const auto entries = TelemetryEntries();
		EXPECT_TRUE(HasTelemetry(entries, SnaccTelemetryData::Direction::OUTBOUND, SnaccTelemetryData::Outcome::DISPATCHED, SnaccTelemetryData::Stage::OUTBOUND_WAIT,
			SnaccTelemetryData::Reason::WAIT_SKIPPED, ROSE_NOERROR));
		const SnaccTelemetryData* pTelemetry = FindOutboundWaitTelemetry(entries, SnaccTelemetryData::Reason::WAIT_SKIPPED);
		ASSERT_NE(nullptr, pTelemetry);
		EXPECT_GT(pTelemetry->m_stRequestData, 0u);
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

TEST_F(TelemetryRuntimeTest, TransportSendFailureTelemetryBer)
{
	AssertTransportSendFailureTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, TransportSendFailureTelemetryJson)
{
	AssertTransportSendFailureTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, ShutdownPendingInvokeTelemetryBer)
{
	AssertShutdownPendingInvokeTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, ShutdownPendingInvokeTelemetryJson)
{
	AssertShutdownPendingInvokeTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, SettingsChangedEventTelemetryBer)
{
	AssertSettingsChangedEventTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, SettingsChangedEventTelemetryJson)
{
	AssertSettingsChangedEventTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, AsyncHappyPathTelemetryBer)
{
	AssertAsyncHappyPathTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, AsyncHappyPathTelemetryJson)
{
	AssertAsyncHappyPathTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, AsyncApplicationErrorTelemetryBer)
{
	AssertAsyncApplicationErrorTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, AsyncRejectTelemetryJson)
{
	AssertAsyncRejectTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, AsyncTimeoutTelemetryBer)
{
	AssertAsyncTimeoutTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, AsyncTimeoutTelemetryJson)
{
	AssertAsyncTimeoutTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, AsyncShutdownTelemetryJson)
{
	AssertAsyncShutdownTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, AsyncTransportSendFailureTelemetryBer)
{
	AssertAsyncTransportSendFailureTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, AsyncTransportSendFailureTelemetryJson)
{
	AssertAsyncTransportSendFailureTelemetry(TransportEncoding::JSON);
}

TEST_F(TelemetryRuntimeTest, AsyncRequestDataMatchesSyncInvokeBer)
{
	AssertAsyncRequestDataMatchesSyncInvoke(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, AsyncWaitSkippedTelemetryBer)
{
	AssertAsyncWaitSkippedTelemetry(TransportEncoding::BER);
}

TEST_F(TelemetryRuntimeTest, AsyncWaitSkippedTelemetryJson)
{
	AssertAsyncWaitSkippedTelemetry(TransportEncoding::JSON);
}

} // namespace sample_runtime_tests
