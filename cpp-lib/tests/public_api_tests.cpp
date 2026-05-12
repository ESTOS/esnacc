#include "test_support/sample_runtime_harness.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace sample_runtime_tests
{
namespace
{
class TransportProbe : public ISnaccROSETransport
{
public:
	long m_lReturnValue = ROSE_NOERROR; // result returned to the caller when data is sent
	std::vector<std::string> m_payloads; // raw payloads observed by the transport

	long SendBinaryDataBlockEx(const char* lpBytes, size_t size, SnaccInvokeContext& /* ctx */) override
	{
		m_payloads.emplace_back(lpBytes, size);
		return m_lReturnValue;
	}
};

struct DecodeErrorObservation
{
	bool bAlreadyTransportLogged = false;
	TransportEncoding encoding = TransportEncoding::UNKNOWN;
	std::string strPayload;
	std::string strWhat;
};

class DecodeObservedRuntimeEndpoint : public ObservedRuntimeEndpoint
{
public:
	using ObservedRuntimeEndpoint::ObservedRuntimeEndpoint;

	void ClearDecodeErrors()
	{
		m_decodeErrors.clear();
	}

	const std::vector<DecodeErrorObservation>& DecodeErrors() const
	{
		return m_decodeErrors;
	}

	void OnRoseDecodeError(const bool bAlreadyTransportLogged, const SNACC::TransportEncoding encoding, const char* lpBytes, unsigned long ulSize, const std::string& strWhat) override
	{
		DecodeErrorObservation observation;
		observation.bAlreadyTransportLogged = bAlreadyTransportLogged;
		observation.encoding = encoding;
		observation.strPayload.assign(lpBytes, ulSize);
		observation.strWhat = strWhat;
		m_decodeErrors.push_back(std::move(observation));
	}

private:
	std::vector<DecodeErrorObservation> m_decodeErrors;
};

bool HasNonErrorLog(const std::vector<CapturedLogEntry>& entries)
{
	return std::any_of(entries.begin(), entries.end(), [](const CapturedLogEntry& entry) {
		return !entry.bError;
	});
}

bool HasInboundNonErrorLog(const std::vector<CapturedLogEntry>& entries)
{
	return std::any_of(entries.begin(), entries.end(), [](const CapturedLogEntry& entry) {
		return !entry.bOutbound && !entry.bError;
	});
}

bool HasBerLog(const std::vector<CapturedLogEntry>& entries)
{
	return std::any_of(entries.begin(), entries.end(), [](const CapturedLogEntry& entry) {
		return !entry.bError && entry.strPayload.find("\"BER\"") != std::string::npos;
	});
}

std::string GetMalformedPayloadForApiTest(const TransportEncoding encoding)
{
	if (encoding == TransportEncoding::BER)
		return std::string("\xA1\x01", 2);
	return "J0000007{\"bad\":";
}

void AssertOnBinaryDataBlockResultDecodeErrorsInvokeHook(const TransportEncoding encoding)
{
	DecodeObservedRuntimeEndpoint endpoint{L"DecodeHookEndpoint", "decode-hook-session"};
	endpoint.SetTransportEncoding(encoding);
	endpoint.SetLogLevels(EAsnLogLevel::JSON_AND_BER, EAsnLogLevel::JSON_AND_BER);

	const std::string malformedPayload = GetMalformedPayloadForApiTest(encoding);

	endpoint.OnBinaryDataBlockResult(malformedPayload.data(), static_cast<unsigned long>(malformedPayload.size()), false);
	ASSERT_EQ(1u, endpoint.DecodeErrors().size());
	EXPECT_FALSE(endpoint.DecodeErrors().front().bAlreadyTransportLogged);
	EXPECT_EQ(encoding, endpoint.DecodeErrors().front().encoding);
	EXPECT_EQ(malformedPayload, endpoint.DecodeErrors().front().strPayload);

	endpoint.ClearCapturedLogs();
	endpoint.ClearDecodeErrors();

	endpoint.OnBinaryDataBlockResult(malformedPayload.data(), static_cast<unsigned long>(malformedPayload.size()), true);
	ASSERT_EQ(1u, endpoint.DecodeErrors().size());
	EXPECT_TRUE(endpoint.DecodeErrors().front().bAlreadyTransportLogged);
	EXPECT_EQ(encoding, endpoint.DecodeErrors().front().encoding);
	EXPECT_EQ(malformedPayload, endpoint.DecodeErrors().front().strPayload);
}
} // namespace

// Covers focused public API behavior that is not obvious from the broader
// scenario tests alone.
class PublicApiRuntimeTest : public RuntimeTestBase
{
protected:
	// Verifies that `OnBinaryDataBlock` respects the explicit log toggle.
	void AssertOnBinaryDataBlockLoggingFlagControlsRawPayloadLogging()
	{
		InitializeEndpoints(TransportEncoding::BER);
		SetServerLogLevels(EAsnLogLevel::JSON_AND_BER, EAsnLogLevel::JSON_AND_BER);

		const std::string malformedPayload = MalformedPayload(TransportEncoding::BER);
		m_server.OnBinaryDataBlock(malformedPayload.data(), static_cast<unsigned long>(malformedPayload.size()), false);
		EXPECT_FALSE(HasInboundNonErrorLog(ServerLogs()));
		EXPECT_TRUE(std::any_of(ServerLogs().begin(), ServerLogs().end(), [](const CapturedLogEntry& entry) { return entry.bError; }));

		m_server.ClearCapturedLogs();
		m_server.OnBinaryDataBlock(malformedPayload.data(), static_cast<unsigned long>(malformedPayload.size()), true);
		EXPECT_TRUE(HasBerLog(ServerLogs()));
		EXPECT_TRUE(std::any_of(ServerLogs().begin(), ServerLogs().end(), [](const CapturedLogEntry& entry) { return entry.bError; }));
	}

	// Verifies that `OnBinaryDataBlockResult` can complete a pending invoke and
	// that its logging toggle controls raw response logging.
	void AssertOnBinaryDataBlockResultLoggingFlagControlsResponseLogging()
	{
		InitializeEndpoints(TransportEncoding::BER);
		SetClientLogLevels(EAsnLogLevel::JSON_AND_BER, EAsnLogLevel::JSON_AND_BER);

		m_server.Transport().EnqueueAction(TransportAction::Queue());
		auto pendingInvoke = InvokeGetSettingsAsync(1000);
		std::this_thread::sleep_for(std::chrono::milliseconds(25));
		ASSERT_EQ(1u, m_server.Transport().QueuedMessageCount());
		std::vector<std::string> queuedResponses = m_server.Transport().TakeQueuedMessages();
		ASSERT_EQ(1u, queuedResponses.size());
		m_client.ClearCapturedLogs();

		const bool handledWithoutLogging = m_client.OnBinaryDataBlockResult(queuedResponses.front().data(), static_cast<unsigned long>(queuedResponses.front().size()), false);
		EXPECT_TRUE(handledWithoutLogging);
		EXPECT_EQ(ROSE_NOERROR, pendingInvoke.get());
		EXPECT_FALSE(HasBerLog(ClientLogs()));

		InitializeEndpoints(TransportEncoding::BER);
		SetClientLogLevels(EAsnLogLevel::JSON_AND_BER, EAsnLogLevel::JSON_AND_BER);
		m_server.Transport().EnqueueAction(TransportAction::Queue());
		auto secondPendingInvoke = InvokeGetSettingsAsync(1000);
		std::this_thread::sleep_for(std::chrono::milliseconds(25));
		queuedResponses = m_server.Transport().TakeQueuedMessages();
		ASSERT_EQ(1u, queuedResponses.size());
		m_client.ClearCapturedLogs();

		const bool handledWithLogging = m_client.OnBinaryDataBlockResult(queuedResponses.front().data(), static_cast<unsigned long>(queuedResponses.front().size()), true);
		EXPECT_TRUE(handledWithLogging);
		EXPECT_EQ(ROSE_NOERROR, secondPendingInvoke.get());
		EXPECT_TRUE(HasBerLog(ClientLogs()));
	}
};

TEST(PublicApiSmokeTest, TransportEncodingCanBeSetAndReadBack)
{
	ObservedRuntimeEndpoint endpoint{L"ApiEndpoint", "api-session"};

	EXPECT_TRUE(endpoint.SetTransportEncoding(TransportEncoding::BER));
	EXPECT_EQ(TransportEncoding::BER, endpoint.GetTransportEncoding());
	EXPECT_TRUE(endpoint.SetTransportEncoding(TransportEncoding::JSON_NO_HEADING));
	EXPECT_EQ(TransportEncoding::JSON_NO_HEADING, endpoint.GetTransportEncoding());
}

TEST(PublicApiSmokeTest, SetSnaccROSETransportRoutesOutboundBytes)
{
	ObservedRuntimeEndpoint endpoint{L"TransportEndpoint", "transport-session"};
	TransportProbe transport;
	endpoint.SetTransportEncoding(TransportEncoding::JSON_NO_HEADING);
	endpoint.SetSnaccROSETransport(&transport);

	ENetUC_Settings_ManagerROSE component(&endpoint);
	AsnSettingsChangedArgument eventArgument;
	SetSettingsValues(eventArgument.settings, true, "transport-user");

	const long roseResult = component.Event_asnSettingsChanged(&eventArgument);
	EXPECT_EQ(ROSE_NOERROR, roseResult);
	ASSERT_EQ(1u, transport.m_payloads.size());
	EXPECT_NE(std::string::npos, transport.m_payloads.front().find("transport-user"));
}

TEST_F(PublicApiRuntimeTest, OnBinaryDataBlockLoggingFlagControlsRawPayloadLogging)
{
	AssertOnBinaryDataBlockLoggingFlagControlsRawPayloadLogging();
}

TEST_F(PublicApiRuntimeTest, OnBinaryDataBlockResultLoggingFlagControlsResponseLogging)
{
	AssertOnBinaryDataBlockResultLoggingFlagControlsResponseLogging();
}

TEST(PublicApiSmokeTest, OnBinaryDataBlockResultDecodeErrorsInvokeHookBer)
{
	AssertOnBinaryDataBlockResultDecodeErrorsInvokeHook(TransportEncoding::BER);
}

TEST(PublicApiSmokeTest, OnBinaryDataBlockResultDecodeErrorsInvokeHookJson)
{
	AssertOnBinaryDataBlockResultDecodeErrorsInvokeHook(TransportEncoding::JSON);
}

TEST_F(PublicApiRuntimeTest, StopProcessingBlocksNewOutboundInvokesAndEvents)
{
	InitializeEndpoints(TransportEncoding::JSON);
	m_client.StopProcessing(true);

	AsnGetSettingsArgument argument;
	AsnGetSettingsResult result;
	AsnRequestError error;

	const long invokeResult = m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error);
	EXPECT_EQ(ROSE_TE_SHUTDOWN, invokeResult);

	ENetUC_Settings_ManagerROSE component(&m_client);
	AsnSettingsChangedArgument eventArgument;
	SetSettingsValues(eventArgument.settings, true, "blocked-user");
	const long eventResult = component.Event_asnSettingsChanged(&eventArgument);
	EXPECT_EQ(ROSE_TE_SHUTDOWN, eventResult);
}

TEST_F(PublicApiRuntimeTest, StopProcessingBlocksInboundDispatchUntilReEnabled)
{
	InitializeEndpoints(TransportEncoding::JSON);
	m_server.StopProcessing(true);

	AsnSetSettingsArgument argument;
	SetSettingsValues(argument.settings, true, "server-should-ignore");
	AsnSetSettingsResult result;
	AsnRequestError error;

	const long blockedResult = m_clientSettingsModule.InvokeSetSettings(&argument, &result, &error, 250);
	EXPECT_NE(ROSE_NOERROR, blockedResult);
	EXPECT_EQ(0, ClientSettingsEventCount());

	m_server.StopProcessing(false);

	const long resumedResult = m_clientSettingsModule.InvokeSetSettings(&argument, &result, &error, 250);
	EXPECT_EQ(ROSE_NOERROR, resumedResult);
	EXPECT_EQ(1, ClientSettingsEventCount());
}

TEST(PublicApiSmokeTest, ConfigureFileLoggingWritesAndCanBeDisabled)
{
	ObservedRuntimeEndpoint endpoint{L"FileLoggingEndpoint", "file-session"};
	TransportProbe transport;
	SnaccRoseOperationLookup::CleanUp();
	ENetUC_Settings_ManagerROSE::RegisterOperations();
	endpoint.SetTransportEncoding(TransportEncoding::JSON_NO_HEADING);
	endpoint.SetSnaccROSETransport(&transport);
	endpoint.SetLogLevels(EAsnLogLevel::JSON, EAsnLogLevel::JSON);
	endpoint.SetForwardLogsToBase(true);

	const std::filesystem::path logPath = std::filesystem::temp_directory_path() / "snacc_runtime_logging_test.json";
	std::filesystem::remove(logPath);
	ASSERT_EQ(0, endpoint.ConfigureFileLogging(logPath.c_str(), false, true));

	ENetUC_Settings_ManagerROSE component(&endpoint);
	AsnSettingsChangedArgument eventArgument;
	SetSettingsValues(eventArgument.settings, true, "file-log-user");

	const long roseResult = component.Event_asnSettingsChanged(&eventArgument);
	EXPECT_EQ(ROSE_NOERROR, roseResult);
	EXPECT_EQ(0, endpoint.ConfigureFileLogging(nullptr));

	std::string fileContents;
	{
		std::ifstream input(logPath, std::ios::binary);
		ASSERT_TRUE(input.is_open());
		fileContents.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
	}
	EXPECT_NE(std::string::npos, fileContents.find("file-log-user"));

	std::filesystem::remove(logPath);
	SnaccRoseOperationLookup::CleanUp();
}

TEST(PublicApiSmokeTest, OperationLookupRegistersAndCleansUp)
{
	SnaccRoseOperationLookup::CleanUp();
	EXPECT_FALSE(SnaccRoseOperationLookup::Initialized());

	SnaccRoseOperationLookup::RegisterOperation(3210, "testOperation", 77);
	EXPECT_TRUE(SnaccRoseOperationLookup::Initialized());
	EXPECT_EQ(3210u, SnaccRoseOperationLookup::LookUpID("testOperation"));
	ASSERT_NE(nullptr, SnaccRoseOperationLookup::LookUpName(3210));
	EXPECT_STREQ("testOperation", SnaccRoseOperationLookup::LookUpName(3210));
	EXPECT_EQ(77u, SnaccRoseOperationLookup::LookUpInterfaceID(3210));

	SnaccRoseOperationLookup::CleanUp();
	EXPECT_FALSE(SnaccRoseOperationLookup::Initialized());
	EXPECT_EQ(0u, SnaccRoseOperationLookup::LookUpID("testOperation"));
	EXPECT_EQ(nullptr, SnaccRoseOperationLookup::LookUpName(3210));
	EXPECT_EQ(0u, SnaccRoseOperationLookup::LookUpInterfaceID(3210));
}

TEST(PublicApiSmokeTest, TelemetryDebugTextCoversGroupedRejectReasons)
{
	EXPECT_STREQ("DISPATCHED", SnaccTelemetryData::GetDebugText(SnaccTelemetryData::Outcome::DISPATCHED));
	EXPECT_STREQ("REJECT_PROTOCOL", SnaccTelemetryData::GetDebugText(SnaccTelemetryData::Reason::REJECT_PROTOCOL));
	EXPECT_STREQ("REJECT_SESSION", SnaccTelemetryData::GetDebugText(SnaccTelemetryData::Reason::REJECT_SESSION));
	EXPECT_STREQ("REJECT_AUTHENTICATION", SnaccTelemetryData::GetDebugText(SnaccTelemetryData::Reason::REJECT_AUTHENTICATION));
}

TEST(PublicApiSmokeTest, TelemetryFinalizeCapturesStoredMetadata)
{
	auto telemetry = SnaccTelemetryData::Create(SnaccTelemetryData::Direction::OUTBOUND, 99, "testOperation", 12);
	telemetry->finalize(SnaccTelemetryData::Outcome::RESULT, SnaccTelemetryData::Stage::OUTBOUND_WAIT, SnaccTelemetryData::Reason::REMOTE_RESULT, ROSE_NOERROR, 7u);

	EXPECT_EQ(SnaccTelemetryData::Outcome::RESULT, telemetry->m_Outcome);
	EXPECT_EQ(SnaccTelemetryData::Stage::OUTBOUND_WAIT, telemetry->m_Stage);
	EXPECT_EQ(SnaccTelemetryData::Reason::REMOTE_RESULT, telemetry->m_Reason);
	ASSERT_TRUE(telemetry->m_lRoseResult.has_value());
	EXPECT_EQ(ROSE_NOERROR, telemetry->m_lRoseResult.value());
	ASSERT_TRUE(telemetry->m_stResponseData.has_value());
	EXPECT_EQ(7u, telemetry->m_stResponseData.value());
}

} // namespace sample_runtime_tests
