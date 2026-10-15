#include "test_support/sample_runtime_harness.h"

#include <algorithm>

namespace snacclib
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

		// Decoded JSON notation for one direction.
		// The BER hex line is a different entry and contains the "BER" wrapper when the JSON bit is also set.
		// Pretty output from getPrettyPrinted contains a newline. A compact payload does not satisfy this.
		bool HasPrettyDecodedJsonEntry(const std::vector<CapturedLogEntry>& entries, const bool bOutbound, const std::string& firstFragment, const std::string& secondFragment, const std::string& operationName = {})
		{
			return std::any_of(entries.begin(), entries.end(), [&](const CapturedLogEntry& entry) {
				if (entry.bOutbound != bOutbound || entry.bError)
					return false;
				if (!operationName.empty() && entry.strOperationName != operationName)
					return false;
				if (entry.strPayload.find("\"BER\"") != std::string::npos)
					return false;
				if (entry.strPayload.find('\n') == std::string::npos)
					return false;
				if (entry.strPayload.find(firstFragment) == std::string::npos)
					return false;
				return secondFragment.empty() || entry.strPayload.find(secondFragment) != std::string::npos;
			});
		}

		// BER-only notation. With the JSON bit off, LogTransportData writes the hex bytes themselves
		// and does not wrap them as "BER". Reject envelopes are context tag [4], so the hex starts with A4.
		bool HasRawBerHexEntry(const std::vector<CapturedLogEntry>& entries, const bool bOutbound, const std::string& hexPrefix)
		{
			return std::any_of(entries.begin(), entries.end(), [&](const CapturedLogEntry& entry) {
				if (entry.bOutbound != bOutbound || entry.bError)
					return false;
				if (entry.strPayload.find('{') != std::string::npos)
					return false;
				return entry.strPayload.rfind(hexPrefix, 0) == 0;
			});
		}
	} // namespace

	// Verifies that the runtime emits meaningful inbound and outbound log entries
	// for representative success and failure paths in both transport encodings.
	class snacclib_LoggingRuntimeTest : public RuntimeTestBase
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

		// Requires one pretty decoded JSON entry for this direction.
		// firstFragment is the ROSE choice key. secondFragment is payload text when the message carries it.
		// operationName is set when the runtime attaches a name to that JSON entry (invokes).
		void ExpectPrettyDecodedJson(const std::vector<CapturedLogEntry>& entries, const bool bOutbound, const char* firstFragment, const char* secondFragment, const char* where, const char* operationName = nullptr)
		{
			const char* second = secondFragment ? secondFragment : "";
			const char* operation = operationName ? operationName : "";
			EXPECT_TRUE(HasPrettyDecodedJsonEntry(entries, bOutbound, firstFragment, second, operation)) << where << " missing pretty decoded JSON containing \"" << firstFragment << "\"" << (second[0] ? " and \"" : "") << (second[0] ? second : "") << (second[0] ? "\"" : "") << (operation[0] ? " for operation " : "") << (operation[0] ? operation : "");
		}

		// Requires the combined JSON+BER hex line for this direction.
		void ExpectWrappedBerHex(const std::vector<CapturedLogEntry>& entries, const bool bOutbound, const char* where)
		{
			EXPECT_TRUE(HasBerEntry(entries, bOutbound)) << where << " missing BER hex entry";
		}

		// BER wire with JSON_AND_BER: invoke and result each have pretty JSON and a hex line, both directions.
		void AssertBerInvokeAndResultLoggedInBothNotations()
		{
			InitializeEndpoints(TransportEncoding::BER);
			EnableBerLogging();

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;

			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
			EXPECT_EQ(ROSE_NOERROR, roseResult);

			ExpectPrettyDecodedJson(ClientLogs(), true, "\"invoke\"", nullptr, "client outbound invoke", "asnGetSettings");
			ExpectPrettyDecodedJson(ServerLogs(), false, "\"invoke\"", nullptr, "server inbound invoke", "asnGetSettings");
			ExpectPrettyDecodedJson(ServerLogs(), true, "\"result\"", "initial-user", "server outbound result");
			ExpectPrettyDecodedJson(ClientLogs(), false, "\"result\"", "initial-user", "client inbound result");
			ExpectWrappedBerHex(ClientLogs(), true, "client outbound");
			ExpectWrappedBerHex(ServerLogs(), false, "server inbound");
			ExpectWrappedBerHex(ServerLogs(), true, "server outbound");
			ExpectWrappedBerHex(ClientLogs(), false, "client inbound");
		}

		// BER wire with JSON_AND_BER: the application error has pretty JSON and a hex line, both directions.
		void AssertBerErrorLoggedInBothNotations()
		{
			InitializeEndpoints(TransportEncoding::BER);
			EnableBerLogging();

			HandlerModes handlerModes;
			handlerModes.getSettingsReturnsError = true;
			ConfigureServerHandlers(handlerModes);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;

			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
			EXPECT_EQ(ROSE_ERROR_VALUE, roseResult);

			ExpectPrettyDecodedJson(ServerLogs(), true, "\"error\"", "get settings handler returned error", "server outbound error");
			ExpectPrettyDecodedJson(ClientLogs(), false, "\"error\"", "get settings handler returned error", "client inbound error");
			ExpectWrappedBerHex(ServerLogs(), true, "server outbound error");
			ExpectWrappedBerHex(ClientLogs(), false, "client inbound error");
		}

		// BER wire with JSON_AND_BER: a functionMissing reject has pretty JSON and a hex line, both directions.
		// The inbound JSON assertion is the known gap: the client currently records only the BER hex line.
		void AssertBerRejectLoggedInBothNotations()
		{
			InitializeEndpoints(TransportEncoding::BER);
			EnableBerLogging();

			HandlerModes handlerModes;
			handlerModes.implementGetSettings = false;
			ConfigureServerHandlers(handlerModes);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;

			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
			EXPECT_EQ(ROSE_REJECT_FUNCTIONMISSING, roseResult);

			ExpectPrettyDecodedJson(ServerLogs(), true, "\"reject\"", "functionMissing", "server outbound reject");
			ExpectPrettyDecodedJson(ClientLogs(), false, "\"reject\"", "functionMissing", "client inbound reject");
			ExpectWrappedBerHex(ServerLogs(), true, "server outbound reject");
			ExpectWrappedBerHex(ClientLogs(), false, "client inbound reject");
		}

		// BER wire with JSON_AND_BER: each event has pretty JSON and a hex line, sender and receiver.
		void AssertBerEventLoggedInBothNotations()
		{
			InitializeEndpoints(TransportEncoding::BER);
			EnableBerLogging();

			AsnCreateFancyEventsArgument argument;
			argument.iEventDelay = 0;
			argument.iEventCount = 2;
			AsnCreateFancyEventsResult result;
			AsnRequestError error;

			const long roseResult = m_clientEventModule.InvokeCreateFancyEvents(&argument, &result, &error);
			EXPECT_EQ(ROSE_NOERROR, roseResult);

			ExpectPrettyDecodedJson(ServerLogs(), true, "\"invoke\"", "iEventCounter", "server outbound event");
			ExpectPrettyDecodedJson(ClientLogs(), false, "\"invoke\"", "iEventCounter", "client inbound event");
			ExpectWrappedBerHex(ServerLogs(), true, "server outbound event");
			ExpectWrappedBerHex(ClientLogs(), false, "client inbound event");
		}

		// JSON wire with the JSON bit: the reject is pretty JSON on both sides, and no BER hex line is written.
		void AssertJsonRejectLoggedAsPrettyJsonOnly()
		{
			InitializeEndpoints(TransportEncoding::JSON);
			EnableJsonLogging();

			HandlerModes handlerModes;
			handlerModes.implementGetSettings = false;
			ConfigureServerHandlers(handlerModes);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;

			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
			EXPECT_EQ(ROSE_REJECT_FUNCTIONMISSING, roseResult);

			ExpectPrettyDecodedJson(ServerLogs(), true, "\"reject\"", "functionMissing", "server outbound reject");
			ExpectPrettyDecodedJson(ClientLogs(), false, "\"reject\"", "functionMissing", "client inbound reject");
			EXPECT_FALSE(HasBerEntry(ServerLogs(), true));
			EXPECT_FALSE(HasBerEntry(ClientLogs(), false));
		}

		// JSON wire with JSON_AND_BER still logs JSON only. The BER bit applies to BER payloads.
		void AssertJsonTransportWithBothBitsLogsJsonWithoutHex()
		{
			InitializeEndpoints(TransportEncoding::JSON);
			SetServerLogLevels(EAsnLogLevel::JSON_AND_BER, EAsnLogLevel::JSON_AND_BER);
			SetClientLogLevels(EAsnLogLevel::JSON_AND_BER, EAsnLogLevel::JSON_AND_BER);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;

			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
			EXPECT_EQ(ROSE_NOERROR, roseResult);

			ExpectPrettyDecodedJson(ClientLogs(), true, "\"invoke\"", nullptr, "client outbound invoke", "asnGetSettings");
			ExpectPrettyDecodedJson(ServerLogs(), false, "\"invoke\"", nullptr, "server inbound invoke", "asnGetSettings");
			ExpectPrettyDecodedJson(ServerLogs(), true, "\"result\"", "initial-user", "server outbound result");
			ExpectPrettyDecodedJson(ClientLogs(), false, "\"result\"", "initial-user", "client inbound result");
			EXPECT_FALSE(HasBerEntry(ClientLogs(), true));
			EXPECT_FALSE(HasBerEntry(ServerLogs(), false));
			EXPECT_FALSE(HasBerEntry(ServerLogs(), true));
			EXPECT_FALSE(HasBerEntry(ClientLogs(), false));
		}

		// BER bit alone: the reject is hex on both sides and the decoded JSON detail is absent.
		void AssertBerOnlyRejectLoggedAsHex()
		{
			InitializeEndpoints(TransportEncoding::BER);
			SetServerLogLevels(EAsnLogLevel::BER, EAsnLogLevel::BER);
			SetClientLogLevels(EAsnLogLevel::BER, EAsnLogLevel::BER);

			HandlerModes handlerModes;
			handlerModes.implementGetSettings = false;
			ConfigureServerHandlers(handlerModes);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;

			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
			EXPECT_EQ(ROSE_REJECT_FUNCTIONMISSING, roseResult);

			EXPECT_TRUE(HasRawBerHexEntry(ServerLogs(), true, "A4")) << "server outbound reject missing raw BER hex";
			EXPECT_TRUE(HasRawBerHexEntry(ClientLogs(), false, "A4")) << "client inbound reject missing raw BER hex";
			EXPECT_FALSE(HasPrettyDecodedJsonEntry(ServerLogs(), true, "\"reject\"", "functionMissing"));
			EXPECT_FALSE(HasPrettyDecodedJsonEntry(ClientLogs(), false, "\"reject\"", "functionMissing"));
		}

		// JSON_ALWAYS_PRETTY_PRINTED forces a newline in the JSON notation on a JSON transport.
		void AssertPrettyLevelLogsNewlines()
		{
			InitializeEndpoints(TransportEncoding::JSON);
			const auto pretty = static_cast<EAsnLogLevel>(static_cast<int>(EAsnLogLevel::JSON) | static_cast<int>(EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED));
			SetServerLogLevels(pretty, pretty);
			SetClientLogLevels(pretty, pretty);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;

			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
			EXPECT_EQ(ROSE_NOERROR, roseResult);

			ExpectPrettyDecodedJson(ClientLogs(), true, "\"invoke\"", nullptr, "client outbound invoke", "asnGetSettings");
			ExpectPrettyDecodedJson(ServerLogs(), false, "\"invoke\"", nullptr, "server inbound invoke", "asnGetSettings");
			ExpectPrettyDecodedJson(ServerLogs(), true, "\"result\"", "initial-user", "server outbound result");
			ExpectPrettyDecodedJson(ClientLogs(), false, "\"result\"", "initial-user", "client inbound result");
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

	TEST_F(snacclib_LoggingRuntimeTest, HappyPathLogsInvokeAndResultBer)
	{
		AssertHappyPathLogsInvokeAndResult(TransportEncoding::BER);
	}

	TEST_F(snacclib_LoggingRuntimeTest, HappyPathLogsInvokeAndResultJson)
	{
		AssertHappyPathLogsInvokeAndResult(TransportEncoding::JSON);
	}

	TEST_F(snacclib_LoggingRuntimeTest, ApplicationErrorLogsErrorResponseBer)
	{
		AssertApplicationErrorLogsErrorResponse(TransportEncoding::BER);
	}

	TEST_F(snacclib_LoggingRuntimeTest, ApplicationErrorLogsErrorResponseJson)
	{
		AssertApplicationErrorLogsErrorResponse(TransportEncoding::JSON);
	}

	TEST_F(snacclib_LoggingRuntimeTest, RejectLogsRejectResponseBer)
	{
		AssertRejectLogsRejectResponse(TransportEncoding::BER);
	}

	TEST_F(snacclib_LoggingRuntimeTest, RejectLogsRejectResponseJson)
	{
		AssertRejectLogsRejectResponse(TransportEncoding::JSON);
	}

	TEST_F(snacclib_LoggingRuntimeTest, MalformedInboundPayloadLogsRawDataAndErrorBer)
	{
		AssertMalformedInboundPayloadLogsRawDataAndError(TransportEncoding::BER);
	}

	TEST_F(snacclib_LoggingRuntimeTest, MalformedInboundPayloadLogsRawDataAndErrorJson)
	{
		AssertMalformedInboundPayloadLogsRawDataAndError(TransportEncoding::JSON);
	}

	TEST_F(snacclib_LoggingRuntimeTest, EventFlowLogsOutboundAndInboundEventsBer)
	{
		AssertEventFlowLogsOutboundAndInboundEvents(TransportEncoding::BER);
	}

	TEST_F(snacclib_LoggingRuntimeTest, EventFlowLogsOutboundAndInboundEventsJson)
	{
		AssertEventFlowLogsOutboundAndInboundEvents(TransportEncoding::JSON);
	}

	// BUILDSYS-661: every ROSE PDU is logged in each notation the bitmask enables.
	TEST_F(snacclib_LoggingRuntimeTest, BerInvokeAndResultLoggedAsPrettyJsonAndHex)
	{
		AssertBerInvokeAndResultLoggedInBothNotations();
	}

	TEST_F(snacclib_LoggingRuntimeTest, BerErrorLoggedAsPrettyJsonAndHex)
	{
		AssertBerErrorLoggedInBothNotations();
	}

	TEST_F(snacclib_LoggingRuntimeTest, BerRejectLoggedAsPrettyJsonAndHex)
	{
		AssertBerRejectLoggedInBothNotations();
	}

	TEST_F(snacclib_LoggingRuntimeTest, BerEventLoggedAsPrettyJsonAndHex)
	{
		AssertBerEventLoggedInBothNotations();
	}

	TEST_F(snacclib_LoggingRuntimeTest, JsonRejectLoggedAsPrettyJsonWithoutHex)
	{
		AssertJsonRejectLoggedAsPrettyJsonOnly();
	}

	TEST_F(snacclib_LoggingRuntimeTest, JsonTransportWithBothBitsLogsJsonWithoutHex)
	{
		AssertJsonTransportWithBothBitsLogsJsonWithoutHex();
	}

	TEST_F(snacclib_LoggingRuntimeTest, BerOnlyRejectLoggedAsHexWithoutDecodedJson)
	{
		AssertBerOnlyRejectLoggedAsHex();
	}

	TEST_F(snacclib_LoggingRuntimeTest, JsonPrettyLevelLogsNewlines)
	{
		AssertPrettyLevelLogsNewlines();
	}

} // namespace snacclib
