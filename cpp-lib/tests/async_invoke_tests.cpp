#include "test_support/sample_runtime_harness.h"

namespace sample_runtime_tests
{

// Covers SendInvokeAsync completion semantics in the sample runtime harness.
class AsyncInvokeRuntimeTest : public RuntimeTestBase
{
protected:
	long SendAsyncGetSettings(AsyncInvokeLatch& latch, AsnGetSettingsResult& result, AsnRequestError& error, int timeoutMs)
	{
		AsnGetSettingsArgument argument;
		SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
		auto pCtx = m_client.CreateSessionInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
		pCtx->SetAsyncCompletion(latch.Callback(), &result, &error);
		return m_client.SendInvokeAsync(invokeMsg.GetPtr(), &result, &error, "asnGetSettings", timeoutMs, std::move(pCtx));
	}

	long SendFireAndForgetGetSettings()
	{
		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		SnaccScopedInvokeMessage invokeMsg(m_client.GetNextInvokeID(), OPID_asnGetSettings, &argument);
		auto pCtx = m_client.CreateSessionInvokeContext(invokeMsg.GetPtr(), "asnGetSettings");
		return m_client.SendInvokeAsync(invokeMsg.GetPtr(), &result, &error, "asnGetSettings", 0, std::move(pCtx));
	}

	void AssertFireAndForgetDoesNotInvokeCallback(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.Transport().EnqueueAction(TransportAction::Queue());

		AsyncInvokeLatch latch;
		const long sendResult = SendFireAndForgetGetSettings();
		ASSERT_EQ(ROSE_NOERROR, sendResult);

		std::this_thread::sleep_for(std::chrono::milliseconds(25));
		EXPECT_EQ(0, latch.CallbackCount());

		FlushServerOutbound();
		std::this_thread::sleep_for(std::chrono::milliseconds(25));
		EXPECT_EQ(0, latch.CallbackCount());
	}

	void AssertFireAndForgetIgnoresImmediateResponse(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);

		AsyncInvokeLatch latch;
		const long sendResult = SendFireAndForgetGetSettings();
		ASSERT_EQ(ROSE_NOERROR, sendResult);
		EXPECT_EQ(0, latch.CallbackCount());
	}

	void AssertAsyncGetSettingsSucceeds(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long sendResult = SendAsyncGetSettings(latch, result, error, 500);
		ASSERT_EQ(ROSE_NOERROR, sendResult);
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(500)));
		EXPECT_EQ(ROSE_NOERROR, latch.RoseResult());
		EXPECT_EQ(1, latch.CallbackCount());
		ASSERT_NE(nullptr, result.settings.u8sUsername);
		EXPECT_EQ("initial-user", result.settings.u8sUsername->getASCII());
	}

	void AssertAsyncGetSettingsReturnsApplicationError(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		HandlerModes handlerModes;
		handlerModes.getSettingsReturnsError = true;
		ConfigureServerHandlers(handlerModes);

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long sendResult = SendAsyncGetSettings(latch, result, error, 500);
		ASSERT_EQ(ROSE_NOERROR, sendResult);
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(500)));
		EXPECT_EQ(ROSE_ERROR_VALUE, latch.RoseResult());
		EXPECT_EQ(7001, error.iErrorDetail.GetLongLong());
	}

	void AssertAsyncGetSettingsReturnsReject(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		HandlerModes handlerModes;
		handlerModes.implementGetSettings = false;
		ConfigureServerHandlers(handlerModes);

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long sendResult = SendAsyncGetSettings(latch, result, error, 500);
		ASSERT_EQ(ROSE_NOERROR, sendResult);
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(500)));
		EXPECT_EQ(ROSE_REJECT_FUNCTIONMISSING, latch.RoseResult());
	}

	void AssertAsyncGetSettingsTimesOut(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.Transport().EnqueueAction(TransportAction::Drop());

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long sendResult = SendAsyncGetSettings(latch, result, error, 25);
		ASSERT_EQ(ROSE_NOERROR, sendResult);
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(500)));
		EXPECT_EQ(ROSE_TE_TIMEOUT, latch.RoseResult());
		EXPECT_EQ(1, latch.CallbackCount());
	}

	void AssertLateAsyncResponseIsIgnoredAfterTimeout(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.Transport().EnqueueAction(TransportAction::Queue());

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long sendResult = SendAsyncGetSettings(latch, result, error, 25);
		ASSERT_EQ(ROSE_NOERROR, sendResult);
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(500)));
		EXPECT_EQ(ROSE_TE_TIMEOUT, latch.RoseResult());

		FlushServerOutbound();
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		EXPECT_EQ(1, latch.CallbackCount());
	}

	void AssertStopProcessingCompletesAsyncInvokeWithShutdown(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_server.Transport().EnqueueAction(TransportAction::Queue());

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long sendResult = SendAsyncGetSettings(latch, result, error, 1000);
		ASSERT_EQ(ROSE_NOERROR, sendResult);

		std::this_thread::sleep_for(std::chrono::milliseconds(25));
		m_client.StopProcessing(true);

		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(500)));
		EXPECT_EQ(ROSE_TE_SHUTDOWN, latch.RoseResult());
		EXPECT_EQ(1, latch.CallbackCount());
	}

	void AssertAsyncSendFailureIsReported(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		m_client.Transport().EnqueueAction(TransportAction::Fail());

		AsyncInvokeLatch latch;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long sendResult = SendAsyncGetSettings(latch, result, error, 250);
		EXPECT_EQ(ROSE_TE_TRANSPORTFAILED, sendResult);
		ASSERT_TRUE(latch.WaitFor(std::chrono::milliseconds(250)));
		EXPECT_EQ(ROSE_TE_TRANSPORTFAILED, latch.RoseResult());
		EXPECT_EQ(1, latch.CallbackCount());
	}
};

TEST_F(AsyncInvokeRuntimeTest, AsyncGetSettingsSucceedsBer)
{
	AssertAsyncGetSettingsSucceeds(TransportEncoding::BER);
}

TEST_F(AsyncInvokeRuntimeTest, AsyncGetSettingsSucceedsJson)
{
	AssertAsyncGetSettingsSucceeds(TransportEncoding::JSON);
}

TEST_F(AsyncInvokeRuntimeTest, AsyncGetSettingsReturnsApplicationErrorBer)
{
	AssertAsyncGetSettingsReturnsApplicationError(TransportEncoding::BER);
}

TEST_F(AsyncInvokeRuntimeTest, AsyncGetSettingsReturnsRejectJson)
{
	AssertAsyncGetSettingsReturnsReject(TransportEncoding::JSON);
}

TEST_F(AsyncInvokeRuntimeTest, AsyncGetSettingsTimesOutBer)
{
	AssertAsyncGetSettingsTimesOut(TransportEncoding::BER);
}

TEST_F(AsyncInvokeRuntimeTest, AsyncGetSettingsTimesOutJson)
{
	AssertAsyncGetSettingsTimesOut(TransportEncoding::JSON);
}

TEST_F(AsyncInvokeRuntimeTest, LateAsyncResponseIsIgnoredAfterTimeoutBer)
{
	AssertLateAsyncResponseIsIgnoredAfterTimeout(TransportEncoding::BER);
}

TEST_F(AsyncInvokeRuntimeTest, StopProcessingCompletesAsyncInvokeWithShutdownJson)
{
	AssertStopProcessingCompletesAsyncInvokeWithShutdown(TransportEncoding::JSON);
}

TEST_F(AsyncInvokeRuntimeTest, AsyncSendFailureIsReportedBer)
{
	AssertAsyncSendFailureIsReported(TransportEncoding::BER);
}

TEST_F(AsyncInvokeRuntimeTest, AsyncSendFailureIsReportedJson)
{
	AssertAsyncSendFailureIsReported(TransportEncoding::JSON);
}

TEST_F(AsyncInvokeRuntimeTest, FireAndForgetDoesNotInvokeCallbackBer)
{
	AssertFireAndForgetDoesNotInvokeCallback(TransportEncoding::BER);
}

TEST_F(AsyncInvokeRuntimeTest, FireAndForgetIgnoresImmediateResponseJson)
{
	AssertFireAndForgetIgnoresImmediateResponse(TransportEncoding::JSON);
}

} // namespace sample_runtime_tests
