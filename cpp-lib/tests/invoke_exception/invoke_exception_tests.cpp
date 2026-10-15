#include "test_support/sample_runtime_harness.h"

#include <algorithm>

namespace snacclib
{
	namespace
	{
		// True when an error log entry contains the exception text.
		bool HasErrorLog(const std::vector<CapturedLogEntry>& entries, const std::string& fragment)
		{
			return std::any_of(entries.begin(), entries.end(), [&](const CapturedLogEntry& entry) { return entry.bError && entry.strPayload.find(fragment) != std::string::npos; });
		}
	} // namespace

	// SnaccException leaving the invoke path must be logged and answered with a ROSE reject.
	// The connection stays usable for the next invoke.
	class snacclib_InvokeExceptionRuntimeTest : public RuntimeTestBase
	{
	protected:
		// A later get-settings call succeeds, so the failed invoke did not drop the session.
		void ExpectServerStillServesGetSettings()
		{
			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			EXPECT_EQ(ROSE_NOERROR, m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error));
		}

		// Product OnInvoke handler throws SnaccException.
		void AssertHandlerThrowLogsAndRejects(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			HandlerModes handlerModes;
			handlerModes.throwSnaccExceptionOnGetSettings = true;
			ConfigureServerHandlers(handlerModes);

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);

			EXPECT_EQ(ROSE_REJECT_MISTYPEDARGUMENT, roseResult);
			EXPECT_TRUE(HasErrorLog(ServerLogs(), "get settings handler fault"));
			ExpectServerStillServesGetSettings();
		}

		// Argument BDec/JDec throws inside DecodeInvoke. That catch logs and returns
		// ROSE_RE_DECODE_FAILED, which the invoke path sends as an unrecognisedOperation reject.
		void AssertDecodeThrowLogsAndRejects(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			m_server.FailNextInvokeDecode();

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);

			EXPECT_EQ(ROSE_REJECT_UNKNOWNOPERATION, roseResult);
			EXPECT_TRUE(HasErrorLog(ServerLogs(), "invoke decode fault"));
			ExpectServerStillServesGetSettings();
		}

		// Result BEnc/JEnc throws "Choice is empty" from EncodeResult.
		// OnInvokeMessage logs it and answers with mistypedArgument.
		void AssertResultEncodeThrowLogsAndRejects(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			m_server.FailNextResultEncode();

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);

			EXPECT_EQ(ROSE_REJECT_MISTYPEDARGUMENT, roseResult);
			EXPECT_TRUE(HasErrorLog(ServerLogs(), "Choice is empty"));
			ExpectServerStillServesGetSettings();
		}

		// Result encode throws, then reject logging throws again.
		// OnBinaryDataBlock catches that second SnaccException, logs it, and still sends a reject.
		void AssertRejectHandlingThrowLogsAndStillRejects(const TransportEncoding encoding)
		{
			InitializeEndpoints(encoding);
			if (encoding == TransportEncoding::BER)
				SetServerLogLevels(EAsnLogLevel::JSON_AND_BER, EAsnLogLevel::JSON_AND_BER);
			else
				SetServerLogLevels(EAsnLogLevel::JSON, EAsnLogLevel::JSON);

			m_server.FailNextResultEncode();
			m_server.FailNextOutboundLog();

			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			const long roseResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);

			EXPECT_EQ(ROSE_REJECT_MISTYPEDARGUMENT, roseResult);
			EXPECT_TRUE(HasErrorLog(ServerLogs(), "Choice is empty"));
			EXPECT_TRUE(HasErrorLog(ServerLogs(), "reject handling fault"));
			ExpectServerStillServesGetSettings();
		}
	};

	TEST_F(snacclib_InvokeExceptionRuntimeTest, HandlerThrowLogsAndRejectsBer)
	{
		AssertHandlerThrowLogsAndRejects(TransportEncoding::BER);
	}

	TEST_F(snacclib_InvokeExceptionRuntimeTest, DecodeThrowLogsAndRejectsBer)
	{
		AssertDecodeThrowLogsAndRejects(TransportEncoding::BER);
	}

	TEST_F(snacclib_InvokeExceptionRuntimeTest, ResultEncodeThrowLogsAndRejectsBer)
	{
		AssertResultEncodeThrowLogsAndRejects(TransportEncoding::BER);
	}

	TEST_F(snacclib_InvokeExceptionRuntimeTest, RejectHandlingThrowLogsAndStillRejectsBer)
	{
		AssertRejectHandlingThrowLogsAndStillRejects(TransportEncoding::BER);
	}

	TEST_F(snacclib_InvokeExceptionRuntimeTest, HandlerThrowLogsAndRejectsJson)
	{
		AssertHandlerThrowLogsAndRejects(TransportEncoding::JSON);
	}

	TEST_F(snacclib_InvokeExceptionRuntimeTest, DecodeThrowLogsAndRejectsJson)
	{
		AssertDecodeThrowLogsAndRejects(TransportEncoding::JSON);
	}

	TEST_F(snacclib_InvokeExceptionRuntimeTest, ResultEncodeThrowLogsAndRejectsJson)
	{
		AssertResultEncodeThrowLogsAndRejects(TransportEncoding::JSON);
	}

	TEST_F(snacclib_InvokeExceptionRuntimeTest, RejectHandlingThrowLogsAndStillRejectsJson)
	{
		AssertRejectHandlingThrowLogsAndStillRejects(TransportEncoding::JSON);
	}

} // namespace snacclib
