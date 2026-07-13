#include "test_support/sample_runtime_harness.h"

#include <algorithm>
#include <thread>

namespace sample_runtime_tests
{
	namespace
	{
		const SessionInvokeContext* AsSessionContext(const SnaccInvokeContext* pContext)
		{
			return dynamic_cast<const SessionInvokeContext*>(pContext);
		}

		const SnaccTelemetryData* FindOutboundWaitTelemetry(const std::vector<std::shared_ptr<const SnaccTelemetryData>>& entries, const SnaccTelemetryData::Reason reason)
		{
			const auto it = std::find_if(entries.begin(), entries.end(), [&](const std::shared_ptr<const SnaccTelemetryData>& entry) { return entry && entry->m_Direction == SnaccTelemetryData::Direction::OUTBOUND && entry->m_Stage == SnaccTelemetryData::Stage::OUTBOUND_WAIT && entry->m_Reason == reason; });
			return it == entries.end() ? nullptr : it->get();
		}

		bool HasInboundResponseTelemetryForOperation(const std::vector<std::shared_ptr<const SnaccTelemetryData>>& entries, const unsigned int operationId)
		{
			return std::any_of(entries.begin(), entries.end(), [&](const std::shared_ptr<const SnaccTelemetryData>& entry) { return entry && entry->m_Direction == SnaccTelemetryData::Direction::INBOUND && entry->m_Stage == SnaccTelemetryData::Stage::INBOUND_RESPONSE && entry->m_uiOperationID == operationId; });
		}
	} // namespace

	// Specifies custom SnaccInvokeContext lifecycle for inbound and outbound ROSE paths.
	class InvokeContextRuntimeTest : public RuntimeTestBase
	{
	protected:
		void AssertInboundHandlerReceivesCustomInvokeContext(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;

			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
			EXPECT_EQ(ROSE_NOERROR, roseResult);

			const auto& handlerSnapshot = ServerInboundObservation().HandlerSnapshot();
			ASSERT_TRUE(handlerSnapshot.WasCaptured());
			EXPECT_TRUE(handlerSnapshot.IsSessionContext()) << "inbound handler must receive CreateInvokeContext() product";
			EXPECT_EQ("server-session", handlerSnapshot.LocalSessionId());
			EXPECT_EQ("client-session", handlerSnapshot.InvokeSessionId());
		}

		void AssertInboundResponseSendReusesHandlerInvokeContext(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;

			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
			EXPECT_EQ(ROSE_NOERROR, roseResult);

			EXPECT_TRUE(ServerInboundObservation().HandlerWasInvoked());
			EXPECT_GT(ServerInboundObservation().TransportSendCount(), 0u);
			EXPECT_TRUE(ServerInboundObservation().SameInstanceHandlerToTransport()) << "inbound response send must reuse the same invoke context instance seen in the handler";
		}

		void AssertInboundHandlerRejectReusesInvokeContext(const TransportEncoding encoding)
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

			EXPECT_TRUE(ServerInboundObservation().HandlerWasInvoked());
			EXPECT_GT(ServerInboundObservation().TransportSendCount(), 0u);
			EXPECT_TRUE(ServerInboundObservation().SameInstanceHandlerToTransport()) << "inbound reject send must reuse the same invoke context instance seen in the handler";
		}

		void AssertInboundApplicationErrorReusesInvokeContext(const TransportEncoding encoding)
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

			EXPECT_TRUE(ServerInboundObservation().HandlerWasInvoked());
			EXPECT_GT(ServerInboundObservation().TransportSendCount(), 0u);
			EXPECT_TRUE(ServerInboundObservation().SameInstanceHandlerToTransport()) << "inbound error response send must reuse the same invoke context instance seen in the handler";
		}

		void AssertInboundResponseSendFailureKeepsInvokeContextOnAttemptedSend(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			m_server.Transport().EnqueueAction(TransportAction::Fail());

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;

			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, 250);
			EXPECT_EQ(ROSE_TE_TIMEOUT, roseResult) << "client must time out when the inbound response never arrives; ROSE_TE_TRANSPORTFAILED applies only to a failed send on the caller side";

			EXPECT_TRUE(ServerInboundObservation().HandlerWasInvoked());
			EXPECT_GT(ServerInboundObservation().TransportSendCount(), 0u);
			EXPECT_TRUE(ServerInboundObservation().SameInstanceHandlerToTransport()) << "failed inbound response send must still pass the handler invoke context to transport";
		}

		void AssertUnknownInboundOperationRejectUsesCustomInvokeContext(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);

			const long roseResult = m_clientSettingsModule.InvokeUnknownOperation();
			EXPECT_EQ(ROSE_REJECT_UNKNOWNOPERATION, roseResult);
			EXPECT_FALSE(ServerInboundObservation().HandlerWasInvoked()) << "module handler must not run for unknown operations";

			const auto& transportSnapshot = ServerInboundObservation().LastTransportSnapshot();
			ASSERT_TRUE(transportSnapshot.WasCaptured());
			EXPECT_TRUE(transportSnapshot.IsSessionContext()) << "inbound reject must still use CreateInvokeContext() product";
			EXPECT_EQ("server-session", transportSnapshot.LocalSessionId());
		}

		void AssertUnparsableInboundDoesNotReachHandler(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			m_server.ReceiveRaw(MalformedPayload(encoding));

			EXPECT_FALSE(ServerInboundObservation().HandlerWasInvoked());
			auto count = ServerInboundObservation().TransportSendCount();
			EXPECT_EQ(0u, count);
		}

		void AssertOutboundCallerContextReachesTransport(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
			auto pContext = CreateClientInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
			pContext->SetTelemetryNote("outbound-request");

			const long roseResult = m_clientSettingsModule.InvokeGetSettingsWithContext(&argument, &result, &error, pContext);
			EXPECT_EQ(ROSE_NOERROR, roseResult);

			const auto& transportSnapshot = ClientOutboundTransportObservation().LastTransportSnapshot();
			ASSERT_TRUE(transportSnapshot.WasCaptured());
			EXPECT_TRUE(transportSnapshot.IsSessionContext());
			EXPECT_EQ("client-session", transportSnapshot.LocalSessionId());
			EXPECT_EQ("outbound-request", transportSnapshot.TelemetryNote());
			EXPECT_EQ(pContext.get(), ClientOutboundTransportObservation().LastContextAddress());
		}

		void AssertOutboundStubCreatedContextReachesTransport(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;

			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
			EXPECT_EQ(ROSE_NOERROR, roseResult);

			const auto& transportSnapshot = ClientOutboundTransportObservation().LastTransportSnapshot();
			ASSERT_TRUE(transportSnapshot.WasCaptured());
			EXPECT_TRUE(transportSnapshot.IsSessionContext()) << "outbound invoke without caller context must use CreateInvokeContext() product";
			EXPECT_EQ("client-session", transportSnapshot.LocalSessionId());
			EXPECT_EQ("client-session", transportSnapshot.InvokeSessionId());
		}

		void AssertOutboundCallerContextSurvivesUntilCallerScopeEnds(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
			auto pContext = CreateClientInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
			pContext->SetTelemetryNote("caller-owned");

			const long roseResult = m_clientSettingsModule.InvokeGetSettingsWithContext(&argument, &result, &error, pContext);
			EXPECT_EQ(ROSE_NOERROR, roseResult);
			EXPECT_FALSE(pContext->WasPreparedForTelemetry());
			EXPECT_EQ("caller-owned", pContext->TelemetryNote());

			const SnaccTelemetryData* pTelemetry = FindOutboundWaitTelemetry(TelemetryEntries(), SnaccTelemetryData::Reason::REMOTE_RESULT);
			ASSERT_NE(nullptr, pTelemetry);
			ASSERT_TRUE(pTelemetry->m_pctx);
			const auto* pSessionTelemetryContext = AsSessionContext(pTelemetry->m_pctx.get());
			ASSERT_NE(nullptr, pSessionTelemetryContext);
			EXPECT_TRUE(pSessionTelemetryContext->WasPreparedForTelemetry());
			EXPECT_EQ("prepared:caller-owned", pSessionTelemetryContext->TelemetryNote());
		}

		void AssertOutboundEncodeFailureStillUsesCallerContext(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);

			AsnSetSettingsArgument argument;
			SetSettingsValues(argument.settings, true, std::string(10'000'000, 'x'));
			AsnSetSettingsResult result;
			AsnRequestError error;
			SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnSetSettings, &argument);
			auto pContext = CreateClientInvokeContext(invokeMsg.GetPtr(), "asnSetSettings");
			pContext->SetTelemetryNote("encode-failure");
			pContext->SetInvokeTimeout(250);

			const long roseResult = m_client.SendInvoke(invokeMsg.GetPtr(), &result, &error, "asnSetSettings", pContext);
			EXPECT_EQ(ROSE_TE_ENCODE_FAILED, roseResult);
			EXPECT_FALSE(pContext->WasPreparedForTelemetry());
			EXPECT_EQ("encode-failure", pContext->TelemetryNote());
			EXPECT_EQ(0u, ClientOutboundTransportObservation().TransportSendCount()) << "encode failure must occur before transport send";
		}

		void AssertNoTransportReturnsTransportFailedForInvoke(const TransportEncoding encoding)
		{
			ObservedRuntimeEndpoint endpoint{L"NoTransportEndpoint", "solo-session"};
			endpoint.SetEncoding(encoding);
			SettingsServiceModule serviceModule(endpoint);
			ENetUC_Settings_ManagerROSE component(&endpoint);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			SnaccScopedInvokeMessage invokeMsg(endpoint.GetNextInvokeID(), OPID_asnGetSettings, &argument);
			auto pContext = endpoint.CreateSessionInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
			pContext->SetTelemetryNote("no-transport");

			const long roseResult = component.Invoke_asnGetSettings(&argument, &result, &error, pContext);
			EXPECT_EQ(ROSE_TE_TRANSPORTFAILED, roseResult);
			EXPECT_FALSE(pContext->WasPreparedForTelemetry());
			EXPECT_EQ("no-transport", pContext->TelemetryNote());
		}

		void AssertNoTransportReturnsTransportFailedForEvent(const TransportEncoding encoding)
		{
			ObservedRuntimeEndpoint endpoint{L"NoTransportEndpoint", "solo-session"};
			endpoint.SetEncoding(encoding);
			SettingsServiceModule serviceModule(endpoint);
			ENetUC_Settings_ManagerROSE component(&endpoint);

			AsnSettingsChangedArgument eventArgument;
			SetSettingsValues(eventArgument.settings, true, "event-without-transport");
			auto pContext = endpoint.CreateSessionInvokeContext(nullptr, "asnSettingsChanged");
			pContext->SetTelemetryNote("event-no-transport");

			const long roseResult = component.Event_asnSettingsChanged(&eventArgument, pContext);
			EXPECT_EQ(ROSE_TE_TRANSPORTFAILED, roseResult);
			EXPECT_FALSE(pContext->WasPreparedForTelemetry());
			EXPECT_EQ("event-no-transport", pContext->TelemetryNote());
		}

		void AssertOutboundTransportFailureKeepsCallerContextOnSendAttempt(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			m_client.Transport().EnqueueAction(TransportAction::Fail());

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
			auto pContext = CreateClientInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
			pContext->SetTelemetryNote("transport-failure");
			pContext->SetInvokeTimeout(250);

			const long roseResult = m_clientSettingsModule.InvokeGetSettingsWithContext(&argument, &result, &error, pContext);
			EXPECT_EQ(ROSE_TE_TRANSPORTFAILED, roseResult);
			ASSERT_GT(ClientOutboundTransportObservation().TransportSendCount(), 0u);
			EXPECT_EQ(pContext.get(), ClientOutboundTransportObservation().LastContextAddress());
		}

		void AssertOutboundTimeoutKeepsCallerContextOnRequestSend(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			m_server.Transport().EnqueueAction(TransportAction::Drop());

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
			auto pContext = CreateClientInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
			pContext->SetTelemetryNote("timeout");
			pContext->SetInvokeTimeout(25);

			const long roseResult = m_clientSettingsModule.InvokeGetSettingsWithContext(&argument, &result, &error, pContext);
			EXPECT_EQ(ROSE_TE_TIMEOUT, roseResult);
			ASSERT_GT(ClientOutboundTransportObservation().TransportSendCount(), 0u);
			EXPECT_EQ(pContext.get(), ClientOutboundTransportObservation().LastContextAddress());

			const SnaccTelemetryData* pTelemetry = FindOutboundWaitTelemetry(TelemetryEntries(), SnaccTelemetryData::Reason::TIMEOUT);
			ASSERT_NE(nullptr, pTelemetry);
			ASSERT_TRUE(pTelemetry->m_pctx);
			const auto* pSessionTelemetryContext = AsSessionContext(pTelemetry->m_pctx.get());
			ASSERT_NE(nullptr, pSessionTelemetryContext);
			EXPECT_TRUE(pSessionTelemetryContext->WasPreparedForTelemetry());
			EXPECT_EQ("prepared:timeout", pSessionTelemetryContext->TelemetryNote());
		}

		void AssertOutboundRemoteRejectKeepsCallerContextOnRequestSend(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			HandlerModes handlerModes;
			handlerModes.implementGetSettings = false;
			ConfigureServerHandlers(handlerModes);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
			auto pContext = CreateClientInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
			pContext->SetTelemetryNote("remote-reject");
			pContext->SetInvokeTimeout(250);

			const long roseResult = m_clientSettingsModule.InvokeGetSettingsWithContext(&argument, &result, &error, pContext);
			EXPECT_EQ(ROSE_REJECT_FUNCTIONMISSING, roseResult);
			ASSERT_GT(ClientOutboundTransportObservation().TransportSendCount(), 0u);
			EXPECT_EQ(pContext.get(), ClientOutboundTransportObservation().LastContextAddress());
			EXPECT_FALSE(pContext->WasPreparedForTelemetry());
			EXPECT_EQ("remote-reject", pContext->TelemetryNote());
		}

		void AssertOutboundRemoteErrorKeepsCallerContextOnRequestSend(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			HandlerModes handlerModes;
			handlerModes.getSettingsReturnsError = true;
			ConfigureServerHandlers(handlerModes);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
			auto pContext = CreateClientInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
			pContext->SetTelemetryNote("remote-error");
			pContext->SetInvokeTimeout(250);

			const long roseResult = m_clientSettingsModule.InvokeGetSettingsWithContext(&argument, &result, &error, pContext);
			EXPECT_EQ(ROSE_ERROR_VALUE, roseResult);
			ASSERT_GT(ClientOutboundTransportObservation().TransportSendCount(), 0u);
			EXPECT_EQ(pContext.get(), ClientOutboundTransportObservation().LastContextAddress());
			EXPECT_FALSE(pContext->WasPreparedForTelemetry());
			EXPECT_EQ("remote-error", pContext->TelemetryNote());
		}

		void AssertOutboundMalformedResponseKeepsCallerContextThroughDecodeFailure(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			m_client.Transport().EnqueueAction(TransportAction::Queue());

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
			auto pContext = CreateClientInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
			pContext->SetTelemetryNote("decode-failure");
			pContext->SetInvokeTimeout(250);

			auto responseFuture = std::async(std::launch::async, [&]() { return m_client.SendInvoke(invokeMsg.GetPtr(), &result, &error, "asnGetSettings", pContext); });

			for (int i = 0; i < 20 && m_client.Transport().QueuedMessageCount() == 0; ++i)
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			ASSERT_EQ(1U, m_client.Transport().QueuedMessageCount());
			m_client.Transport().TakeQueuedMessages();

			std::string responsePayload;
			AsnBool wrongPayload(true);
			ASSERT_EQ(ROSE_NOERROR, m_server.EncodeResult(invokeMsg.GetPtr()->invokeID, &wrongPayload, responsePayload));
			m_client.ReceiveRaw(responsePayload);

			const long roseResult = responseFuture.get();
			EXPECT_EQ(ROSE_RE_DECODE_FAILED, roseResult);
			ASSERT_GT(ClientOutboundTransportObservation().TransportSendCount(), 0u);
			EXPECT_EQ(pContext.get(), ClientOutboundTransportObservation().LastContextAddress());
			EXPECT_FALSE(pContext->WasPreparedForTelemetry());
			EXPECT_EQ("decode-failure", pContext->TelemetryNote());
		}

		void AssertOrphanResultDoesNotBreakPendingInvoke(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			m_server.Transport().EnqueueAction(TransportAction::Queue());

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
			auto pContext = CreateClientInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
			pContext->SetTelemetryNote("pending-invoke");
			pContext->SetInvokeTimeout(500);

			auto pendingInvoke = std::async(std::launch::async, [&]() { return m_clientSettingsModule.InvokeGetSettingsWithContext(&argument, &result, &error, pContext); });

			for (int i = 0; i < 20 && m_server.Transport().QueuedMessageCount() == 0; ++i)
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			ASSERT_EQ(1u, m_server.Transport().QueuedMessageCount());

			AsnBool orphanPayload(true);
			std::string orphanResponse;
			ASSERT_EQ(ROSE_NOERROR, m_client.EncodeResult(4242, &orphanPayload, orphanResponse));
			m_client.ReceiveRaw(orphanResponse);

			FlushServerOutbound();
			EXPECT_EQ(ROSE_NOERROR, pendingInvoke.get());
			EXPECT_FALSE(pContext->WasPreparedForTelemetry());
			EXPECT_EQ("pending-invoke", pContext->TelemetryNote());
		}

		void AssertOrphanResultEmitsInboundResponseTelemetryWithoutPendingOperation(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);

			AsnBool orphanPayload(true);
			std::string orphanResponse;
			ASSERT_EQ(ROSE_NOERROR, m_client.EncodeResult(4242, &orphanPayload, orphanResponse));
			m_client.ReceiveRaw(orphanResponse);

			EXPECT_TRUE(HasInboundResponseTelemetryForOperation(TelemetryEntries(), 0));
		}

		void AssertOrphanErrorDoesNotBreakPendingInvoke(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			m_server.Transport().EnqueueAction(TransportAction::Queue());

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
			auto pContext = CreateClientInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
			pContext->SetTelemetryNote("pending-error");
			pContext->SetInvokeTimeout(500);

			auto pendingInvoke = std::async(std::launch::async, [&]() { return m_clientSettingsModule.InvokeGetSettingsWithContext(&argument, &result, &error, pContext); });

			for (int i = 0; i < 20 && m_server.Transport().QueuedMessageCount() == 0; ++i)
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			ASSERT_EQ(1u, m_server.Transport().QueuedMessageCount());

			AsnRequestError orphanError;
			orphanError.iErrorDetail = 4242;
			orphanError.u8sErrorString.setASCII("orphan error");
			std::string orphanResponse;
			ASSERT_EQ(ROSE_NOERROR, m_client.EncodeError(5252, &orphanError, orphanResponse));
			m_client.ReceiveRaw(orphanResponse);

			FlushServerOutbound();
			EXPECT_EQ(ROSE_NOERROR, pendingInvoke.get());
			EXPECT_FALSE(pContext->WasPreparedForTelemetry());
			EXPECT_EQ("pending-error", pContext->TelemetryNote());
		}

		void AssertOrphanRejectDoesNotBreakPendingInvoke(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			m_server.Transport().EnqueueAction(TransportAction::Queue());

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
			auto pContext = CreateClientInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
			pContext->SetTelemetryNote("pending-reject");
			pContext->SetInvokeTimeout(500);

			auto pendingInvoke = std::async(std::launch::async, [&]() { return m_clientSettingsModule.InvokeGetSettingsWithContext(&argument, &result, &error, pContext); });

			for (int i = 0; i < 20 && m_server.Transport().QueuedMessageCount() == 0; ++i)
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			ASSERT_EQ(1u, m_server.Transport().QueuedMessageCount());

			std::string orphanResponse;
			ASSERT_EQ(ROSE_NOERROR, m_client.EncodeRejectInvoke(6262, InvokeProblem::unrecognisedOperation, orphanResponse, "orphan reject"));
			m_client.ReceiveRaw(orphanResponse);

			FlushServerOutbound();
			EXPECT_EQ(ROSE_NOERROR, pendingInvoke.get());
			EXPECT_FALSE(pContext->WasPreparedForTelemetry());
			EXPECT_EQ("pending-reject", pContext->TelemetryNote());
		}
	};

	TEST_F(InvokeContextRuntimeTest, InboundHandlerReceivesCustomInvokeContextBer)
	{
		AssertInboundHandlerReceivesCustomInvokeContext(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, InboundHandlerReceivesCustomInvokeContextJson)
	{
		AssertInboundHandlerReceivesCustomInvokeContext(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, InboundResponseSendReusesHandlerInvokeContextBer)
	{
		AssertInboundResponseSendReusesHandlerInvokeContext(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, InboundResponseSendReusesHandlerInvokeContextJson)
	{
		AssertInboundResponseSendReusesHandlerInvokeContext(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, InboundHandlerRejectReusesInvokeContextBer)
	{
		AssertInboundHandlerRejectReusesInvokeContext(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, InboundHandlerRejectReusesInvokeContextJson)
	{
		AssertInboundHandlerRejectReusesInvokeContext(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, InboundApplicationErrorReusesInvokeContextBer)
	{
		AssertInboundApplicationErrorReusesInvokeContext(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, InboundApplicationErrorReusesInvokeContextJson)
	{
		AssertInboundApplicationErrorReusesInvokeContext(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, InboundResponseSendFailureKeepsInvokeContextBer)
	{
		AssertInboundResponseSendFailureKeepsInvokeContextOnAttemptedSend(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, InboundResponseSendFailureKeepsInvokeContextJson)
	{
		AssertInboundResponseSendFailureKeepsInvokeContextOnAttemptedSend(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, UnknownInboundOperationRejectUsesCustomInvokeContextBer)
	{
		AssertUnknownInboundOperationRejectUsesCustomInvokeContext(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, UnknownInboundOperationRejectUsesCustomInvokeContextJson)
	{
		AssertUnknownInboundOperationRejectUsesCustomInvokeContext(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, UnparsableInboundDoesNotReachHandlerBer)
	{
		AssertUnparsableInboundDoesNotReachHandler(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, UnparsableInboundDoesNotReachHandlerJson)
	{
		AssertUnparsableInboundDoesNotReachHandler(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundCallerContextReachesTransportBer)
	{
		AssertOutboundCallerContextReachesTransport(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundCallerContextReachesTransportJson)
	{
		AssertOutboundCallerContextReachesTransport(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundStubCreatedContextReachesTransportBer)
	{
		AssertOutboundStubCreatedContextReachesTransport(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundStubCreatedContextReachesTransportJson)
	{
		AssertOutboundStubCreatedContextReachesTransport(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundCallerContextSurvivesUntilCallerScopeEndsBer)
	{
		AssertOutboundCallerContextSurvivesUntilCallerScopeEnds(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundCallerContextSurvivesUntilCallerScopeEndsJson)
	{
		AssertOutboundCallerContextSurvivesUntilCallerScopeEnds(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundEncodeFailureKeepsCallerContextJson)
	{
		AssertOutboundEncodeFailureStillUsesCallerContext(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, NoTransportReturnsTransportFailedForInvokeBer)
	{
		AssertNoTransportReturnsTransportFailedForInvoke(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, NoTransportReturnsTransportFailedForInvokeJson)
	{
		AssertNoTransportReturnsTransportFailedForInvoke(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, NoTransportReturnsTransportFailedForEventBer)
	{
		AssertNoTransportReturnsTransportFailedForEvent(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, NoTransportReturnsTransportFailedForEventJson)
	{
		AssertNoTransportReturnsTransportFailedForEvent(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundTransportFailureKeepsCallerContextBer)
	{
		AssertOutboundTransportFailureKeepsCallerContextOnSendAttempt(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundTransportFailureKeepsCallerContextJson)
	{
		AssertOutboundTransportFailureKeepsCallerContextOnSendAttempt(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundTimeoutKeepsCallerContextBer)
	{
		AssertOutboundTimeoutKeepsCallerContextOnRequestSend(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundTimeoutKeepsCallerContextJson)
	{
		AssertOutboundTimeoutKeepsCallerContextOnRequestSend(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundRemoteRejectKeepsCallerContextBer)
	{
		AssertOutboundRemoteRejectKeepsCallerContextOnRequestSend(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundRemoteRejectKeepsCallerContextJson)
	{
		AssertOutboundRemoteRejectKeepsCallerContextOnRequestSend(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundRemoteErrorKeepsCallerContextBer)
	{
		AssertOutboundRemoteErrorKeepsCallerContextOnRequestSend(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundRemoteErrorKeepsCallerContextJson)
	{
		AssertOutboundRemoteErrorKeepsCallerContextOnRequestSend(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundMalformedResponseKeepsCallerContextBer)
	{
		AssertOutboundMalformedResponseKeepsCallerContextThroughDecodeFailure(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, OutboundMalformedResponseKeepsCallerContextJson)
	{
		AssertOutboundMalformedResponseKeepsCallerContextThroughDecodeFailure(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, OrphanResultDoesNotBreakPendingInvokeBer)
	{
		AssertOrphanResultDoesNotBreakPendingInvoke(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, OrphanResultDoesNotBreakPendingInvokeJson)
	{
		AssertOrphanResultDoesNotBreakPendingInvoke(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, OrphanResultEmitsInboundResponseTelemetryBer)
	{
		AssertOrphanResultEmitsInboundResponseTelemetryWithoutPendingOperation(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, OrphanResultEmitsInboundResponseTelemetryJson)
	{
		AssertOrphanResultEmitsInboundResponseTelemetryWithoutPendingOperation(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, OrphanErrorDoesNotBreakPendingInvokeBer)
	{
		AssertOrphanErrorDoesNotBreakPendingInvoke(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, OrphanErrorDoesNotBreakPendingInvokeJson)
	{
		AssertOrphanErrorDoesNotBreakPendingInvoke(TransportEncoding::JSON);
	}

	TEST_F(InvokeContextRuntimeTest, OrphanRejectDoesNotBreakPendingInvokeBer)
	{
		AssertOrphanRejectDoesNotBreakPendingInvoke(TransportEncoding::BER);
	}

	TEST_F(InvokeContextRuntimeTest, OrphanRejectDoesNotBreakPendingInvokeJson)
	{
		AssertOrphanRejectDoesNotBreakPendingInvoke(TransportEncoding::JSON);
	}

} // namespace sample_runtime_tests
