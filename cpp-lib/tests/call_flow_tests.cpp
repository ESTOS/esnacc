#include "test_support/sample_runtime_harness.h"

namespace sample_runtime_tests
{

// Covers the happy-path request/response and event flows for the sample
// settings and event-manager modules.
class CallFlowRuntimeTest : public RuntimeTestBase
{
protected:
	// Verifies that the server returns its current settings state unchanged.
	void AssertGetSettingsRoundTripReturnsCurrentServerState(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
		EXPECT_EQ(ROSE_NOERROR, roseResult);
		ASSERT_NE(nullptr, result.settings.bEnabled);
		ASSERT_NE(nullptr, result.settings.u8sUsername);
		EXPECT_FALSE(static_cast<bool>(*result.settings.bEnabled));
		EXPECT_EQ("initial-user", result.settings.u8sUsername->getASCII());
	}

	// Verifies that setting new values updates server state and emits a client event.
	void AssertSetSettingsRoundTripDispatchesClientEvent(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);

		AsnSetSettingsArgument argument;
		SetSettingsValues(argument.settings, true, "updated-user");
		AsnSetSettingsResult result;
		AsnRequestError error;

		const long setResult = m_clientSettingsModule.InvokeSetSettings(&argument, &result, &error);
		EXPECT_EQ(ROSE_NOERROR, setResult);
		EXPECT_EQ(1, ClientSettingsEventCount());
		EXPECT_TRUE(ClientLastSettingsEventEnabled());
		EXPECT_EQ("updated-user", ClientLastSettingsEventUsername());

		AsnGetSettingsArgument getArgument;
		AsnGetSettingsResult getResult;
		const long getResultCode = m_clientSettingsModule.InvokeGetSettings(&getArgument, &getResult, &error);
		EXPECT_EQ(ROSE_NOERROR, getResultCode);
		ASSERT_NE(nullptr, getResult.settings.bEnabled);
		ASSERT_NE(nullptr, getResult.settings.u8sUsername);
		EXPECT_TRUE(static_cast<bool>(*getResult.settings.bEnabled));
		EXPECT_EQ("updated-user", getResult.settings.u8sUsername->getASCII());
	}

	// Verifies that one invoke can trigger multiple ordered events back to the client.
	void AssertCreateFancyEventsDispatchesServerEventsToClient(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);

		AsnCreateFancyEventsArgument argument;
		argument.iEventDelay = 0;
		argument.iEventCount = 3;
		AsnCreateFancyEventsResult result;
		AsnRequestError error;

		const long roseResult = m_clientEventModule.InvokeCreateFancyEvents(&argument, &result, &error);
		EXPECT_EQ(ROSE_NOERROR, roseResult);
		ASSERT_EQ(3u, ClientFancyEventCount());
		EXPECT_EQ(std::make_pair(1LL, 2LL), ClientFancyEvents().at(0));
		EXPECT_EQ(std::make_pair(2LL, 1LL), ClientFancyEvents().at(1));
		EXPECT_EQ(std::make_pair(3LL, 0LL), ClientFancyEvents().at(2));
	}
};

// Runs the get-settings happy path over BER.
TEST_F(CallFlowRuntimeTest, GetSettingsRoundTripReturnsCurrentServerStateBer)
{
	AssertGetSettingsRoundTripReturnsCurrentServerState(TransportEncoding::BER);
}

// Runs the get-settings happy path over JSON.
TEST_F(CallFlowRuntimeTest, GetSettingsRoundTripReturnsCurrentServerStateJson)
{
	AssertGetSettingsRoundTripReturnsCurrentServerState(TransportEncoding::JSON);
}

// Runs the set-settings happy path over BER.
TEST_F(CallFlowRuntimeTest, SetSettingsRoundTripDispatchesClientEventBer)
{
	AssertSetSettingsRoundTripDispatchesClientEvent(TransportEncoding::BER);
}

// Runs the set-settings happy path over JSON.
TEST_F(CallFlowRuntimeTest, SetSettingsRoundTripDispatchesClientEventJson)
{
	AssertSetSettingsRoundTripDispatchesClientEvent(TransportEncoding::JSON);
}

// Runs the multi-event happy path over BER.
TEST_F(CallFlowRuntimeTest, CreateFancyEventsDispatchesServerEventsToClientBer)
{
	AssertCreateFancyEventsDispatchesServerEventsToClient(TransportEncoding::BER);
}

// Runs the multi-event happy path over JSON.
TEST_F(CallFlowRuntimeTest, CreateFancyEventsDispatchesServerEventsToClientJson)
{
	AssertCreateFancyEventsDispatchesServerEventsToClient(TransportEncoding::JSON);
}

} // namespace sample_runtime_tests
