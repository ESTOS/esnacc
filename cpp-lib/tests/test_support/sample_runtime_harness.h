#pragma once

#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <deque>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "ENetUC_Common.h"
#include "ENetUC_Event_Manager.h"
#include "ENetUC_Event_ManagerROSE.h"
#include "ENetUC_Event_ManagerROSEInterface.h"
#include "ENetUC_Settings_Manager.h"
#include "ENetUC_Settings_ManagerROSE.h"
#include "ENetUC_Settings_ManagerROSEInterface.h"
#include <cpp-lib/include/SNACCROSE.h>
#include <cpp-lib/include/SnaccROSEBase.h>

namespace sample_runtime_tests
{
using namespace SNACC;

// Helper that fills the sample settings payload in the same way production code
// would construct an optional ASN.1 structure before sending it.
inline void SetSettingsValues(AsnSomeSettings& settings, bool enabled, const std::string& username)
{
	if (!settings.bEnabled)
		settings.bEnabled = new AsnBool();
	*settings.bEnabled = enabled;

	if (!settings.u8sUsername)
		settings.u8sUsername = new UTF8String();
	settings.u8sUsername->setASCII(username);
}

// Helper that reads an optional ASN.1 boolean and falls back to false when the
// field is absent in the decoded payload.
inline bool GetOptionalBool(const AsnSomeSettings& settings)
{
	return settings.bEnabled ? static_cast<bool>(*settings.bEnabled) : false;
}

// Helper that reads an optional ASN.1 string and falls back to an empty string
// when the field is absent in the decoded payload.
inline std::string GetOptionalString(const AsnSomeSettings& settings)
{
	return settings.u8sUsername ? settings.u8sUsername->getASCII() : std::string();
}

// Converts the ASCII session id used in tests into the wchar_t form expected by
// the base runtime methods that encode result and error responses.
inline std::wstring ToWide(const std::string& value)
{
	return std::wstring(value.begin(), value.end());
}

// Extracts the session id that travelled with an invoke so the custom test
// context can retain it for later inspection.
inline std::string GetInvokeSessionId(const SNACC::ROSEInvoke* pInvoke)
{
	if (!pInvoke || !pInvoke->sessionID)
		return std::string();
	return pInvoke->sessionID->getASCII();
}

// Captures one logical log entry emitted by the runtime after all formatting and
// BER/JSON conversion logic has already been applied.
struct CapturedLogEntry
{
	bool bOutbound = false;       // true for outbound traffic, false for inbound traffic
	bool bError = false;          // true when the entry represents an error log
	std::string strOperationName; // operation name attached by the runtime if available
	std::string strPayload;       // final JSON or BER payload written to the log sink
};

// Enumerates the transport behaviors the loopback transport can simulate for a
// single outbound send.
enum class TransportActionKind
{
	Deliver,  // forward the payload to the peer immediately
	Drop,     // silently swallow the payload to simulate packet loss
	Fail,     // return a transport-layer error without delivering anything
	Queue,    // hold the payload until the test flushes it manually
	Truncate  // deliver only part of the payload to simulate wire corruption
};

// Configures one simulated behavior for the next transport send operation.
struct TransportAction
{
	TransportActionKind kind = TransportActionKind::Deliver; // behavior to apply to the next send
	long result = ROSE_TE_TRANSPORTFAILED;                   // transport error returned for Fail

	// Convenience factory for a dropped outbound frame.
	static TransportAction Drop()
	{
		return {TransportActionKind::Drop, ROSE_NOERROR};
	}

	// Convenience factory for an explicit transport failure.
	static TransportAction Fail(long failure = ROSE_TE_TRANSPORTFAILED)
	{
		return {TransportActionKind::Fail, failure};
	}

	// Convenience factory for delaying a frame until FlushQueuedMessages().
	static TransportAction Queue()
	{
		return {TransportActionKind::Queue, ROSE_NOERROR};
	}

	// Convenience factory for partial payload delivery.
	static TransportAction Truncate()
	{
		return {TransportActionKind::Truncate, ROSE_NOERROR};
	}
};

// Bundles module behavior switches so tests can turn individual handlers into
// rejects or application errors without rewriting the module implementation.
struct HandlerModes
{
	bool implementGetSettings = true;           // when false the settings get handler returns a reject
	bool getSettingsReturnsError = false;       // when true the settings get handler returns ROSEError
	bool implementSetSettings = true;           // when false the settings set handler returns a reject
	bool setSettingsReturnsError = false;       // when true the settings set handler returns ROSEError
	bool implementCreateFancyEvents = true;     // when false the event creator returns a reject
	bool createFancyEventsReturnsError = false; // when true the event creator returns ROSEError
};

class RuntimeEndpoint;

// In-memory transport used by both client and server session hosts. It lets the
// tests inject transport failures without needing sockets or threads.
class LoopbackTransport : public ISnaccROSETransport
{
public:
	// Binds the transport to the sending endpoint and its loopback peer.
	LoopbackTransport(RuntimeEndpoint& owner, RuntimeEndpoint& remote)
		: m_owner(owner),
		  m_remote(remote)
	{
	}

	// Called by SnaccROSEBase whenever a payload should be sent to the peer.
	long SendBinaryDataBlockEx(const char* lpBytes, size_t size, SnaccInvokeContext& ctx) override;

	// Queues one synthetic transport behavior for the next outbound send.
	void EnqueueAction(TransportAction action)
	{
		m_actions.push_back(action);
	}

	// Delivers all queued payloads in the order they were originally sent.
	void FlushQueuedMessages();

	// Moves all queued payloads out so tests can feed them into a runtime entry point manually.
	std::vector<std::string> TakeQueuedMessages()
	{
		auto queuedMessages = std::move(m_queuedMessages);
		m_queuedMessages.clear();
		return queuedMessages;
	}

	// Returns the number of payloads currently waiting for FlushQueuedMessages().
	size_t QueuedMessageCount() const
	{
		return m_queuedMessages.size();
	}

private:
	RuntimeEndpoint& m_owner;                // endpoint that performs outbound sends
	RuntimeEndpoint& m_remote;               // peer endpoint that receives delivered payloads
	std::deque<TransportAction> m_actions;   // scripted behaviors consumed one send at a time
	std::vector<std::string> m_queuedMessages; // delayed payloads kept until the test flushes them
};

// Small abstraction that allows the runtime host to route inbound traffic by
// generated interface id instead of knowing concrete module types.
class IRuntimeModule
{
public:
	// Virtual destructor for safe polymorphic cleanup.
	virtual ~IRuntimeModule() = default;
	// Returns the generated interface id used for dispatch lookup.
	virtual unsigned int InterfaceId() const = 0;
	// Hands the decoded ROSE message to the generated dispatcher owned by the module.
	virtual long Dispatch(std::unique_ptr<SNACC::ROSEMessage>& pMsg, SnaccInvokeContext& ctx, std::string& strResponse) = 0;
};

// Test-specific invoke context that remembers both the local session owning the
// runtime host and the session id that arrived on the invoke itself.
class SessionInvokeContext : public SnaccInvokeContext
{
public:
	// Factory used by RuntimeEndpoint::CreateInvokeContext() to build the richer context.
	static std::shared_ptr<SessionInvokeContext> Create(const SnaccInvokeContextInit& init, const std::string& localSessionId)
	{
		return std::shared_ptr<SessionInvokeContext>(new SessionInvokeContext(init, localSessionId));
	}

	// Keeps the concrete type when the base runtime clones the invoke context.
	std::shared_ptr<SnaccInvokeContext> Clone() const override
	{
		return std::shared_ptr<SnaccInvokeContext>(new SessionInvokeContext(*this));
	}

	// Placeholder hook for future telemetry tests that may need to detach data.
	void PrepareForTelemetry() override
	{
		m_bPreparedForTelemetry = true;
		if (!m_strTelemetryNote.empty())
			m_strTelemetryNote = "prepared:" + m_strTelemetryNote;
	}

	// Stores a small test marker that should survive context cloning into telemetry.
	void SetTelemetryNote(const std::string& value)
	{
		m_strTelemetryNote = value;
	}

	// Returns the current telemetry marker stored on this context.
	const std::string& TelemetryNote() const
	{
		return m_strTelemetryNote;
	}

	// Returns whether PrepareForTelemetry() has already run on this concrete instance.
	bool WasPreparedForTelemetry() const
	{
		return m_bPreparedForTelemetry;
	}

	const std::string m_strLocalSessionId; // session id of the endpoint creating the context
	const std::string m_strInvokeSessionId; // session id carried on the invoke payload itself

private:
	// Stores the local and wire session ids when the invoke context is created.
	SessionInvokeContext(const SnaccInvokeContextInit& init, const std::string& localSessionId)
		: SnaccInvokeContext(init),
		  m_strLocalSessionId(localSessionId),
		  m_strInvokeSessionId(GetInvokeSessionId(init.m_pInvoke))
	{
	}

	// Copies the session-specific fields when the context is cloned.
	SessionInvokeContext(const SessionInvokeContext& other)
		: SnaccInvokeContext(other),
		  m_strLocalSessionId(other.m_strLocalSessionId),
		  m_strInvokeSessionId(other.m_strInvokeSessionId),
		  m_bPreparedForTelemetry(other.m_bPreparedForTelemetry),
		  m_strTelemetryNote(other.m_strTelemetryNote)
	{
	}

	bool m_bPreparedForTelemetry = false; // indicates whether the telemetry clone was normalized for retention
	std::string m_strTelemetryNote;       // extra test data used to verify clone and prepare semantics
};

// Copies invoke-context fields while the runtime reference is still alive.
class InvokeContextSnapshot
{
public:
	void Reset()
	{
		m_bCaptured = false;
		m_bIsSessionContext = false;
		m_strLocalSessionId.clear();
		m_strInvokeSessionId.clear();
		m_strTelemetryNote.clear();
	}

	void Capture(const SnaccInvokeContext& ctx)
	{
		m_bCaptured = true;
		if (const auto* pSessionContext = dynamic_cast<const SessionInvokeContext*>(&ctx))
		{
			m_bIsSessionContext = true;
			m_strLocalSessionId = pSessionContext->m_strLocalSessionId;
			m_strInvokeSessionId = pSessionContext->m_strInvokeSessionId;
			m_strTelemetryNote = pSessionContext->TelemetryNote();
			return;
		}

		m_bIsSessionContext = false;
		m_strLocalSessionId.clear();
		m_strInvokeSessionId.clear();
		m_strTelemetryNote.clear();
	}

	bool WasCaptured() const
	{
		return m_bCaptured;
	}

	bool IsSessionContext() const
	{
		return m_bIsSessionContext;
	}

	const std::string& LocalSessionId() const
	{
		return m_strLocalSessionId;
	}

	const std::string& InvokeSessionId() const
	{
		return m_strInvokeSessionId;
	}

	const std::string& TelemetryNote() const
	{
		return m_strTelemetryNote;
	}

private:
	bool m_bCaptured = false;
	bool m_bIsSessionContext = false;
	std::string m_strLocalSessionId;
	std::string m_strInvokeSessionId;
	std::string m_strTelemetryNote;
};

// Records inbound handler and transport observations for one server endpoint.
class InboundInvokeObservation
{
public:
	void Reset()
	{
		m_handlerSnapshot.Reset();
		m_lastTransportSnapshot.Reset();
		m_pHandlerContextAddress = nullptr;
		m_bSameInstanceHandlerToTransport = false;
		m_nTransportSendCount = 0;
	}

	void CaptureHandlerContext(SnaccInvokeContext& ctx)
	{
		m_handlerSnapshot.Capture(ctx);
		m_pHandlerContextAddress = &ctx;
		m_bSameInstanceHandlerToTransport = false;
	}

	void CaptureTransportSend(SnaccInvokeContext& ctx)
	{
		m_lastTransportSnapshot.Capture(ctx);
		++m_nTransportSendCount;
		if (m_pHandlerContextAddress != nullptr)
			m_bSameInstanceHandlerToTransport = (&ctx == m_pHandlerContextAddress);
	}

	const InvokeContextSnapshot& HandlerSnapshot() const
	{
		return m_handlerSnapshot;
	}

	const InvokeContextSnapshot& LastTransportSnapshot() const
	{
		return m_lastTransportSnapshot;
	}

	bool HandlerWasInvoked() const
	{
		return m_handlerSnapshot.WasCaptured();
	}

	bool SameInstanceHandlerToTransport() const
	{
		return m_bSameInstanceHandlerToTransport;
	}

	size_t TransportSendCount() const
	{
		return m_nTransportSendCount;
	}

private:
	InvokeContextSnapshot m_handlerSnapshot;
	InvokeContextSnapshot m_lastTransportSnapshot;
	SnaccInvokeContext* m_pHandlerContextAddress = nullptr;
	bool m_bSameInstanceHandlerToTransport = false;
	size_t m_nTransportSendCount = 0;
};

// Records outbound transport observations copied at send time.
class OutboundTransportObservation
{
public:
	void Reset()
	{
		m_lastTransportSnapshot.Reset();
		m_pLastContextAddress = nullptr;
		m_nTransportSendCount = 0;
	}

	void CaptureTransportSend(SnaccInvokeContext& ctx)
	{
		m_lastTransportSnapshot.Capture(ctx);
		m_pLastContextAddress = &ctx;
		++m_nTransportSendCount;
	}

	const InvokeContextSnapshot& LastTransportSnapshot() const
	{
		return m_lastTransportSnapshot;
	}

	const SnaccInvokeContext* LastContextAddress() const
	{
		return m_pLastContextAddress;
	}

	size_t TransportSendCount() const
	{
		return m_nTransportSendCount;
	}

private:
	InvokeContextSnapshot m_lastTransportSnapshot;
	const SnaccInvokeContext* m_pLastContextAddress = nullptr;
	size_t m_nTransportSendCount = 0;
};

// Session-scoped runtime host used on both client and server sides. It models a
// single ROSE connection, owns the transport binding, injects session ids, and
// dispatches inbound invokes to the registered modules.
class RuntimeEndpoint : public SnaccROSEBase
{
public:
	// Creates one connection host with a stable logical session id for tests.
	RuntimeEndpoint(const wchar_t* className, const std::string& sessionId)
		: SnaccROSEBase(className),
		  m_strSessionId(sessionId),
		  m_wstrSessionId(ToWide(sessionId))
	{
		StopProcessing(false);
	}

	// Attaches a loopback transport that forwards outbound data to the peer host.
	void ConnectTo(RuntimeEndpoint& remote)
	{
		m_transport = std::make_unique<LoopbackTransport>(*this, remote);
		SetSnaccROSETransport(m_transport.get());
	}

	// Clears invoke-context observations recorded by the test harness.
	void ResetInvokeContextObservations()
	{
		m_inboundObservation.Reset();
		m_outboundTransportObservation.Reset();
	}

	// Returns inbound handler and response-send observations for this endpoint.
	const InboundInvokeObservation& InboundObservation() const
	{
		return m_inboundObservation;
	}

	// Returns outbound transport observations copied at send time.
	const OutboundTransportObservation& OutboundTransportObservationState() const
	{
		return m_outboundTransportObservation;
	}

	// Records the invoke context seen inside an inbound handler callback.
	void RecordInboundHandlerContext(SnaccInvokeContext& ctx)
	{
		m_inboundObservation.CaptureHandlerContext(ctx);
	}

	// Records an outbound transport send on this endpoint.
	void RecordTransportSend(SnaccInvokeContext& ctx)
	{
		m_inboundObservation.CaptureTransportSend(ctx);
		m_outboundTransportObservation.CaptureTransportSend(ctx);
	}

	// Selects the wire encoding used for all outbound traffic on this host.
	void SetEncoding(const TransportEncoding encoding)
	{
		if (!SetTransportEncoding(encoding))
			throw std::runtime_error("failed to set transport encoding");
	}

	// Registers one module so inbound operation ids can be mapped to a dispatcher.
	void RegisterModule(IRuntimeModule& module)
	{
		m_modules[module.InterfaceId()] = &module;
	}

	// Exposes the session id so tests and custom contexts can assert it later.
	const std::string& SessionId() const
	{
		return m_strSessionId;
	}

	// Creates the same session-aware context the runtime would construct for an
	// outbound invoke after the session id has been attached to the payload.
	std::shared_ptr<SessionInvokeContext> CreateSessionInvokeContext(SNACC::ROSEInvoke* pInvoke, const char* szOperationName)
	{
		AttachSessionId(pInvoke);
		SnaccInvokeContextInit init(SnaccInvokeDirection::OUTBOUND, pInvoke, szOperationName);
		return SessionInvokeContext::Create(init, m_strSessionId);
	}

	// Gives tests direct access to transport scripting helpers.
	LoopbackTransport& Transport()
	{
		return *m_transport;
	}

	const LoopbackTransport& Transport() const
	{
		return *m_transport;
	}

	// Injects already encoded transport data directly into the inbound pipeline.
	void ReceiveRaw(const std::string& payload)
	{
		OnBinaryDataBlock(payload.data(), static_cast<unsigned long>(payload.size()));
	}

	// The test harness disables runtime logging to keep test output deterministic.
	EAsnLogLevel GetLogLevel(const bool /* bOutbound */) override
	{
		return EAsnLogLevel::DISABLED;
	}

	// Creates the test-specific invoke context carrying local and wire session ids.
	std::shared_ptr<SnaccInvokeContext> CreateInvokeContext(const SnaccInvokeContextInit& init) override
	{
		return SessionInvokeContext::Create(init, m_strSessionId);
	}

	// Attaches the endpoint session id before the base runtime encodes an invoke.
	long SendInvoke(SNACC::ROSEInvoke* pInvoke, SNACC::AsnType* pResult, SNACC::AsnType* pError, const char* szOperationName, int iTimeout = -1, std::shared_ptr<SnaccInvokeContext> pCtx = {}) override
	{
		AttachSessionId(pInvoke);
		return SnaccROSEBase::SendInvoke(pInvoke, pResult, pError, szOperationName, iTimeout, std::move(pCtx));
	}

	// Attaches the endpoint session id before the base runtime encodes an event.
	long SendEvent(SNACC::ROSEInvoke* pInvoke, const char* szOperationName, std::shared_ptr<SnaccInvokeContext> pCtx = {}) override
	{
		AttachSessionId(pInvoke);
		return SnaccROSEBase::SendEvent(pInvoke, szOperationName, std::move(pCtx));
	}

	// Ensures outbound result payloads inherit the session id of this connection.
	long EncodeResult(unsigned int uiInvokeID, const SNACC::AsnType* pResult, std::string& strResponse, const wchar_t* szSessionID = nullptr) override
	{
		return SnaccROSEBase::EncodeResult(uiInvokeID, pResult, strResponse, szSessionID ? szSessionID : m_wstrSessionId.c_str());
	}

	// Ensures outbound error payloads inherit the session id of this connection.
	long EncodeError(unsigned int uiInvokeID, const SNACC::AsnType* pError, std::string& strResponse, const wchar_t* szSessionID = nullptr) override
	{
		return SnaccROSEBase::EncodeError(uiInvokeID, pError, strResponse, szSessionID ? szSessionID : m_wstrSessionId.c_str());
	}

	// Generic interface-id based dispatcher used by all tests in this harness.
	long OnInvoke(std::unique_ptr<ROSEMessage>& pMsg, SnaccInvokeContext& ctx, std::string& strResponse) override
	{
		const unsigned int interfaceId = SnaccRoseOperationLookup::LookUpInterfaceID(pMsg->invoke->operationID);
		const auto it = m_modules.find(interfaceId);
		if (it == m_modules.end())
			return ROSE_REJECT_UNKNOWNOPERATION;

		return it->second->Dispatch(pMsg, ctx, strResponse);
	}

private:
	// Writes the session id into an outbound invoke before the runtime encodes it.
	void AttachSessionId(SNACC::ROSEInvoke* pInvoke)
	{
		if (!pInvoke)
			return;
		if (!pInvoke->sessionID)
			pInvoke->sessionID = new UTF8String();
		pInvoke->sessionID->setASCII(m_strSessionId);
	}

	std::unique_ptr<LoopbackTransport> m_transport; // transport hook used by the base runtime for outbound sends
	std::map<unsigned int, IRuntimeModule*> m_modules; // registered modules keyed by generated interface id
	std::string m_strSessionId; // narrow session id used in test assertions and invoke payloads
	std::wstring m_wstrSessionId; // wide session id used by SnaccROSEBase result/error encoders
	InboundInvokeObservation m_inboundObservation; // inbound handler and transport snapshots
	OutboundTransportObservation m_outboundTransportObservation; // outbound transport snapshots
};

// Runtime endpoint used by the test fixture when log assertions are required.
// It captures the final log entries while still allowing optional forwarding to
// the file-based logger implemented in SnaccROSEBase.
class ObservedRuntimeEndpoint : public RuntimeEndpoint
{
public:
	using RuntimeEndpoint::RuntimeEndpoint;

	// Configures the active log levels independently for inbound and outbound traffic.
	void SetLogLevels(const EAsnLogLevel inbound, const EAsnLogLevel outbound)
	{
		m_inboundLogLevel = inbound;
		m_outboundLogLevel = outbound;
	}

	// Controls whether captured log entries should additionally be written to the
	// normal file sink via SnaccROSEBase::PrintJSONToLog().
	void SetForwardLogsToBase(const bool bForward)
	{
		m_bForwardLogsToBase = bForward;
	}

	// Clears all captured log entries so each test can start from a known state.
	void ClearCapturedLogs()
	{
		m_logEntries.clear();
	}

	// Returns the captured log entries in emission order.
	const std::vector<CapturedLogEntry>& CapturedLogs() const
	{
		return m_logEntries;
	}

	// Returns the number of captured log entries.
	size_t CapturedLogCount() const
	{
		return m_logEntries.size();
	}

	// Uses the test-controlled log level instead of hard-wiring logging off.
	EAsnLogLevel GetLogLevel(const bool bOutbound) override
	{
		return bOutbound ? m_outboundLogLevel : m_inboundLogLevel;
	}

	// Captures the final log entry and optionally forwards it to the base file logger.
	bool PrintJSONToLog(const bool bOutbound, const bool bError, const char* szOperationName, const char* szData, const size_t size = 0) override
	{
		if (szData)
		{
			CapturedLogEntry entry;
			entry.bOutbound = bOutbound;
			entry.bError = bError;
			entry.strOperationName = szOperationName ? szOperationName : "";
			entry.strPayload.assign(szData, size ? size : strlen(szData));
			m_logEntries.push_back(std::move(entry));
		}

		if (m_bForwardLogsToBase)
			RuntimeEndpoint::PrintJSONToLog(bOutbound, bError, szOperationName, szData, size);

		return szData != nullptr;
	}

private:
	EAsnLogLevel m_inboundLogLevel = EAsnLogLevel::DISABLED;  // active inbound log level for the endpoint
	EAsnLogLevel m_outboundLogLevel = EAsnLogLevel::DISABLED; // active outbound log level for the endpoint
	bool m_bForwardLogsToBase = false;                        // whether logs are also forwarded into file logging
	std::vector<CapturedLogEntry> m_logEntries;              // captured final log entries for assertions
};

// Collects telemetry callback invocations from the process-wide runtime callback
// registration point so tests can assert lifecycle metadata afterward.
class TelemetryRecorder : public SnaccTelemetryCallback
{
public:
	// Stores each telemetry record in arrival order.
	void OnInvokeProcessed(std::shared_ptr<const SnaccTelemetryData> pTelemetry) override
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		m_entries.push_back(std::move(pTelemetry));
	}

	// Clears all previously recorded telemetry entries.
	void Clear()
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		m_entries.clear();
	}

	// Returns a stable snapshot of all telemetry entries seen so far.
	std::vector<std::shared_ptr<const SnaccTelemetryData>> Snapshot() const
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		return m_entries;
	}

private:
	mutable std::mutex m_mutex;                                      // protects asynchronous callback delivery
	std::vector<std::shared_ptr<const SnaccTelemetryData>> m_entries; // recorded telemetry callbacks
};

// Small RAII helper that installs a telemetry recorder for one test scope and
// reliably unregisters it again even when assertions fail.
class ScopedTelemetryCapture
{
public:
	explicit ScopedTelemetryCapture(TelemetryRecorder& recorder)
	{
		recorder.Clear();
		SnaccROSEBase::SetTelemetryCallback(&recorder);
	}

	~ScopedTelemetryCapture()
	{
		SnaccROSEBase::SetTelemetryCallback(nullptr);
	}
};

// Server-side module implementing the generated settings interface. It owns the
// generated ROSE component so events originate from the module instead of from
// the raw connection host.
class SettingsServiceModule : public ENetUC_Settings_ManagerROSEInterface, public IRuntimeModule
{
public:
	// Registers the module immediately so the host can dispatch settings invokes to it.
	explicit SettingsServiceModule(RuntimeEndpoint& endpoint)
		: m_endpoint(endpoint),
		  m_component(&endpoint)
	{
		m_endpoint.RegisterModule(*this);
	}

	// Lets tests switch individual handlers into reject or error behavior.
	void ConfigureHandlers(const HandlerModes& modes)
	{
		m_handlerModes = modes;
	}

	// Restores the module to the baseline "working server" state before each test.
	void ResetState()
	{
		m_handlerModes = HandlerModes{};
		m_currentEnabled = false;
		m_currentUsername = "initial-user";
	}

	// Returns the generated interface id for settings operations and events.
	unsigned int InterfaceId() const override
	{
		return ENetUC_Settings_ManagerROSE::m_iid;
	}

	// Calls the generated dispatcher for all settings traffic addressed to this module.
	long Dispatch(std::unique_ptr<SNACC::ROSEMessage>& pMsg, SnaccInvokeContext& ctx, std::string& strResponse) override
	{
		return ENetUC_Settings_ManagerROSE::OnInvoke(pMsg, &m_endpoint, this, ctx, strResponse);
	}

	// Implements the sample "get settings" invoke on the server side.
	InvokeResult OnInvoke_asnGetSettings(AsnGetSettingsArgument* /* argument */, AsnGetSettingsResult* result, AsnRequestError* error, SnaccInvokeContext& ctx) override
	{
		m_endpoint.RecordInboundHandlerContext(ctx);

		if (!m_handlerModes.implementGetSettings)
			return InvokeResult::returnReject;
		if (m_handlerModes.getSettingsReturnsError)
		{
			error->iErrorDetail = 7001;
			error->u8sErrorString.setASCII("get settings handler returned error");
			return InvokeResult::returnError;
		}

		SetSettingsValues(result->settings, m_currentEnabled, m_currentUsername);
		return InvokeResult::returnResult;
	}

	// Implements the sample "set settings" invoke and emits the settings changed event.
	InvokeResult OnInvoke_asnSetSettings(AsnSetSettingsArgument* argument, AsnSetSettingsResult* /* result */, AsnRequestError* error, SnaccInvokeContext& /* ctx */) override
	{
		if (!m_handlerModes.implementSetSettings)
			return InvokeResult::returnReject;
		if (m_handlerModes.setSettingsReturnsError)
		{
			error->iErrorDetail = 7002;
			error->u8sErrorString.setASCII("set settings handler returned error");
			return InvokeResult::returnError;
		}

		m_currentEnabled = GetOptionalBool(argument->settings);
		m_currentUsername = GetOptionalString(argument->settings);

		AsnSettingsChangedArgument eventArgument;
		SetSettingsValues(eventArgument.settings, m_currentEnabled, m_currentUsername);
		const long sendResult = m_component.Event_asnSettingsChanged(&eventArgument);
		if (sendResult != ROSE_NOERROR)
		{
			error->iErrorDetail = sendResult;
			error->u8sErrorString.setASCII("settings changed event dispatch failed");
			return InvokeResult::returnError;
		}

		return InvokeResult::returnResult;
	}

private:
	RuntimeEndpoint& m_endpoint;           // hosting connection used for generated dispatch and wire encoding
	ENetUC_Settings_ManagerROSE m_component; // generated settings ROSE component owned by this module
	HandlerModes m_handlerModes;          // active handler behavior switches for negative tests
	bool m_currentEnabled = false;        // persisted server-side settings state used by get/set calls
	std::string m_currentUsername = "initial-user"; // persisted server-side username used by get/set calls
};

// Client-side settings module. It owns the generated client stub and also
// implements the generated event interface so server-side events land in the
// same module that originated the invoke.
class SettingsClientModule : public ENetUC_Settings_ManagerROSEInterface, public IRuntimeModule
{
public:
	// Registers the client module so inbound settings events/results are dispatched here.
	explicit SettingsClientModule(RuntimeEndpoint& endpoint)
		: m_endpoint(endpoint),
		  m_component(&endpoint)
	{
		m_endpoint.RegisterModule(*this);
	}

	// Clears the observed event state before each test run.
	void ResetState()
	{
		m_settingsEventCount = 0;
		m_lastSettingsEventEnabled = false;
		m_lastSettingsEventUsername.clear();
	}

	// Returns the generated interface id so the host can dispatch settings traffic here.
	unsigned int InterfaceId() const override
	{
		return ENetUC_Settings_ManagerROSE::m_iid;
	}

	// Uses the generated settings dispatcher for inbound results/events on the client.
	long Dispatch(std::unique_ptr<SNACC::ROSEMessage>& pMsg, SnaccInvokeContext& ctx, std::string& strResponse) override
	{
		return ENetUC_Settings_ManagerROSE::OnInvoke(pMsg, &m_endpoint, this, ctx, strResponse);
	}

	// Issues the generated "get settings" invoke from the client side.
	long InvokeGetSettings(AsnGetSettingsArgument* argument, AsnGetSettingsResult* result, AsnRequestError* error, int timeoutMs = -1)
	{
		return m_component.Invoke_asnGetSettings(argument, result, error, timeoutMs);
	}

	// Issues the same invoke through the raw runtime API so tests can attach a custom context.
	long InvokeGetSettingsWithContext(AsnGetSettingsArgument* argument, AsnGetSettingsResult* result, AsnRequestError* error, std::shared_ptr<SnaccInvokeContext> pCtx, int timeoutMs = -1)
	{
		SnaccScopedInvokeMessage invokeMsg(m_endpoint.GetNextInvokeID(), OPID_asnGetSettings, argument);
		return m_endpoint.SendInvoke(invokeMsg.GetPtr(), result, error, "asnGetSettings", timeoutMs, std::move(pCtx));
	}

	// Issues the generated "set settings" invoke from the client side.
	long InvokeSetSettings(AsnSetSettingsArgument* argument, AsnSetSettingsResult* result, AsnRequestError* error, int timeoutMs = -1)
	{
		return m_component.Invoke_asnSetSettings(argument, result, error, timeoutMs);
	}

	// Sends an invoke with an unregistered operation id to exercise reject handling.
	long InvokeUnknownOperation(int timeoutMs = 250)
	{
		AsnGetSettingsArgument argument;
		AsnGetSettingsResult result;
		AsnRequestError error;
		SnaccScopedInvokeMessage invokeMsg(m_endpoint.GetNextInvokeID(), 4999, &argument);
		return m_endpoint.SendInvoke(invokeMsg.GetPtr(), &result, &error, "asnUnknownOperation", timeoutMs);
	}

	// Sends a settings invoke without an argument payload to exercise decode rejects.
	long InvokeSetSettingsWithoutArgument(AsnSetSettingsResult* result, AsnRequestError* error, int timeoutMs = 250)
	{
		ROSEInvoke invoke;
		invoke.invokeID = m_endpoint.GetNextInvokeID();
		invoke.operationID = OPID_asnSetSettings;
		return m_endpoint.SendInvoke(&invoke, result, error, "asnSetSettings", timeoutMs);
	}

	// Sends a settings invoke with an intentionally wrong argument type.
	long InvokeSetSettingsWithWrongArgument(AsnType* wrongArgument, AsnSetSettingsResult* result, AsnRequestError* error, int timeoutMs = 250)
	{
		SnaccScopedInvokeMessage invokeMsg(m_endpoint.GetNextInvokeID(), OPID_asnSetSettings, wrongArgument);
		return m_endpoint.SendInvoke(invokeMsg.GetPtr(), result, error, "asnSetSettings", timeoutMs);
	}

	// Returns how many settings-changed events the client observed.
	int SettingsEventCount() const
	{
		return m_settingsEventCount;
	}

	// Returns the last event's enabled flag as observed by the client.
	bool LastSettingsEventEnabled() const
	{
		return m_lastSettingsEventEnabled;
	}

	// Returns the last event's username as observed by the client.
	const std::string& LastSettingsEventUsername() const
	{
		return m_lastSettingsEventUsername;
	}

	// Records the settings-changed event emitted by the server module.
	void OnEvent_asnSettingsChanged(AsnSettingsChangedArgument* argument) override
	{
		++m_settingsEventCount;
		m_lastSettingsEventEnabled = GetOptionalBool(argument->settings);
		m_lastSettingsEventUsername = GetOptionalString(argument->settings);
	}

private:
	RuntimeEndpoint& m_endpoint;           // hosting client connection used for invokes and inbound dispatch
	ENetUC_Settings_ManagerROSE m_component; // generated settings ROSE component owned by this module
	int m_settingsEventCount = 0;          // number of settings-changed events seen by the client
	bool m_lastSettingsEventEnabled = false; // last enabled value delivered via event
	std::string m_lastSettingsEventUsername; // last username delivered via event
};

// Server-side event module implementing the generated event-manager interface.
// It owns the generated component so outbound events originate from the module.
class EventServiceModule : public ENetUC_Event_ManagerROSEInterface, public IRuntimeModule
{
public:
	// Registers the module immediately so inbound event-manager invokes are routed here.
	explicit EventServiceModule(RuntimeEndpoint& endpoint)
		: m_endpoint(endpoint),
		  m_component(&endpoint)
	{
		m_endpoint.RegisterModule(*this);
	}

	// Lets tests switch the event generator into reject or error behavior.
	void ConfigureHandlers(const HandlerModes& modes)
	{
		m_handlerModes = modes;
	}

	// Resets the event generator to the baseline working state.
	void ResetState()
	{
		m_handlerModes = HandlerModes{};
	}

	// Returns the generated interface id for event-manager operations.
	unsigned int InterfaceId() const override
	{
		return ENetUC_Event_ManagerROSE::m_iid;
	}

	// Calls the generated dispatcher for inbound event-manager traffic.
	long Dispatch(std::unique_ptr<SNACC::ROSEMessage>& pMsg, SnaccInvokeContext& ctx, std::string& strResponse) override
	{
		return ENetUC_Event_ManagerROSE::OnInvoke(pMsg, &m_endpoint, this, ctx, strResponse);
	}

	// Implements the sample invoke that emits a configurable number of fancy events.
	InvokeResult OnInvoke_asnCreateFancyEvents(AsnCreateFancyEventsArgument* argument, AsnCreateFancyEventsResult* /* result */, AsnRequestError* error, SnaccInvokeContext& /* ctx */) override
	{
		if (!m_handlerModes.implementCreateFancyEvents)
			return InvokeResult::returnReject;
		if (m_handlerModes.createFancyEventsReturnsError)
		{
			error->iErrorDetail = 7003;
			error->u8sErrorString.setASCII("create fancy events handler returned error");
			return InvokeResult::returnError;
		}

		const long long eventCount = argument->iEventCount;
		for (long long index = 0; index < eventCount; ++index)
		{
			AsnFancyEventArgument eventArgument;
			eventArgument.iEventCounter = index + 1;
			eventArgument.iEventsLeft = eventCount - index - 1;
			const long sendResult = m_component.Event_asnFancyEvent(&eventArgument);
			if (sendResult != ROSE_NOERROR)
			{
				error->iErrorDetail = sendResult;
				error->u8sErrorString.setASCII("fancy event dispatch failed");
				return InvokeResult::returnError;
			}
		}

		return InvokeResult::returnResult;
	}

private:
	RuntimeEndpoint& m_endpoint;        // hosting server connection used for generated dispatch
	ENetUC_Event_ManagerROSE m_component; // generated event-manager ROSE component owned by this module
	HandlerModes m_handlerModes;       // active event-module behavior switches for negative tests
};

// Client-side event module. It owns the generated event-manager client stub and
// records inbound fancy-event notifications for later assertions.
class EventClientModule : public ENetUC_Event_ManagerROSEInterface, public IRuntimeModule
{
public:
	// Registers the client module so event notifications are dispatched back here.
	explicit EventClientModule(RuntimeEndpoint& endpoint)
		: m_endpoint(endpoint),
		  m_component(&endpoint)
	{
		m_endpoint.RegisterModule(*this);
	}

	// Clears all observed fancy-event notifications before each test.
	void ResetState()
	{
		m_fancyEvents.clear();
	}

	// Returns the generated interface id for event-manager traffic.
	unsigned int InterfaceId() const override
	{
		return ENetUC_Event_ManagerROSE::m_iid;
	}

	// Uses the generated event-manager dispatcher for inbound notifications.
	long Dispatch(std::unique_ptr<SNACC::ROSEMessage>& pMsg, SnaccInvokeContext& ctx, std::string& strResponse) override
	{
		return ENetUC_Event_ManagerROSE::OnInvoke(pMsg, &m_endpoint, this, ctx, strResponse);
	}

	// Issues the generated "create fancy events" invoke from the client side.
	long InvokeCreateFancyEvents(AsnCreateFancyEventsArgument* argument, AsnCreateFancyEventsResult* result, AsnRequestError* error, int timeoutMs = -1)
	{
		return m_component.Invoke_asnCreateFancyEvents(argument, result, error, timeoutMs);
	}

	// Issues the same invoke through the raw runtime API so tests can attach a custom context.
	long InvokeCreateFancyEventsWithContext(AsnCreateFancyEventsArgument* argument, AsnCreateFancyEventsResult* result, AsnRequestError* error, std::shared_ptr<SnaccInvokeContext> pCtx, int timeoutMs = -1)
	{
		SnaccScopedInvokeMessage invokeMsg(m_endpoint.GetNextInvokeID(), OPID_asnCreateFancyEvents, argument);
		return m_endpoint.SendInvoke(invokeMsg.GetPtr(), result, error, "asnCreateFancyEvents", timeoutMs, std::move(pCtx));
	}

	// Sends the event-manager invoke with an intentionally wrong argument type.
	long InvokeCreateFancyEventsWithWrongArgument(AsnType* wrongArgument, AsnCreateFancyEventsResult* result, AsnRequestError* error, int timeoutMs = 250)
	{
		SnaccScopedInvokeMessage invokeMsg(m_endpoint.GetNextInvokeID(), OPID_asnCreateFancyEvents, wrongArgument);
		return m_endpoint.SendInvoke(invokeMsg.GetPtr(), result, error, "asnCreateFancyEvents", timeoutMs);
	}

	// Returns how many fancy-event notifications were observed by the client.
	size_t FancyEventCount() const
	{
		return m_fancyEvents.size();
	}

	// Returns all fancy-event pairs in the order they were received.
	const std::vector<std::pair<long long, long long>>& FancyEvents() const
	{
		return m_fancyEvents;
	}

	// Records each fancy event as a simple value pair for easy assertions.
	void OnEvent_asnFancyEvent(AsnFancyEventArgument* argument) override
	{
		m_fancyEvents.emplace_back(argument->iEventCounter.GetLongLong(), argument->iEventsLeft.GetLongLong());
	}

private:
	RuntimeEndpoint& m_endpoint;            // hosting client connection used for invokes and dispatch
	ENetUC_Event_ManagerROSE m_component;   // generated event-manager ROSE component owned by this module
	std::vector<std::pair<long long, long long>> m_fancyEvents; // all fancy events observed by the client
};

// Applies the scripted transport behavior and either delivers, queues, drops, or
// corrupts the outbound payload.
inline long LoopbackTransport::SendBinaryDataBlockEx(const char* lpBytes, size_t size, SnaccInvokeContext& ctx)
{
	m_owner.RecordTransportSend(ctx);

	TransportAction action;
	if (!m_actions.empty())
	{
		action = m_actions.front();
		m_actions.pop_front();
	}

	switch (action.kind)
	{
		case TransportActionKind::Deliver:
			m_remote.OnBinaryDataBlock(lpBytes, static_cast<unsigned long>(size));
			return ROSE_NOERROR;
		case TransportActionKind::Drop:
			return ROSE_NOERROR;
		case TransportActionKind::Fail:
			return action.result;
		case TransportActionKind::Queue:
			m_queuedMessages.emplace_back(lpBytes, size);
			return ROSE_NOERROR;
		case TransportActionKind::Truncate:
		{
			const size_t truncatedSize = size > 1 ? size / 2 : size;
			m_remote.OnBinaryDataBlock(lpBytes, static_cast<unsigned long>(truncatedSize));
			return ROSE_NOERROR;
		}
		default:
			throw std::runtime_error("invalid transport action");
	}
}

// Releases all delayed payloads back into the peer endpoint in send order.
inline void LoopbackTransport::FlushQueuedMessages()
{
	for (const auto& payload : m_queuedMessages)
		m_remote.OnBinaryDataBlock(payload.data(), static_cast<unsigned long>(payload.size()));
	m_queuedMessages.clear();
}

// Shared fixture base used by all handwritten test files. It creates one client
// session host and one server session host together with their attached modules.
class RuntimeTestBase : public ::testing::Test
{
protected:
	// Registers generated operation ids once for the whole fixture hierarchy.
	static void SetUpTestSuite()
	{
		SnaccRoseOperationLookup::CleanUp();
		ENetUC_Settings_ManagerROSE::RegisterOperations();
		ENetUC_Event_ManagerROSE::RegisterOperations();
	}

	// Resets the global operation registry after the suite is finished.
	static void TearDownTestSuite()
	{
		SnaccRoseOperationLookup::CleanUp();
	}

	// Constructs all modules around the already constructed client/server hosts.
	RuntimeTestBase()
		: m_telemetryRecorder(),
		  m_telemetryScope(m_telemetryRecorder),
		  m_serverSettingsModule(m_server),
		  m_clientSettingsModule(m_client),
		  m_serverEventModule(m_server),
		  m_clientEventModule(m_client)
	{
	}

	// Reconnects both hosts, selects the encoding, and resets all module state.
	void InitializeEndpoints(const TransportEncoding encoding)
	{
		m_server.ConnectTo(m_client);
		m_client.ConnectTo(m_server);
		m_server.SetEncoding(encoding);
		m_client.SetEncoding(encoding);
		m_serverSettingsModule.ResetState();
		m_clientSettingsModule.ResetState();
		m_serverEventModule.ResetState();
		m_clientEventModule.ResetState();
		m_server.ResetInvokeContextObservations();
		m_client.ResetInvokeContextObservations();
		m_server.SetLogLevels(EAsnLogLevel::DISABLED, EAsnLogLevel::DISABLED);
		m_client.SetLogLevels(EAsnLogLevel::DISABLED, EAsnLogLevel::DISABLED);
		m_server.SetForwardLogsToBase(false);
		m_client.SetForwardLogsToBase(false);
		m_server.ClearCapturedLogs();
		m_client.ClearCapturedLogs();
		m_telemetryRecorder.Clear();
	}

	// Applies one logical-behavior configuration to all server-side modules.
	void ConfigureServerHandlers(const HandlerModes& modes)
	{
		m_serverSettingsModule.ConfigureHandlers(modes);
		m_serverEventModule.ConfigureHandlers(modes);
	}

	// Releases delayed server-to-client traffic configured through LoopbackTransport.
	void FlushServerOutbound()
	{
		m_server.Transport().FlushQueuedMessages();
	}

	// Releases delayed client-to-server traffic configured through LoopbackTransport.
	void FlushClientOutbound()
	{
		m_client.Transport().FlushQueuedMessages();
	}

	// Convenience accessor for the number of settings events observed by the client module.
	int ClientSettingsEventCount() const
	{
		return m_clientSettingsModule.SettingsEventCount();
	}

	// Convenience accessor for the last settings event enabled flag observed by the client.
	bool ClientLastSettingsEventEnabled() const
	{
		return m_clientSettingsModule.LastSettingsEventEnabled();
	}

	// Convenience accessor for the last settings event username observed by the client.
	const std::string& ClientLastSettingsEventUsername() const
	{
		return m_clientSettingsModule.LastSettingsEventUsername();
	}

	// Convenience accessor for the number of fancy events observed by the client module.
	size_t ClientFancyEventCount() const
	{
		return m_clientEventModule.FancyEventCount();
	}

	// Convenience accessor for the full fancy-event history seen by the client.
	const std::vector<std::pair<long long, long long>>& ClientFancyEvents() const
	{
		return m_clientEventModule.FancyEvents();
	}

	// Configures the server endpoint log levels for inbound and outbound traffic.
	void SetServerLogLevels(const EAsnLogLevel inbound, const EAsnLogLevel outbound)
	{
		m_server.SetLogLevels(inbound, outbound);
	}

	// Configures the client endpoint log levels for inbound and outbound traffic.
	void SetClientLogLevels(const EAsnLogLevel inbound, const EAsnLogLevel outbound)
	{
		m_client.SetLogLevels(inbound, outbound);
	}

	// Allows tests to forward captured server logs into the real file sink as well.
	void SetServerLogForwardingToBase(const bool bForward)
	{
		m_server.SetForwardLogsToBase(bForward);
	}

	// Allows tests to forward captured client logs into the real file sink as well.
	void SetClientLogForwardingToBase(const bool bForward)
	{
		m_client.SetForwardLogsToBase(bForward);
	}

	// Exposes the server-side captured log entries for assertions.
	const std::vector<CapturedLogEntry>& ServerLogs() const
	{
		return m_server.CapturedLogs();
	}

	// Exposes the client-side captured log entries for assertions.
	const std::vector<CapturedLogEntry>& ClientLogs() const
	{
		return m_client.CapturedLogs();
	}

	// Returns the telemetry records captured through the process-wide callback hook.
	std::vector<std::shared_ptr<const SnaccTelemetryData>> TelemetryEntries() const
	{
		return m_telemetryRecorder.Snapshot();
	}

	// Returns inbound invoke-context observations recorded on the server endpoint.
	const InboundInvokeObservation& ServerInboundObservation() const
	{
		return m_server.InboundObservation();
	}

	// Returns outbound transport observations recorded on the client endpoint.
	const OutboundTransportObservation& ClientOutboundTransportObservation() const
	{
		return m_client.OutboundTransportObservationState();
	}

	// Gives tests a simple way to create a session-aware outbound context for the client.
	std::shared_ptr<SessionInvokeContext> CreateClientInvokeContext(SNACC::ROSEInvoke* pInvoke, const char* szOperationName)
	{
		return m_client.CreateSessionInvokeContext(pInvoke, szOperationName);
	}

	// Gives tests a simple way to create a session-aware outbound context for the server.
	std::shared_ptr<SessionInvokeContext> CreateServerInvokeContext(SNACC::ROSEInvoke* pInvoke, const char* szOperationName)
	{
		return m_server.CreateSessionInvokeContext(pInvoke, szOperationName);
	}

	// Starts an asynchronous get-settings invoke so lifecycle tests can race it
	// against shutdown or delayed transport behavior.
	std::future<long> InvokeGetSettingsAsync(int timeoutMs = 500)
	{
		return std::async(std::launch::async, [this, timeoutMs]() {
			AsnGetSettingsArgument argument;
			AsnGetSettingsResult result;
			AsnRequestError error;
			return m_clientSettingsModule.InvokeGetSettings(&argument, &result, &error, timeoutMs);
		});
	}

	// Returns one intentionally malformed payload for each encoding so transport
	// tests can inject undecodable inbound data.
	static std::string MalformedPayload(const TransportEncoding encoding)
	{
		if (encoding == TransportEncoding::BER)
			return std::string("\xA1\x01", 2);
		return "J0000007{\"bad\":";
	}

	TelemetryRecorder m_telemetryRecorder;   // collects callback records emitted by SnaccROSEBase
	ScopedTelemetryCapture m_telemetryScope; // keeps telemetry callback capture installed for the fixture lifetime
	ObservedRuntimeEndpoint m_server{L"LoopbackServer", "server-session"}; // server-side session host used by all tests
	ObservedRuntimeEndpoint m_client{L"LoopbackClient", "client-session"}; // client-side session host used by all tests
	SettingsServiceModule m_serverSettingsModule; // server module handling settings invokes
	SettingsClientModule m_clientSettingsModule;  // client module issuing settings invokes and receiving settings events
	EventServiceModule m_serverEventModule;       // server module handling event-manager invokes
	EventClientModule m_clientEventModule;        // client module issuing event invokes and receiving fancy events
};

} // namespace sample_runtime_tests
