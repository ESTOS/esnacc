#include "test_support/sample_runtime_harness.h"

#include <algorithm>

namespace sample_runtime_tests
{
namespace
{
bool HasLogEntry(const std::vector<CapturedLogEntry>& entries, const bool bOutbound, const bool bError, const std::string& payloadFragment, const std::string& operationName = {})
{
	return std::any_of(entries.begin(), entries.end(), [&](const CapturedLogEntry& entry) {
		if (entry.bOutbound != bOutbound || entry.bError != bError)
			return false;
		if (!operationName.empty() && entry.strOperationName != operationName)
			return false;
		return payloadFragment.empty() || entry.strPayload.find(payloadFragment) != std::string::npos;
	});
}

bool HasBerEntry(const std::vector<CapturedLogEntry>& entries, const bool bOutbound)
{
	return HasLogEntry(entries, bOutbound, false, "\"BER\"");
}
} // namespace

// Verifies that the runtime emits meaningful inbound and outbound log entries
// for representative success and failure paths in both transport encodings.
class LoggingRuntimeTest : public RuntimeTestBase
{
protected:
	// Enables the richer BER+JSON logging on both endpoints so BER tests can
	// assert both semantic JSON output and raw BER transport data.
	void EnableBerLogging()
	{
		SetServerLogLevels(EAsnLogLevel::JSON_AND_BER, EAsnLogLevel::JSON_AND_BER);
		SetClientLogLevels(EAsnLogLevel::JSON_AND_BER, EAsnLogLevel::JSON_AND_BER);
	}

	// Enables JSON logging on both endpoints for JSON transport tests.
	void EnableJsonLogging()
	{
		SetServerLogLevels(EAsnLogLevel::JSON, EAsnLogLevel::JSON);
		SetClientLogLevels(EAsnLogLevel::JSON, EAsnLogLevel::JSON);
	}

	// Verifies that a normal invoke/result flow is logged on both ends.
	void AssertHappyPathLogsInvokeAndResult(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		if (encoding == TransportEncoding::BER)
			EnableBerLogging();
		else
			EnableJsonLogging();

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
		EXPECT_EQ(ROSE_NOERROR, roseResult);
		EXPECT_TRUE(HasLogEntry(ClientLogs(), true, false, "", "asnGetSettings"));
		EXPECT_TRUE(HasLogEntry(ServerLogs(), false, false, "", "asnGetSettings"));
		EXPECT_TRUE(HasLogEntry(ServerLogs(), true, false, "initial-user"));
		EXPECT_TRUE(HasLogEntry(ClientLogs(), false, false, "initial-user"));

		if (encoding == TransportEncoding::BER)
		{
			EXPECT_TRUE(HasBerEntry(ClientLogs(), true));
			EXPECT_TRUE(HasBerEntry(ServerLogs(), false));
			EXPECT_TRUE(HasBerEntry(ServerLogs(), true));
			EXPECT_TRUE(HasBerEntry(ClientLogs(), false));
		}
	}

	// Verifies that application-error responses are logged on both ends.
	void AssertApplicationErrorLogsErrorResponse(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		if (encoding == TransportEncoding::BER)
			EnableBerLogging();
		else
			EnableJsonLogging();

		HandlerModes handlerModes;
		handlerModes.getSettingsReturnsError = true;
		ConfigureServerHandlers(handlerModes);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
		EXPECT_EQ(ROSE_ERROR_VALUE, roseResult);
		EXPECT_TRUE(HasLogEntry(ServerLogs(), true, false, "get settings handler returned error"));
		EXPECT_TRUE(HasLogEntry(ClientLogs(), false, false, "get settings handler returned error"));
	}

	// Verifies that reject responses are logged on both ends.
	void AssertRejectLogsRejectResponse(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		if (encoding == TransportEncoding::BER)
			EnableBerLogging();
		else
			EnableJsonLogging();

		HandlerModes handlerModes;
		handlerModes.implementGetSettings = false;
		ConfigureServerHandlers(handlerModes);

		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;

		const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
		EXPECT_EQ(ROSE_REJECT_FUNCTIONMISSING, roseResult);

		if (encoding == TransportEncoding::BER)
		{
			EXPECT_TRUE(HasBerEntry(ServerLogs(), true));
			EXPECT_TRUE(HasBerEntry(ClientLogs(), false));
		}
		else
		{
			EXPECT_TRUE(HasLogEntry(ServerLogs(), true, false, "functionMissing"));
			EXPECT_TRUE(HasLogEntry(ClientLogs(), false, false, "functionMissing"));
		}
	}

	// Verifies that malformed inbound payloads are logged as raw data plus an error entry.
	void AssertMalformedInboundPayloadLogsRawDataAndError(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		if (encoding == TransportEncoding::BER)
			SetServerLogLevels(EAsnLogLevel::JSON_AND_BER, EAsnLogLevel::JSON_AND_BER);
		else
			SetServerLogLevels(EAsnLogLevel::JSON, EAsnLogLevel::JSON);

		m_server.ReceiveRaw(MalformedPayload(encoding));

		if (encoding == TransportEncoding::BER)
			EXPECT_TRUE(HasBerEntry(ServerLogs(), false));
		else
			EXPECT_TRUE(HasLogEntry(ServerLogs(), false, false, "{\"bad\":"));

		EXPECT_TRUE(HasLogEntry(ServerLogs(), false, true, "exception"));
	}

	// Verifies that server events are logged both when sent and when received.
	void AssertEventFlowLogsOutboundAndInboundEvents(const TransportEncoding encoding)
	{
		InitializeEndpoints(encoding);
		if (encoding == TransportEncoding::BER)
			EnableBerLogging();
		else
			EnableJsonLogging();

		AsnCreateFancyEventsArgument argument;
		argument.iEventDelay = 0;
		argument.iEventCount = 2;
		AsnCreateFancyEventsResult result;
		AsnRequestError error;

		const long roseResult = m_clientEventModule.InvokeCreateFancyEvents(&argument, &result, &error);
		EXPECT_EQ(ROSE_NOERROR, roseResult);
		EXPECT_TRUE(HasLogEntry(ServerLogs(), true, false, "iEventCounter"));
		EXPECT_TRUE(HasLogEntry(ClientLogs(), false, false, "iEventCounter"));
	}
};

TEST_F(LoggingRuntimeTest, HappyPathLogsInvokeAndResultBer)
{
	AssertHappyPathLogsInvokeAndResult(TransportEncoding::BER);
}

TEST_F(LoggingRuntimeTest, HappyPathLogsInvokeAndResultJson)
{
	AssertHappyPathLogsInvokeAndResult(TransportEncoding::JSON);
}

TEST_F(LoggingRuntimeTest, ApplicationErrorLogsErrorResponseBer)
{
	AssertApplicationErrorLogsErrorResponse(TransportEncoding::BER);
}

TEST_F(LoggingRuntimeTest, ApplicationErrorLogsErrorResponseJson)
{
	AssertApplicationErrorLogsErrorResponse(TransportEncoding::JSON);
}

TEST_F(LoggingRuntimeTest, RejectLogsRejectResponseBer)
{
	AssertRejectLogsRejectResponse(TransportEncoding::BER);
}

TEST_F(LoggingRuntimeTest, RejectLogsRejectResponseJson)
{
	AssertRejectLogsRejectResponse(TransportEncoding::JSON);
}

TEST_F(LoggingRuntimeTest, MalformedInboundPayloadLogsRawDataAndErrorBer)
{
	AssertMalformedInboundPayloadLogsRawDataAndError(TransportEncoding::BER);
}

TEST_F(LoggingRuntimeTest, MalformedInboundPayloadLogsRawDataAndErrorJson)
{
	AssertMalformedInboundPayloadLogsRawDataAndError(TransportEncoding::JSON);
}

TEST_F(LoggingRuntimeTest, EventFlowLogsOutboundAndInboundEventsBer)
{
	AssertEventFlowLogsOutboundAndInboundEvents(TransportEncoding::BER);
}

TEST_F(LoggingRuntimeTest, EventFlowLogsOutboundAndInboundEventsJson)
{
	AssertEventFlowLogsOutboundAndInboundEvents(TransportEncoding::JSON);
}

} // namespace sample_runtime_tests
