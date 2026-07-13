#include "../include/SnaccROSEBase.h"
#include "../include/SNACCROSE.h"
#include "snacc-assert.h"
#include <stdio.h>
#include <wchar.h>
#include <iomanip>
#include <locale>
#include <chrono>
#include <filesystem>
#include <optional>

std::string getPrettyPrinted(const SJson::Value& value);

using namespace SNACC;

namespace
{
	const char* GetOperationNameOrEmpty(const SNACC::ROSEInvoke* pInvoke, const char* szOperationName)
	{
		if (szOperationName)
			return szOperationName;
		else if (pInvoke)
		{
			const char* szResolvedName = SnaccRoseOperationLookup::LookUpName(pInvoke->operationID);
			if (szResolvedName)
				return szResolvedName;
		}
		return "";
	}

	// Binds a stack-owned ROSEReject into a ROSEMessage for EncodeReject without
	// transferring ownership. Detaches in ~dtor so ~ROSEMessage never deletes the
	// caller's reject (including on encode exceptions).
	class RoseEncodeRejectBorrow final
	{
	public:
		RoseEncodeRejectBorrow(SNACC::ROSEMessage& message, SNACC::ROSEReject& reject)
		{
			message.choiceId = ROSEMessage::rejectCid;
			message.reject = &reject;
			m_pMessage = &message;
		}

		~RoseEncodeRejectBorrow() noexcept
		{
			if (m_pMessage)
				m_pMessage->reject = nullptr;
		}

		RoseEncodeRejectBorrow(const RoseEncodeRejectBorrow&) = delete;
		RoseEncodeRejectBorrow& operator=(const RoseEncodeRejectBorrow&) = delete;

	private:
		// ROSEMessage whose reject arm we borrow into for encoding.
		SNACC::ROSEMessage* m_pMessage = nullptr;
	};

	// Builds a stack ROSEResult inside a ROSEMessage for outbound EncodeResult.
	// Owns encode-side allocations; borrows caller AsnType payload. Detach in
	// ~dtor prevents ~ROSEMessage from deleting stack envelopes or caller values.
	class RoseEncodeResultEnvelope final
	{
	public:
		RoseEncodeResultEnvelope(unsigned int uiInvokeID, const AsnType* pResult, const wchar_t* szSessionID)
		{
			m_result.invokeID = uiInvokeID;
			m_result.result = new ROSEResultSeq;
			m_result.result->resultValue = 0;
			m_result.result->result.value = const_cast<AsnType*>(pResult);
			if (szSessionID)
				m_result.sessionID = new UTF8String(szSessionID);
			m_message.choiceId = ROSEMessage::resultCid;
			m_message.result = &m_result;
		}

		~RoseEncodeResultEnvelope() noexcept
		{
			detach();
		}

		SNACC::ROSEMessage& message() noexcept
		{
			return m_message;
		}

		RoseEncodeResultEnvelope(const RoseEncodeResultEnvelope&) = delete;
		RoseEncodeResultEnvelope& operator=(const RoseEncodeResultEnvelope&) = delete;

	private:
		void detach() noexcept
		{
			if (m_message.result == &m_result)
				m_message.result = nullptr;
			if (m_result.result)
				m_result.result->result.value = nullptr;
		}

		// Stack envelope; owns ROSEResultSeq and optional sessionID.
		SNACC::ROSEResult m_result;
		// Encode view; borrows m_result until detach().
		SNACC::ROSEMessage m_message;
	};

	// Builds a stack ROSEError inside a ROSEMessage for outbound EncodeError.
	// Owns encode-side allocations (AsnAny wrapper, optional sessionID); borrows
	// caller AsnType payload. Detach in ~dtor prevents ~ROSEMessage from deleting
	// stack envelopes or caller values (including on encode exceptions).
	class RoseEncodeErrorEnvelope final
	{
	public:
		RoseEncodeErrorEnvelope(unsigned int uiInvokeID, const AsnType* pError, const wchar_t* szSessionID)
		{
			m_error.invokedID = uiInvokeID;
			m_error.error_value = 0;
			m_error.error = new AsnAny();
			m_error.error->value = const_cast<AsnType*>(pError);
			if (szSessionID)
				m_error.sessionID = new UTF8String(szSessionID);
			m_message.choiceId = ROSEMessage::errorCid;
			m_message.error = &m_error;
		}

		~RoseEncodeErrorEnvelope() noexcept
		{
			detach();
		}

		SNACC::ROSEMessage& message() noexcept
		{
			return m_message;
		}

		RoseEncodeErrorEnvelope(const RoseEncodeErrorEnvelope&) = delete;
		RoseEncodeErrorEnvelope& operator=(const RoseEncodeErrorEnvelope&) = delete;

	private:
		void detach() noexcept
		{
			if (m_message.error == &m_error)
				m_message.error = nullptr;
			if (m_error.error)
				m_error.error->value = nullptr;
		}

		// Stack envelope; owns AsnAny wrapper and optional sessionID.
		SNACC::ROSEError m_error;
		// Encode view; borrows m_error until detach().
		SNACC::ROSEMessage m_message;
	};

	// Binds caller-owned ROSEInvoke into a stack ROSEMessage for Send encoding.
	// Detaches invoke pointer in ~dtor so ~ROSEMessage does not delete pInvoke.
	class ScopedEncodeInvokeBorrow final
	{
	public:
		ScopedEncodeInvokeBorrow(SNACC::ROSEMessage& message, SNACC::ROSEInvoke& invoke)
			: m_message(message)
		{
			m_message.choiceId = ROSEMessage::invokeCid;
			m_message.invoke = &invoke;
		}

		~ScopedEncodeInvokeBorrow() noexcept
		{
			m_message.invoke = nullptr;
		}

		ScopedEncodeInvokeBorrow(const ScopedEncodeInvokeBorrow&) = delete;
		ScopedEncodeInvokeBorrow& operator=(const ScopedEncodeInvokeBorrow&) = delete;

	private:
		// Outbound invoke ROSEMessage being encoded.
		SNACC::ROSEMessage& m_message;
	};

	// JSON Send requires operationName on the invoke; adds a temporary name only
	// for encoding when the caller did not set one, and removes it on scope exit.
	class ScopedInvokeOperationName final
	{
	public:
		ScopedInvokeOperationName(SNACC::ROSEInvoke& invoke, const char* szOperationName)
		{
			if (!invoke.operationName && szOperationName)
			{
				m_pInvoke = &invoke;
				invoke.operationName = UTF8String::CreateNewFromASCII(szOperationName);
			}
		}

		~ScopedInvokeOperationName() noexcept
		{
			if (m_pInvoke && m_pInvoke->operationName)
			{
				delete m_pInvoke->operationName;
				m_pInvoke->operationName = nullptr;
			}
		}

		ScopedInvokeOperationName(const ScopedInvokeOperationName&) = delete;
		ScopedInvokeOperationName& operator=(const ScopedInvokeOperationName&) = delete;

	private:
		// Set only when this scope allocated operationName on the invoke.
		SNACC::ROSEInvoke* m_pInvoke = nullptr;
	};
} // namespace

SnaccInvokeContextInit::SnaccInvokeContextInit(SnaccInvokeDirection direction, SNACC::ROSEInvoke* pInvoke, const char* szOperationName /*= nullptr*/)
	: m_direction(direction),
	  m_pInvoke(pInvoke),
	  m_strOperationName(GetOperationNameOrEmpty(pInvoke, szOperationName))
{
}

std::shared_ptr<SnaccInvokeContext> SnaccInvokeContext::Create(const SnaccInvokeContextInit& init)
{
	return std::shared_ptr<SnaccInvokeContext>(new SnaccInvokeContext(init));
}

std::shared_ptr<SnaccInvokeContext> SnaccInvokeContext::Clone() const
{
	return std::shared_ptr<SnaccInvokeContext>(new SnaccInvokeContext(*this));
}

SnaccScopedInvokeMessage::SnaccScopedInvokeMessage(long invokeID, unsigned int uiOperationID, SNACC::AsnType* pArgument)
	: m_pInvoke(new SNACC::ROSEInvoke())
{
	m_pInvoke->invokeID = invokeID;
	m_pInvoke->operationID = uiOperationID;
	m_pInvoke->argument = new AsnAny;
	m_pInvoke->argument->value = pArgument;
}

SnaccScopedInvokeMessage::~SnaccScopedInvokeMessage()
{
	if (m_pInvoke && m_pInvoke->argument)
		m_pInvoke->argument->value = nullptr;
}

SNACC::ROSEInvoke* SnaccScopedInvokeMessage::GetPtr()
{
	return m_pInvoke.get();
}

const SNACC::ROSEInvoke* SnaccScopedInvokeMessage::GetPtr() const
{
	return m_pInvoke.get();
}

SnaccInvokeContext::SnaccInvokeContext(const SnaccInvokeContextInit& init)
{
}

SnaccInvokeContext::SnaccInvokeContext(const SnaccInvokeContext& other)
	: m_lRejectResult(other.m_lRejectResult),
	  m_bResponseIsError(other.m_bResponseIsError)
{
	if (other.m_pRejectAuth)
		m_pRejectAuth = static_cast<SNACC::ROSEAuthResult*>(other.m_pRejectAuth->Clone());
}

SnaccInvokeContext::~SnaccInvokeContext()
{
	if (m_pRejectAuth)
	{
		delete m_pRejectAuth;
		m_pRejectAuth = nullptr;
	}
}

void SnaccInvokeContext::PrepareForTelemetry()
{
}

void SnaccInvokeContext::SetAsyncCompletion(SnaccInvokeAsyncCallback callback, SNACC::AsnType* pResult, SNACC::AsnType* pError)
{
	m_asyncCallback = std::move(callback);
	m_pAsyncResult = pResult;
	m_pAsyncError = pError;
}

bool SnaccInvokeContext::HasAsyncCompletion() const
{
	return static_cast<bool>(m_asyncCallback);
}

const SnaccInvokeAsyncCallback& SnaccInvokeContext::AsyncCallback() const
{
	return m_asyncCallback;
}

SNACC::AsnType* SnaccInvokeContext::AsyncResultBuffer() const
{
	return m_pAsyncResult;
}

SNACC::AsnType* SnaccInvokeContext::AsyncErrorBuffer() const
{
	return m_pAsyncError;
}

std::string getPrettyPrinted(const SJson::Value& value)
{
	SJson::StreamWriterBuilder wbuilder;
	wbuilder["indentation"] = "\t";
	return SJson::writeString(wbuilder, value);
}

std::optional<SnaccROSEBase::InboundInvokeRejectContext> SnaccROSEBase::InboundInvokeRejectContext::TryFrom(const SNACC::ROSEMessage& message)
{
	if (message.choiceId != ROSEMessage::invokeCid || !message.invoke)
		return std::nullopt;
	const auto* pInvoke = message.invoke;
	InboundInvokeRejectContext ctx;
	ctx.m_invokeId = pInvoke->invokeID;
	ctx.m_uiOperationID = pInvoke->operationID;
	const char* szName = SnaccRoseOperationLookup::LookUpName(ctx.m_uiOperationID);
	if (szName)
		ctx.m_strOperationName = szName;
	return ctx;
}

bool SnaccROSEBase::InboundInvokeRejectContext::CanSendReject() const
{
	return m_invokeId != 99999;
}

const char* SnaccROSEBase::InboundInvokeRejectContext::OperationNameCStr() const
{
	return m_strOperationName.empty() ? nullptr : m_strOperationName.c_str();
}

void SnaccROSEBase::EmitInboundDecodeFailureTelemetry(unsigned long ulMessageSize, unsigned int uiOperationID, const char* szOperationName, SnaccTelemetryData::Outcome outcome, long lTelemetryResult)
{
	OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, uiOperationID, szOperationName, ulMessageSize, outcome, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, lTelemetryResult));
}

void SnaccROSEBase::EmitInboundGenericDecodeFailureTelemetry(unsigned long ulMessageSize)
{
	EmitInboundDecodeFailureTelemetry(ulMessageSize, 0, nullptr, SnaccTelemetryData::Outcome::UNHANDLED, ROSE_RE_DECODE_FAILED);
}

SnaccROSEBase::InboundDecodeFailureResult SnaccROSEBase::HandleInboundRoseDecodeFailure(const std::optional<InboundInvokeRejectContext>& rejectCtx, bool bSendReject, const std::string& strRejectDetails)
{
	InboundDecodeFailureResult result;
	if (!rejectCtx)
		return result;
	result.uiOperationID = rejectCtx->m_uiOperationID;
	result.szOperationName = rejectCtx->OperationNameCStr();
	if (!bSendReject || !rejectCtx->CanSendReject())
		return result;

	ROSEReject reject;
	if (rejectCtx->m_invokeId)
	{
		reject.invokedID.choiceId = ROSERejectChoice::invokedIDCid;
		reject.invokedID.invokedID = new AsnInt(rejectCtx->m_invokeId);
	}
	else
	{
		reject.invokedID.choiceId = ROSERejectChoice::invokednullCid;
		reject.invokedID.invokednull = new AsnNull;
	}

	reject.reject = new RejectProblem();
	reject.reject->choiceId = RejectProblem::invokeProblemCid;
	reject.reject->invokeProblem = new InvokeProblem(SNACC::InvokeProblem::mistypedArgument);
	reject.details = UTF8String::CreateNewFromASCII(strRejectDetails.c_str());

	auto pRejectCtx = SnaccInvokeContext::Create(SnaccInvokeContextInit(SnaccInvokeDirection::INBOUND, nullptr, rejectCtx->OperationNameCStr()));
	const long lRejectResult = SendRejectEx(&reject, *pRejectCtx);
	if (lRejectResult == ROSE_NOERROR)
	{
		result.outcome = SnaccTelemetryData::Outcome::REJECT;
		result.lTelemetryResult = ROSE_REJECT_MISTYPEDARGUMENT;
	}
	else
		result.lTelemetryResult = lRejectResult;
	return result;
}

void SnaccROSEBase::HandleInboundEnvelopeSnaccDecodeFailure(SNACC::TransportEncoding transportEncoding, SNACC::TransportEncoding hookEncoding, const char* lpBytes, unsigned long ulMessageSize, bool& bLogTransportData, const SNACC::SnaccException& ex, const std::optional<InboundInvokeRejectContext>& rejectCtx, bool bSendReject, bool bRejectDetailsFromPrettyJson, const char* szMethod)
{
	if (bLogTransportData)
		bLogTransportData = LogTransportData(false, transportEncoding, nullptr, lpBytes, ulMessageSize, nullptr, nullptr);

	SJson::Value error;
	error["exception"] = ex.what();
	error["method"] = szMethod;
	error["error"] = (int)ex.m_errorCode;
	const std::string strError = getPrettyPrinted(error);
	PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());
	OnRoseDecodeError(bLogTransportData, hookEncoding, lpBytes, ulMessageSize, ex.what());

	const std::string rejectDetails = bRejectDetailsFromPrettyJson ? strError : ex.what();
	const auto failure = HandleInboundRoseDecodeFailure(rejectCtx, bSendReject, rejectDetails);
	EmitInboundDecodeFailureTelemetry(ulMessageSize, failure.uiOperationID, failure.szOperationName, failure.outcome, failure.lTelemetryResult);
}

void SnaccROSEBase::HandleInboundJsonParseDecodeFailure(SNACC::TransportEncoding transportEncoding, const char* lpBytes, unsigned long ulMessageSize, bool& bLogTransportData, const std::string& parseError, const char* szMethod)
{
	if (bLogTransportData)
		bLogTransportData = LogTransportData(false, transportEncoding, nullptr, lpBytes, ulMessageSize, nullptr, nullptr);

	SJson::Value error;
	error["exception"] = parseError;
	error["method"] = szMethod;
	const std::string strError = getPrettyPrinted(error);
	PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());
	OnRoseDecodeError(bLogTransportData, transportEncoding, lpBytes, ulMessageSize, parseError);
	EmitInboundGenericDecodeFailureTelemetry(ulMessageSize);
}

void SnaccROSEBase::HandleInboundUnknownEncodingDecodeFailure(const char* lpBytes, unsigned long ulMessageSize, bool& bLogTransportData)
{
	if (bLogTransportData)
		bLogTransportData = LogTransportData(false, SNACC::TransportEncoding::BER, nullptr, lpBytes, ulMessageSize, nullptr, nullptr);
	OnRoseDecodeError(bLogTransportData, SNACC::TransportEncoding::BER, lpBytes, ulMessageSize, "unknown encoding");
	EmitInboundGenericDecodeFailureTelemetry(ulMessageSize);
}

void SnaccROSEBase::HandleInboundOuterDecodeFailure(unsigned long ulMessageSize, const char* szException, const char* szMethod, std::optional<int> errorCode)
{
	SJson::Value error;
	error["exception"] = szException;
	error["method"] = szMethod;
	if (errorCode)
		error["error"] = *errorCode;
	const std::string strError = getPrettyPrinted(error);
	PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());
	EmitInboundGenericDecodeFailureTelemetry(ulMessageSize);
}

/*
 * Searches in haystack for needle but searches not until the null byte in haystack but until reaching limit
 */
const char* strstr_limited(const char* haystack, const char* needle, size_t limit)
{
	try
	{
		size_t needle_length = strlen(needle);
		if (!needle_length)
			return haystack;

		for (size_t i = 0; i <= limit - needle_length; ++i)
			if (haystack[i] == *needle && strncmp(haystack + i, needle, needle_length) == 0)
				return (haystack + i);
	}
	catch (...)
	{
		ASSERT(0);
	}

	return NULL;
}

SnaccTelemetryData::Reason GetUnhandledReasonFromResult(const long lRoseResult)
{
	switch (lRoseResult)
	{
		case ROSE_REJECT_UNKNOWNOPERATION:
		case ROSE_REJECT_MISTYPEDARGUMENT:
		case ROSE_REJECT_FUNCTIONMISSING:
		case ROSE_REJECT_UNKNOWN:
		case ROSE_REJECT_ARGUMENT_MISSING:
			return SnaccTelemetryData::Reason::REJECT_PROTOCOL;
		case ROSE_REJECT_INVALIDSESSIONID:
		case ROSE_REJECT_STARTSSLREQUIRED:
			return SnaccTelemetryData::Reason::REJECT_SESSION;
		case ROSE_REJECT_AUTHENTICATIONINCOMPLETE:
		case ROSE_REJECT_AUTHENTICATIONFAILED:
		case ROSE_REJECT_AUTHENTICATION_SERVER_BUSY:
		case ROSE_REJECT_AUTHENTICATION_USER_TEMPORARY_LOCKED_OUT:
		case ROSE_REJECT_AUTHENTICATION_USER_LOCKED_OUT:
			return SnaccTelemetryData::Reason::REJECT_AUTHENTICATION;
		case ROSE_TE_TRANSPORTFAILED:
			return SnaccTelemetryData::Reason::SEND_FAILED;
		case ROSE_TE_ENCODE_FAILED:
			return SnaccTelemetryData::Reason::ENCODE_FAILED;
		case ROSE_TE_TIMEOUT:
			return SnaccTelemetryData::Reason::TIMEOUT;
		case ROSE_TE_SHUTDOWN:
			return SnaccTelemetryData::Reason::SHUTDOWN;
		case ROSE_RE_DECODE_FAILED:
			return SnaccTelemetryData::Reason::DECODE_FAILED;
		case ROSE_RE_INVALID_ANSWER:
			return SnaccTelemetryData::Reason::INVALID_RESPONSE;
		default:
			return SnaccTelemetryData::Reason::UNKNOWN_FAILURE;
	}
}

SnaccTelemetryData::Stage GetOutboundUnhandledStageFromResult(const long lRoseResult)
{
	switch (lRoseResult)
	{
		case ROSE_TE_TRANSPORTFAILED:
		case ROSE_TE_ENCODE_FAILED:
			return SnaccTelemetryData::Stage::OUTBOUND_SEND;
		case ROSE_TE_TIMEOUT:
		case ROSE_TE_SHUTDOWN:
		case ROSE_RE_INVALID_ANSWER:
		default:
			return SnaccTelemetryData::Stage::OUTBOUND_WAIT;
	}
}

// check if at least one Operation has been registerd
bool SnaccRoseOperationLookup::Initialized()
{
	return m_mapOpToID.empty() ? false : true;
}

void SnaccRoseOperationLookup::CleanUp()
{
	m_mapOpToID.clear();
	m_mapIDToOp.clear();
	m_mapIDToInterface.clear();
}

void SnaccRoseOperationLookup::RegisterOperation(unsigned int uiOpID, const char* szOpName, unsigned int uiInterfaceID)
{
#ifdef _DEBUG
	if (m_mapIDToOp.find(uiOpID) != m_mapIDToOp.end())
	{
		// In case we land here we have two operations using the same operationID
		ASSERT(0);
	}
#endif

	m_mapOpToID[szOpName] = uiOpID;
	m_mapIDToOp[uiOpID] = szOpName;
	m_mapIDToInterface[uiOpID] = uiInterfaceID;
}

unsigned int SnaccRoseOperationLookup::LookUpInterfaceID(unsigned int uiOpID)
{
	const auto it = m_mapIDToInterface.find(uiOpID);
	if (it != m_mapIDToInterface.end())
		return it->second;

	return 0;
}

const char* SnaccRoseOperationLookup::LookUpName(unsigned int uiOpID)
{
	const auto it = m_mapIDToOp.find(uiOpID);
	if (it != m_mapIDToOp.end())
		return it->second.c_str();

#ifdef _DEBUG
	// This may only happen if the other side calls a method we are not aware of
	// We handle it here as assert as it should not happen in development
	static std::mutex s_mutex; // prevents multiple threads to write to the log at the same time
	static std::set<int> s_unknownOpIDsAlreadyNotified;
	std::lock_guard<std::mutex> lock(s_mutex);
	if (!s_unknownOpIDsAlreadyNotified.contains(uiOpID))
	{
		s_unknownOpIDsAlreadyNotified.insert(uiOpID);
		fprintf(stderr, "An unknown operation with operationID %d was called.\n", uiOpID);
	}
#endif

	return nullptr;
}

unsigned int SnaccRoseOperationLookup::LookUpID(const char* szOpName)
{
	const auto it = m_mapOpToID.find(szOpName);
	if (it != m_mapOpToID.end())
		return it->second;

	return 0;
}

SnaccROSEPendingOperation::SnaccROSEPendingOperation(long lInvokeID, unsigned int uiOperationID, const char* szOperationName)
	: m_lInvokeID(lInvokeID),
	  m_uiOperationID(uiOperationID),
	  m_strOperationName(szOperationName ? szOperationName : ""),
	  m_lRoseResult(0),
	  m_stResponseData(0)
{
}

SnaccROSEPendingOperation::~SnaccROSEPendingOperation() = default;

bool SnaccROSEPendingOperation::WaitForComplete(long ulTimeOut /*= -1*/)
{
	return m_CompletedEvent.waitfor(ulTimeOut);
}

void SnaccROSEPendingOperation::CompleteOperation(long lRoseResult, std::unique_ptr<SNACC::ROSEMessage> pAnswerMessage, size_t stResponseData)
{
	m_lRoseResult = lRoseResult;
	m_stResponseData = stResponseData;
	if (pAnswerMessage)
		m_pAnswerMessage = std::move(pAnswerMessage);
	if (!m_bAsyncInvoke)
		m_CompletedEvent.signal();
}

bool SnaccROSEPendingOperation::TryClaimAsyncCompletion()
{
	bool expected = false;
	return m_bAsyncCompletionClaimed.compare_exchange_strong(expected, true);
}

void SnaccROSEPendingOperation::EnsureOutboundTelemetry()
{
	if (m_pTelemetry || m_chronoTelemetryCreated == std::chrono::steady_clock::time_point{})
		return;

	m_pTelemetry = SnaccTelemetryData::Create(SnaccTelemetryData::Direction::OUTBOUND, m_uiOperationID, m_strOperationName.c_str(), m_stOutboundRequestData, m_chronoTelemetryCreated);
}

void SnaccROSEPendingOperation::FinalizeTelemetry(long lFinalRoseResult, std::shared_ptr<SnaccInvokeContext> pctx)
{
	if (!m_pTelemetry)
		return;

	if (m_pAnswerMessage && lFinalRoseResult != m_lRoseResult)
	{
		m_pTelemetry->finalize(SnaccTelemetryData::Outcome::UNHANDLED, GetOutboundUnhandledStageFromResult(lFinalRoseResult), GetUnhandledReasonFromResult(lFinalRoseResult), lFinalRoseResult, m_stResponseData, std::move(pctx));
		return;
	}

	if (m_pAnswerMessage)
	{
		switch (m_pAnswerMessage->choiceId)
		{
			case ROSEMessage::resultCid:
				m_pTelemetry->finalize(SnaccTelemetryData::Outcome::RESULT, SnaccTelemetryData::Stage::OUTBOUND_WAIT, SnaccTelemetryData::Reason::REMOTE_RESULT, m_lRoseResult, m_stResponseData, pctx);
				break;
			case ROSEMessage::errorCid:
				m_pTelemetry->finalize(SnaccTelemetryData::Outcome::ERR, SnaccTelemetryData::Stage::OUTBOUND_WAIT, SnaccTelemetryData::Reason::REMOTE_ERROR, m_lRoseResult, m_stResponseData, pctx);
				break;
			case ROSEMessage::rejectCid:
				m_pTelemetry->finalize(SnaccTelemetryData::Outcome::REJECT, SnaccTelemetryData::Stage::OUTBOUND_WAIT, SnaccTelemetryData::Reason::REMOTE_REJECT, m_lRoseResult, m_stResponseData, pctx);
				break;
			default:
				m_pTelemetry->finalize(SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::OUTBOUND_WAIT, SnaccTelemetryData::Reason::INVALID_RESPONSE, m_lRoseResult, std::nullopt, pctx);
				break;
		}
		return;
	}

	if (m_lRoseResult != ROSE_NOERROR)
	{
		m_pTelemetry->finalize(SnaccTelemetryData::Outcome::UNHANDLED, GetOutboundUnhandledStageFromResult(m_lRoseResult), GetUnhandledReasonFromResult(m_lRoseResult), m_lRoseResult, std::nullopt, std::move(pctx));
		return;
	}

	m_pTelemetry->finalize(SnaccTelemetryData::Outcome::DISPATCHED, SnaccTelemetryData::Stage::OUTBOUND_WAIT, SnaccTelemetryData::Reason::WAIT_SKIPPED, m_lRoseResult, std::nullopt, std::move(pctx));
}

SnaccROSEBase::SnaccROSEBase(const wchar_t* szClassName)
	: m_strClassName(szClassName)
{
}

SnaccROSEBase::SnaccROSEBase(const wchar_t* szClassName, const std::set<int>& multithreadInvokeIDs)
	: m_strClassName(szClassName),
	  m_multithreadInvokeIDs(multithreadInvokeIDs)
{
}

void SnaccROSEBase::SetTelemetryCallback(SnaccTelemetryCallback* pTelemetryCallBack)
{
	m_pTelemetryCallback = pTelemetryCallBack;
}

SnaccROSEBase::~SnaccROSEBase(void)
{
	StopProcessing(true);
	StopWatchdogThread();
	ConfigureFileLogging(nullptr);
}

void SnaccROSEBase::StopProcessing(bool bStop /*= true*/)
{
	{
		std::lock_guard<std::mutex> guard(m_InternalProtectMutex);
		m_bProcessingAllowed = bStop ? false : true;
	}

	if (bStop)
		CompleteAllPendingOperations();
}

void SnaccROSEBase::SetMaxInvokeWaitTime(long lMaxInvokeWait)
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);
	m_lMaxInvokeWait = lMaxInvokeWait;
}

void SnaccROSEBase::CompleteAllPendingOperations()
{
	std::vector<long> asyncInvokeIds;
	{
		std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

		for (auto it = m_PendingOperations.begin(); it != m_PendingOperations.end(); ++it)
		{
			if (it->second->m_bAsyncInvoke)
				asyncInvokeIds.push_back(it->second->m_lInvokeID);
			else
				it->second->CompleteOperation(ROSE_TE_SHUTDOWN);
		}
	}

	for (const long invokeID : asyncInvokeIds)
		FinishAsyncInvoke(invokeID, ROSE_TE_SHUTDOWN, {}, 0);
}

bool SnaccROSEBase::IsProcessingAllowed() const
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);
	return m_bProcessingAllowed;
}

SnaccROSEPendingOperation& SnaccROSEBase::AddPendingOperation(int invokeID, unsigned int uiOperationID, const char* szOperationName)
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	auto pendingOperation = SnaccROSEPendingOperation::Create(invokeID, uiOperationID, szOperationName);
	auto result = m_PendingOperations.emplace(invokeID, std::move(pendingOperation));
	ASSERT(result.second);
	return *result.first->second;
}

void SnaccROSEBase::RemovePendingOperation(int invokeID)
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	const auto it = m_PendingOperations.find(invokeID);
	if (it != m_PendingOperations.end())
		m_PendingOperations.erase(it);
}

bool SnaccROSEBase::CompletePendingOperation(int invokeID, std::unique_ptr<SNACC::ROSEMessage> pMessage, unsigned long ulMessageSize)
{
	long lRoseResult = ROSE_RE_INVALID_ANSWER;
	bool finishAsync = false;
	{
		std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

		const auto it = m_PendingOperations.find(invokeID);
		if (it == m_PendingOperations.end())
			return false;

		switch (pMessage->choiceId)
		{
			case ROSEMessage::resultCid:
				lRoseResult = ROSE_NOERROR;
				break;
			case ROSEMessage::errorCid:
				lRoseResult = ROSE_ERROR_VALUE;
				break;
			case ROSEMessage::rejectCid:
				lRoseResult = GetRejectResultCode(pMessage->reject);
				break;
			default:
				break;
		}

		if (it->second->m_bAsyncInvoke)
			finishAsync = true;
		else
		{
			it->second->CompleteOperation(lRoseResult, std::move(pMessage), ulMessageSize);
			return true;
		}
	}

	if (finishAsync)
		FinishAsyncInvoke(invokeID, lRoseResult, std::move(pMessage), ulMessageSize);

	return true;
}

bool SnaccROSEBase::GetPendingOperationTelemetryInfo(int invokeID, unsigned int& uiOperationID, std::string& strOperationName)
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	const auto it = m_PendingOperations.find(invokeID);
	if (it == m_PendingOperations.end())
		return false;

	uiOperationID = it->second->m_uiOperationID;
	strOperationName = it->second->m_strOperationName;
	return true;
}

std::shared_ptr<SnaccInvokeContext> SnaccROSEBase::CreateInvokeContext(const SnaccInvokeContextInit& init)
{
	return SnaccInvokeContext::Create(init);
}

long SnaccROSEBase::GetRejectResultCode(const SNACC::ROSEReject* pReject)
{
	long lRoseResult = ROSE_REJECT_UNKNOWN;
	if (!pReject || !pReject->reject)
		return lRoseResult;

	if (pReject->reject->choiceId == RejectProblem::invokeProblemCid && pReject->reject->invokeProblem)
	{
		if (*pReject->reject->invokeProblem == InvokeProblem::unrecognisedOperation)
			lRoseResult = ROSE_REJECT_UNKNOWNOPERATION;
		else if (*pReject->reject->invokeProblem == InvokeProblem::mistypedArgument)
			lRoseResult = ROSE_REJECT_MISTYPEDARGUMENT;
		else if (*pReject->reject->invokeProblem == InvokeProblem::resourceLimitation)
			lRoseResult = ROSE_REJECT_FUNCTIONMISSING;
		else if (*pReject->reject->invokeProblem == InvokeProblem::authenticationIncomplete)
			lRoseResult = ROSE_REJECT_AUTHENTICATIONINCOMPLETE;
		else if (*pReject->reject->invokeProblem == InvokeProblem::authenticationFailed)
			lRoseResult = ROSE_REJECT_AUTHENTICATIONFAILED;
	}

	return lRoseResult;
}

long SnaccROSEBase::GetRejectResultCode(const SNACC::ROSEReject* pReject, SnaccInvokeContext& ctx)
{
	long lRoseResult = GetRejectResultCode(pReject);
	if (!pReject || !pReject->reject)
		return lRoseResult;

	if (pReject->reject->choiceId == RejectProblem::invokeProblemCid && pReject->reject->invokeProblem)
	{
		if (*pReject->reject->invokeProblem == InvokeProblem::authenticationIncomplete)
		{
			if (pReject->authentication)
				ctx.m_pRejectAuth = (SNACC::ROSEAuthResult*)pReject->authentication->Clone();
			ctx.m_lRejectResult = ROSE_REJECT_AUTHENTICATIONINCOMPLETE;
		}
		else if (*pReject->reject->invokeProblem == InvokeProblem::authenticationFailed)
		{
			if (pReject->authentication)
				ctx.m_pRejectAuth = (SNACC::ROSEAuthResult*)pReject->authentication->Clone();
			ctx.m_lRejectResult = ROSE_REJECT_AUTHENTICATIONFAILED;
		}
	}

	return lRoseResult;
}

bool SnaccROSEBase::OnBinaryDataBlockResult(const char* lpBytes, unsigned long lSize, bool bLogTransportData /*= true */)
{
	if (!lSize)
		return false;

	bool bReturn = false;
	try
	{
		switch (m_eTransportEncoding)
		{
			case SNACC::TransportEncoding::BER:
				{
					AsnBuf buffer((const char*)lpBytes, lSize);
					auto pMessage = std::make_unique<ROSEMessage>();
					AsnLen bytesDecoded = 0;
					std::optional<InboundInvokeRejectContext> rejectCtx;
					try
					{
						pMessage->BDec(buffer, bytesDecoded);
						rejectCtx = InboundInvokeRejectContext::TryFrom(*pMessage);
						if (bLogTransportData)
							LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

						bReturn = OnROSEMessage(std::move(pMessage), false, lSize);
					}
					catch (const SnaccException& ex)
					{
						HandleInboundEnvelopeSnaccDecodeFailure(m_eTransportEncoding, SNACC::TransportEncoding::BER, lpBytes, lSize, bLogTransportData, ex, rejectCtx, false, false, __FUNCTION__);
						return true;
					}
					break;
				}
			case SNACC::TransportEncoding::JSON:
			case SNACC::TransportEncoding::JSON_NO_HEADING:
				{
					int iHeaderLen = GetJsonHeaderLen(lpBytes, lSize);
					SJson::Value value;
					SJson::Reader reader;
					if (reader.parse((const char*)lpBytes + iHeaderLen, (const char*)lpBytes + lSize, value))
					{
						auto pMessage = std::make_unique<ROSEMessage>();
						std::optional<InboundInvokeRejectContext> rejectCtx;
						try
						{
							if (!pMessage->JDec(value))
								throw InvalidTagException("ROSEMessage", "decode failed: ROSEMessage", STACK_ENTRY);

							rejectCtx = InboundInvokeRejectContext::TryFrom(*pMessage);
							if (bLogTransportData)
								LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, pMessage.get(), &value);

							bReturn = OnROSEMessage(std::move(pMessage), false, lSize);
						}
						catch (const SnaccException& ex)
						{
							HandleInboundEnvelopeSnaccDecodeFailure(m_eTransportEncoding, m_eTransportEncoding, lpBytes, lSize, bLogTransportData, ex, rejectCtx, false, true, __FUNCTION__);
							return true;
						}
					}
					else
					{
						HandleInboundJsonParseDecodeFailure(m_eTransportEncoding, lpBytes, lSize, bLogTransportData, reader.getFormattedErrorMessages(), __FUNCTION__);
					}
					break;
				}
			default:
				{
					HandleInboundUnknownEncodingDecodeFailure(lpBytes, lSize, bLogTransportData);
					break;
				}
		}
	}
	catch (const SnaccException& ex)
	{
		HandleInboundOuterDecodeFailure(lSize, ex.what(), __FUNCTION__, static_cast<int>(ex.m_errorCode));
	}
	catch (const std::exception& ex)
	{
		HandleInboundOuterDecodeFailure(lSize, ex.what(), __FUNCTION__);
	}
	catch (...)
	{
		HandleInboundOuterDecodeFailure(lSize, "...", __FUNCTION__);
	}
	return bReturn;
}

int SnaccROSEBase::GetJsonHeaderLen(const char* lpBytes, unsigned long iLength)
{
	unsigned int iLen = 0;
	if (lpBytes[0] == 'J')
	{
		iLen++;
		for (unsigned int i = 1; i < 10; i++)
			if ((iLength >= iLen) && lpBytes[i] >= '0' && lpBytes[i] <= '9')
				iLen++;
			else
				break;
	}
	return iLen;
}

std::string SnaccROSEBase::GetEncoded(const SNACC::TransportEncoding encoding, const AsnType* pValue, unsigned long* pUlSize /* = nullptr */)
{
	std::string strData;
	if (pUlSize)
		*pUlSize = 0;
	switch (encoding)
	{
		case SNACC::TransportEncoding::BER:
			{
				AsnBuf OutBuf;
				AsnLen encodedBytes = pValue->BEnc(OutBuf);
				OutBuf.ResetMode();
				OutBuf.GetSeg(strData, encodedBytes);
				if (pUlSize)
					*pUlSize = encodedBytes;
			}
			break;
		case SNACC::TransportEncoding::JSON_NO_HEADING:
		case SNACC::TransportEncoding::JSON:
			{
				auto value = pValue->JEnc();
				int logLevel = (int)GetLogLevel(true);
				if (logLevel & (int)EAsnLogLevel::JSON || logLevel & (int)EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED)
					strData = getPrettyPrinted(value);
				else
				{
					SJson::FastWriter writer;
					strData = writer.write(value);
				}
			}
			break;
		default:
			ASSERT(false);
			throw std::runtime_error("invalid encoding");
			break;
	}
	return strData;
}

SNACC::TransportEncoding SnaccROSEBase::DetectEncoding(const char* lpBytes, unsigned long ulSize) const
{
	if (!ulSize)
		return SNACC::TransportEncoding::UNKNOWN;

	unsigned char byFirst = (unsigned char)*lpBytes;
	if (byFirst == 0xA1 || byFirst == 0xA2 || byFirst == 0xA3 || byFirst == 0xA4)
		return SNACC::TransportEncoding::BER;
	else if (byFirst == 'J')
		return SNACC::TransportEncoding::JSON;
	else if (byFirst == 'J' || byFirst == '{' || byFirst == '[')
		return SNACC::TransportEncoding::JSON_NO_HEADING;
	else
	{
		ASSERT(false);
		return SNACC::TransportEncoding::UNKNOWN;
	}
}

void SnaccROSEBase::OnBinaryDataBlock(const char* lpBytes, unsigned long ulSize, bool bLogTransportData /*= true*/)
{
	if (!ulSize)
		return;

	if (m_eTransportEncoding == SNACC::TransportEncoding::UNKNOWN)
		m_eTransportEncoding = DetectEncoding(lpBytes, ulSize);

	try
	{
		switch (m_eTransportEncoding)
		{
			case SNACC::TransportEncoding::BER:
				{
					AsnBuf buffer((const char*)lpBytes, ulSize);
					auto pMessage = std::make_unique<ROSEMessage>();
					AsnLen bytesDecoded = 0;
					std::optional<InboundInvokeRejectContext> rejectCtx;
					try
					{
						pMessage->BDec(buffer, bytesDecoded);
						rejectCtx = InboundInvokeRejectContext::TryFrom(*pMessage);
						if (bLogTransportData)
							LogTransportData(false, SNACC::TransportEncoding::BER, nullptr, lpBytes, ulSize, nullptr, nullptr);

						OnROSEMessage(std::move(pMessage), true, ulSize);
					}
					catch (const SnaccException& ex)
					{
						HandleInboundEnvelopeSnaccDecodeFailure(m_eTransportEncoding, SNACC::TransportEncoding::BER, lpBytes, ulSize, bLogTransportData, ex, rejectCtx, true, false, __FUNCTION__);
					}
					break;
				}
			case SNACC::TransportEncoding::JSON:
			case SNACC::TransportEncoding::JSON_NO_HEADING:
				{
					int iHeaderLen = GetJsonHeaderLen(lpBytes, ulSize);

					SJson::Value value;
					SJson::Reader reader;
					if (reader.parse((const char*)lpBytes + iHeaderLen, (const char*)lpBytes + ulSize, value))
					{
						auto pMessage = std::make_unique<ROSEMessage>();
						std::optional<InboundInvokeRejectContext> rejectCtx;
						try
						{
							if (!pMessage->JDec(value))
								throw InvalidTagException("ROSEMessage", "decode failed: ROSEMessage", STACK_ENTRY);
							rejectCtx = InboundInvokeRejectContext::TryFrom(*pMessage);
							if (bLogTransportData)
								LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, ulSize, pMessage.get(), &value);
							OnROSEMessage(std::move(pMessage), true, ulSize);
						}
						catch (const SnaccException& ex)
						{
							HandleInboundEnvelopeSnaccDecodeFailure(m_eTransportEncoding, m_eTransportEncoding, lpBytes, ulSize, bLogTransportData, ex, rejectCtx, true, true, __FUNCTION__);
						}
					}
					else
					{
						HandleInboundJsonParseDecodeFailure(m_eTransportEncoding, lpBytes, ulSize, bLogTransportData, reader.getFormattedErrorMessages(), __FUNCTION__);
					}
					break;
				}
			default:
				{
					// if we don't know the encoding, we need to log it binary to ensure proper readability in the logs (ensure payload is hex converted)
					HandleInboundUnknownEncodingDecodeFailure(lpBytes, ulSize, bLogTransportData);
					break;
				}
		}
	}
	catch (const SnaccException& ex)
	{
		HandleInboundOuterDecodeFailure(ulSize, ex.what(), __FUNCTION__, static_cast<int>(ex.m_errorCode));
	}
	catch (const std::exception& ex)
	{
		HandleInboundOuterDecodeFailure(ulSize, ex.what(), __FUNCTION__);
	}
	catch (...)
	{
		HandleInboundOuterDecodeFailure(ulSize, "...", __FUNCTION__);
	}
}

bool SnaccROSEBase::OnROSEMessage(std::unique_ptr<SNACC::ROSEMessage> pMessage, bool bAllowAllInvokes, unsigned long ulMessageSize)
{
	bool bProcessed = false;
	switch (pMessage->choiceId)
	{
		case ROSEMessage::invokeCid:
			{
				auto& invoke = *pMessage->invoke;
				if (bAllowAllInvokes || m_multithreadInvokeIDs.find(invoke.operationID) != m_multithreadInvokeIDs.end())
				{
					if (invoke.operationName && invoke.operationID == 0)
						invoke.operationID = SnaccRoseOperationLookup::LookUpID(invoke.operationName->getASCII().c_str());

					if (invoke.operationID || invoke.operationName)
						OnInvokeMessage(std::move(pMessage), ulMessageSize);

					bProcessed = true;
				}
				break;
			}
		case ROSEMessage::resultCid:
			{
				const int invokeID = pMessage->result->invokeID;
				OnResultMessage(*pMessage->result, ulMessageSize);
				CompletePendingOperation(invokeID, std::move(pMessage), ulMessageSize);
				return true;
			}
		case ROSEMessage::errorCid:
			{
				const int invokeID = pMessage->error->invokedID;
				OnErrorMessage(*pMessage->error, ulMessageSize);
				CompletePendingOperation(invokeID, std::move(pMessage), ulMessageSize);
				return true;
			}
		case ROSEMessage::rejectCid:
			{
				auto& reject = *pMessage->reject;
				OnRejectMessage(reject, ulMessageSize);
				if (reject.invokedID.choiceId == ROSERejectChoice::invokedIDCid && reject.invokedID.invokedID != nullptr)
				{
					const int invokeID = *reject.invokedID.invokedID;
					CompletePendingOperation(invokeID, std::move(pMessage), ulMessageSize);
					return true;
				}
				bProcessed = true;
				break;
			}
		default:
			break;
	}
	return bProcessed;
}

long SnaccROSEBase::EncodeReject(SNACC::ROSEReject* pReject, std::string& strResponse)
{
	long lRoseResult = ROSE_NOERROR;

	ROSEMessage rejectMsg;
	const RoseEncodeRejectBorrow rejectBorrow(rejectMsg, *pReject);

	// encode now.
	if (m_eTransportEncoding == SNACC::TransportEncoding::BER)
	{
		AsnBuf OutBuf;
		AsnLen BytesEncoded = rejectMsg.BEnc(OutBuf);

		OutBuf.ResetMode();
		OutBuf.GetSeg(strResponse, BytesEncoded);

		LogTransportData(true, m_eTransportEncoding, nullptr, strResponse.c_str(), strResponse.length(), &rejectMsg, nullptr);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		auto value = rejectMsg.JEnc();

		int logLevel = (int)GetLogLevel(true);
		if (logLevel & (int)EAsnLogLevel::JSON || logLevel & (int)EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED)
			strResponse = getPrettyPrinted(value);
		else
		{
			SJson::FastWriter writer;
			strResponse = writer.write(value);
		}

		std::string strPrefix;
		if (m_eTransportEncoding == SNACC::TransportEncoding::JSON)
			lRoseResult = GetJsonLengthPrefix(strResponse, strPrefix);
		if (lRoseResult == ROSE_NOERROR)
		{
			LogTransportData(true, m_eTransportEncoding, nullptr, strResponse.c_str(), strResponse.length(), &rejectMsg, nullptr);
			if (!strPrefix.empty())
				strResponse.insert(0, strPrefix);
		}
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	return lRoseResult;
}

long SnaccROSEBase::SendRejectEx(SNACC::ROSEReject* pReject, SnaccInvokeContext& ctx)
{
	std::string strResponse;
	auto lResult = EncodeReject(pReject, strResponse);
	if (lResult)
		return lResult;

	return SendBinaryDataBlockEx(strResponse.c_str(), strResponse.length(), ctx);
}

long SnaccROSEBase::GetJsonLengthPrefix(std::string_view strJson, std::string& strLenghtPrefix) const
{
	const auto len = strJson.length();
	if (len > 9999999)
		return ROSE_TE_ENCODE_FAILED;

	// Calc prefix e.g. (J1234567)
	char szPrefix[10] = {0};
#if _WIN32
#if _MSC_VER < 1900
	sprintf_s(szPrefix, 10, "J%07d", strJson.length());
#else
	sprintf_s(szPrefix, 10, "J%07zd", strJson.length());
#endif
#else
	snprintf(szPrefix, 10, "J%07zd", strJson.length());
#endif

	strLenghtPrefix = std::string(szPrefix);

	return ROSE_NOERROR;
}

long SnaccROSEBase::EncodeRejectInvoke(unsigned int uiInvokeID, SNACC::InvokeProblem problem, std::string& strResponse, const char* szError /*= nullptr*/, const wchar_t* szSessionID /*= nullptr*/, SNACC::ROSEAuthResult* pAuthHeader /*=0*/)
{
	if (uiInvokeID == 99999)
	{
		// Events do not get an answer
		return 0;
	}
	ROSEReject reject;
	reject.invokedID.choiceId = ROSERejectChoice::invokedIDCid;
	reject.invokedID.invokedID = new AsnInt;
	*reject.invokedID.invokedID = uiInvokeID;
	if (szSessionID)
		reject.sessionID = new UTF8String(szSessionID);
	reject.reject = new RejectProblem;
	reject.reject->choiceId = RejectProblem::invokeProblemCid;
	reject.reject->invokeProblem = new InvokeProblem;
	*reject.reject->invokeProblem = problem;

	if (pAuthHeader)
		reject.authentication = (SNACC::ROSEAuthResult*)pAuthHeader->Clone();

	if ((m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING) && szError)
		reject.details = UTF8String::CreateNewFromUTF8(szError);

	return EncodeReject(&reject, strResponse);
}

long SnaccROSEBase::EncodeInvokeRejectResponse(const SNACC::ROSEInvoke* pInvoke, long lProtocolResult, SnaccInvokeContext& ctx, std::string& strResponse)
{
	const wchar_t* szSessionID = pInvoke->sessionID ? pInvoke->sessionID->c_str() : nullptr;
	long lEncodeResult = ROSE_NOERROR;

	if (lProtocolResult == ROSE_REJECT_UNKNOWNOPERATION)
		lEncodeResult = EncodeRejectInvoke(pInvoke->invokeID, InvokeProblem::unrecognisedOperation, strResponse, "unrecognisedOperation", szSessionID);
	else if (lProtocolResult == ROSE_REJECT_MISTYPEDARGUMENT)
		lEncodeResult = EncodeRejectInvoke(pInvoke->invokeID, InvokeProblem::mistypedArgument, strResponse, "mistypedArgument", szSessionID);
	else if (lProtocolResult == ROSE_REJECT_FUNCTIONMISSING)
		lEncodeResult = EncodeRejectInvoke(pInvoke->invokeID, InvokeProblem::resourceLimitation, strResponse, "functionMissing", szSessionID);
	else if (lProtocolResult == ROSE_REJECT_INVALIDSESSIONID)
		lEncodeResult = EncodeRejectInvoke(pInvoke->invokeID, InvokeProblem::invalidSessionID, strResponse, "invalidSessionID", szSessionID);
	else if (lProtocolResult == ROSE_REJECT_AUTHENTICATIONFAILED)
		lEncodeResult = EncodeRejectInvoke(pInvoke->invokeID, InvokeProblem::authenticationFailed, strResponse, "authenticationFailed", szSessionID, ctx.m_pRejectAuth);
	else if (lProtocolResult == ROSE_REJECT_AUTHENTICATIONINCOMPLETE)
		lEncodeResult = EncodeRejectInvoke(pInvoke->invokeID, InvokeProblem::authenticationIncomplete, strResponse, "authenticationIncomplete", szSessionID, ctx.m_pRejectAuth);
	else if (lProtocolResult == ROSE_REJECT_AUTHENTICATION_USER_TEMPORARY_LOCKED_OUT)
		lEncodeResult = EncodeRejectInvoke(pInvoke->invokeID, InvokeProblem::authenticationFailed, strResponse, "temporaryLockedOut", szSessionID, ctx.m_pRejectAuth);
	else if (lProtocolResult == ROSE_REJECT_AUTHENTICATION_USER_LOCKED_OUT)
		lEncodeResult = EncodeRejectInvoke(pInvoke->invokeID, InvokeProblem::authenticationFailed, strResponse, "lockedOut", szSessionID, ctx.m_pRejectAuth);
	else if (lProtocolResult == ROSE_REJECT_AUTHENTICATION_SERVER_BUSY)
		lEncodeResult = EncodeRejectInvoke(pInvoke->invokeID, InvokeProblem::authenticationFailed, strResponse, "serverBusy", szSessionID, ctx.m_pRejectAuth);
	else if (lProtocolResult == ROSE_REJECT_ARGUMENT_MISSING)
		lEncodeResult = EncodeRejectInvoke(pInvoke->invokeID, InvokeProblem::mistypedArgument, strResponse, "argumentMissing", szSessionID);
	else if (lProtocolResult == ROSE_TE_ENCODE_FAILED)
		lEncodeResult = EncodeRejectInvoke(pInvoke->invokeID, InvokeProblem::resourceLimitation, strResponse, "responseIsTooBig", szSessionID);
	else
	{
		std::stringstream ss;
		ss << "otherError: 0x" << std::hex << lProtocolResult;
		const std::string strOtherError = ss.str();
		lEncodeResult = EncodeRejectInvoke(pInvoke->invokeID, InvokeProblem::unrecognisedOperation, strResponse, strOtherError.c_str(), szSessionID);
	}

	return lEncodeResult == ROSE_NOERROR ? lProtocolResult : lEncodeResult;
}

void SnaccROSEBase::OnInvokeMessage(std::unique_ptr<SNACC::ROSEMessage> pMessage, unsigned long ulMessageSize)
{
	std::string strResponse;
	long lResult = ROSE_REJECT_UNKNOWNOPERATION;
	auto& invoke = *pMessage->invoke;
	const char* szOperationName = SnaccRoseOperationLookup::LookUpName(invoke.operationID);

	SnaccInvokeContextInit init(SnaccInvokeDirection::INBOUND, &invoke, szOperationName);
	auto pCtx = CreateInvokeContext(init);
	auto telemetry = SnaccTelemetryData::Create(SnaccTelemetryData::Direction::INBOUND, invoke.operationID, szOperationName, ulMessageSize);
	auto telemetryResult = SnaccTelemetryData::Outcome::UNHANDLED;
	auto telemetryReason = SnaccTelemetryData::Reason::UNKNOWN_FAILURE;
	bool bInvokeException = false;

	if (!IsProcessingAllowed())
		lResult = ROSE_TE_SHUTDOWN;
	else
	{
		try
		{
			lResult = OnInvoke(pMessage, *pCtx, strResponse);
		}
		catch (const SnaccException& ex)
		{
			// decode of invoke detail failed.
			lResult = ROSE_REJECT_MISTYPEDARGUMENT;
			bInvokeException = true;
			SJson::Value error;
			error["exception"] = ex.what();
			error["method"] = __FUNCTION__;
			error["error"] = (int)ex.m_errorCode;
			error["invokeID"] = (int)invoke.invokeID;
			std::string jsonString = getPrettyPrinted(error);
			PrintJSONToLog(false, true, nullptr, jsonString.c_str(), jsonString.length());
		}
	}

	const bool bIsInvoke = (AsnIntType)invoke.invokeID != 99999;
	const bool bIsRejectResponse = lResult != ROSE_NOERROR && bIsInvoke;

	// if the Result is ROSE_NOERROR the request has been processed with a result or an error (the invoke context points out if it was replied with an error)
	// if the result is ROSE_REJECT_ASYNCOPERATION, the result will be sent async
	if (bIsRejectResponse)
	{
		lResult = EncodeInvokeRejectResponse(&invoke, lResult, *pCtx, strResponse);
		if (lResult != ROSE_NOERROR)
			telemetryReason = GetUnhandledReasonFromResult(lResult);
	}

	if (!strResponse.empty())
	{
		const long lSendResult = SendBinaryDataBlockEx(strResponse.c_str(), strResponse.length(), *pCtx);
		if (lSendResult != ROSE_NOERROR)
		{
			lResult = lSendResult;
			telemetryReason = GetUnhandledReasonFromResult(lSendResult);
		}
	}

	if (lResult == 0)
	{
		// No error while processing
		if (bIsRejectResponse)
		{
			telemetryResult = SnaccTelemetryData::Outcome::REJECT;
			telemetryReason = bInvokeException ? SnaccTelemetryData::Reason::INVOKE_EXCEPTION : SnaccTelemetryData::Reason::LOCAL_REJECT;
		}
		else if (bIsInvoke)
		{
			// It is an invoke so we need to check if we have a result or an error as answer
			if (pCtx->m_bResponseIsError)
			{
				telemetryResult = SnaccTelemetryData::Outcome::ERR;
				telemetryReason = SnaccTelemetryData::Reason::LOCAL_ERROR;
			}
			else
			{
				telemetryResult = SnaccTelemetryData::Outcome::RESULT;
				telemetryReason = SnaccTelemetryData::Reason::LOCAL_RESULT;
			}
		}
		else
		{
			telemetryResult = SnaccTelemetryData::Outcome::EVENT;
			telemetryReason = SnaccTelemetryData::Reason::LOCAL_EVENT;
		}
	}
	else if (telemetryReason == SnaccTelemetryData::Reason::UNKNOWN_FAILURE)
		telemetryReason = GetUnhandledReasonFromResult(lResult);

	telemetry->finalize(telemetryResult, SnaccTelemetryData::Stage::INBOUND_INVOKE, telemetryReason, lResult, strResponse.empty() ? std::nullopt : std::optional(strResponse.length()), std::move(pCtx));
	OnInvokeProcessed(telemetry);
}

void SnaccROSEBase::OnResultMessage(const SNACC::ROSEResult& result, unsigned long ulMessageSize)
{
	unsigned int uiOperationID = 0;
	std::string strOperationName;
	GetPendingOperationTelemetryInfo(result.invokeID, uiOperationID, strOperationName);
	OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, uiOperationID, strOperationName.c_str(), ulMessageSize, SnaccTelemetryData::Outcome::RESULT, SnaccTelemetryData::Stage::INBOUND_RESPONSE, SnaccTelemetryData::Reason::REMOTE_RESULT, ROSE_NOERROR));
}

void SnaccROSEBase::OnErrorMessage(const SNACC::ROSEError& error, unsigned long ulMessageSize)
{
	unsigned int uiOperationID = 0;
	std::string strOperationName;
	GetPendingOperationTelemetryInfo(error.invokedID, uiOperationID, strOperationName);
	OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, uiOperationID, strOperationName.c_str(), ulMessageSize, SnaccTelemetryData::Outcome::ERR, SnaccTelemetryData::Stage::INBOUND_RESPONSE, SnaccTelemetryData::Reason::REMOTE_ERROR, ROSE_ERROR_VALUE));
}

void SnaccROSEBase::OnRejectMessage(const SNACC::ROSEReject& reject, unsigned long ulMessageSize)
{
	unsigned int uiOperationID = 0;
	std::string strOperationName;
	if (reject.invokedID.choiceId == ROSERejectChoice::invokedIDCid && reject.invokedID.invokedID != nullptr)
		GetPendingOperationTelemetryInfo(*reject.invokedID.invokedID, uiOperationID, strOperationName);
	OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, uiOperationID, strOperationName.c_str(), ulMessageSize, SnaccTelemetryData::Outcome::REJECT, SnaccTelemetryData::Stage::INBOUND_RESPONSE, SnaccTelemetryData::Reason::REMOTE_REJECT, GetRejectResultCode(&reject)));
}

long SnaccROSEBase::GetNextInvokeID()
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	m_lInvokeCounter++;
	if (m_lInvokeCounter >= 99998)
		m_lInvokeCounter = 1;

	return m_lInvokeCounter;
}

void SnaccROSEBase::UpdatePendingOutboundRequestData(long invokeID, size_t stRequestData)
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	const auto it = m_PendingOperations.find(invokeID);
	if (it != m_PendingOperations.end())
		it->second->m_stOutboundRequestData = stRequestData;
}

long SnaccROSEBase::Send(SNACC::ROSEInvoke* pInvoke, const char* szOperationName, SnaccInvokeContext& ctx, size_t* pstRequestData /*= nullptr*/)
{
	long lRoseResult = ROSE_NOERROR;
	if (pstRequestData)
		*pstRequestData = 0;

	ROSEMessage invokeMsg;
	const ScopedEncodeInvokeBorrow invokeBorrow(invokeMsg, *pInvoke);

	if (m_eTransportEncoding == SNACC::TransportEncoding::BER)
	{
		unsigned long ulSize;
		std::string strData = GetEncoded(m_eTransportEncoding, &invokeMsg, &ulSize);
		if (pstRequestData)
			*pstRequestData = ulSize;
		UpdatePendingOutboundRequestData(pInvoke->invokeID, ulSize);
		LogTransportData(true, m_eTransportEncoding, szOperationName, strData.c_str(), ulSize, &invokeMsg, nullptr);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), ulSize, ctx);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		const ScopedInvokeOperationName operationName(*pInvoke, szOperationName);

		std::string strData = GetEncoded(m_eTransportEncoding, &invokeMsg);

		std::string strPrefix;
		if (m_eTransportEncoding == SNACC::TransportEncoding::JSON)
			lRoseResult = GetJsonLengthPrefix(strData, strPrefix);
		if (lRoseResult == ROSE_NOERROR)
		{
			LogTransportData(true, m_eTransportEncoding, nullptr, strData.c_str(), strData.length(), &invokeMsg, nullptr);
			if (!strPrefix.empty())
				strData.insert(0, strPrefix);
			if (pstRequestData)
				*pstRequestData = strData.length();
			UpdatePendingOutboundRequestData(pInvoke->invokeID, strData.length());
			lRoseResult = SendBinaryDataBlockEx(strData.c_str(), strData.length(), ctx);
		}
		else if (pstRequestData)
			*pstRequestData = strData.length();
	}
	else
	{
		// You need to specify the transport encoding when the connection is setup
		// The connection is no longer defaulting to BER. This needs to be set explicit.
		// Inbound connections adopt the encoding as soon as the first payload is received
		ASSERT(0);
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	return lRoseResult;
}

long SnaccROSEBase::SendEvent(SNACC::ROSEInvoke* pInvoke, const char* szOperationName, std::shared_ptr<SnaccInvokeContext> pCtx /*= {}*/)
{
	const auto chronoCreated = std::chrono::steady_clock::now();
	if (!pCtx)
		pCtx = CreateInvokeContext(SnaccInvokeContextInit(SnaccInvokeDirection::OUTBOUND, pInvoke, szOperationName));
	auto& ctx = *pCtx;

	size_t stRequestData = 0;
	const long lRoseResult = IsProcessingAllowed() ? Send(pInvoke, szOperationName, ctx, &stRequestData) : ROSE_TE_SHUTDOWN;
	auto telemetry = SnaccTelemetryData::Create(SnaccTelemetryData::Direction::OUTBOUND, pInvoke->operationID, szOperationName, stRequestData, chronoCreated);
	telemetry->finalize(lRoseResult == ROSE_NOERROR ? SnaccTelemetryData::Outcome::EVENT : SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::OUTBOUND_SEND, lRoseResult == ROSE_NOERROR ? SnaccTelemetryData::Reason::LOCAL_EVENT : GetUnhandledReasonFromResult(lRoseResult), lRoseResult, std::nullopt, pCtx);
	OnInvokeProcessed(telemetry);
	return lRoseResult;
}

bool SnaccROSEBase::LogTransportData(const bool bOutbound, const SNACC::TransportEncoding encoding, const char* szOperationName, const char* szData, const size_t size, const SNACC::ROSEMessage* pMsg, const SJson::Value* pParsedValue)
{
	bool bTransportDataWasLogged = false;

	int level = (int)GetLogLevel(bOutbound);
	if (level != (int)EAsnLogLevel::DISABLED)
	{
		// We need to log something...
		bool bLogJSON = level & ((int)EAsnLogLevel::JSON | (int)EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED);
		bool bLogBER = level & (int)EAsnLogLevel::BER;
		if (bLogJSON)
		{
			// We need to retrieve the operationName for an inbound invoke.
			std::string strOperationName;
			if (!szOperationName && pMsg && pMsg->choiceId == ROSEMessage::invokeCid)
			{
				if (pMsg->invoke->operationName)
				{
					strOperationName = pMsg->invoke->operationName->getUTF8();
					if (!strOperationName.empty())
						szOperationName = strOperationName.c_str();
				}
				if (!szOperationName)
					szOperationName = SnaccRoseOperationLookup::LookUpName(pMsg->invoke->operationID);
			}

			// JSON logging is requested
			if (encoding == SNACC::TransportEncoding::JSON || encoding == SNACC::TransportEncoding::JSON_NO_HEADING)
			{
				if (size)
				{
					// The payload is already json -> we can directly log it
					// in Case inbound data shall get pretty printed always we need to check if the payload is alrady pretty printed
					if (pParsedValue && (level & (int)EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED) && strstr_limited(szData, "\n", size) == nullptr)
					{
						std::string strLogData = getPrettyPrinted(*pParsedValue);
						bTransportDataWasLogged = PrintJSONToLog(bOutbound, false, szOperationName, strLogData.c_str(), strLogData.length());
					}
					else
					{
						bTransportDataWasLogged = PrintJSONToLog(bOutbound, false, szOperationName, szData, size);
					}
				}
			}
			else if (pMsg)
			{
				// The strTransportData is not already JSON encoded...
				// So we encode the ROSEMessage as JSON
				auto value = pMsg->JEnc();
				std::string strLogData = getPrettyPrinted(value);
				bTransportDataWasLogged = PrintJSONToLog(bOutbound, false, szOperationName, strLogData.c_str(), strLogData.length());
			}
		}
		if (bLogBER && szData)
		{
			// BER logging is requested
			// Only makes sense if the payload is BER
			if (encoding == SNACC::TransportEncoding::BER)
			{
				bool bFirst = true;
				std::string strLogData;
				if (bLogJSON)
					strLogData += "\"BER\" : \"";
				strLogData.reserve(size * 3);
				const char* szCur = szData;
				for (size_t pos = 0; pos < size; pos++)
				{
					if (bFirst)
						bFirst = false;
					else
						strLogData += " ";
					strLogData += "0123456789ABCDEF"[(*szCur >> 4) & 0xF];
					strLogData += "0123456789ABCDEF"[(*szCur) & 0xF];
					szCur++;
				}
				if (bLogJSON)
					strLogData += "\"";
				bTransportDataWasLogged = PrintJSONToLog(bOutbound, false, nullptr, strLogData.c_str(), strLogData.length());
			}
		}
	}

	return bTransportDataWasLogged;
}

long SnaccROSEBase::SendInvoke(SNACC::ROSEInvoke* pInvoke, SNACC::AsnType* pResult, SNACC::AsnType* pError, const char* szOperationName /*= nullptr*/, int iTimeout /*= -1*/, std::shared_ptr<SnaccInvokeContext> pCtx /*= {}*/)
{
	const auto chronoCreated = std::chrono::steady_clock::now();
	const char* szResolvedOperationName = SnaccRoseOperationLookup::LookUpName(pInvoke->operationID);

	// Ensure that we always have a ctx
	if (!pCtx)
	{
		SnaccInvokeContextInit init(SnaccInvokeDirection::OUTBOUND, pInvoke, szResolvedOperationName ? szResolvedOperationName : szOperationName);
		pCtx = CreateInvokeContext(init);
	}
	auto& ctx = *pCtx;

	if (!IsProcessingAllowed())
	{
		auto telemetry = SnaccTelemetryData::Create(SnaccTelemetryData::Direction::OUTBOUND, pInvoke->operationID, szResolvedOperationName, 0, chronoCreated);
		telemetry->finalize(SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::OUTBOUND_SEND, SnaccTelemetryData::Reason::SHUTDOWN, ROSE_TE_SHUTDOWN, std::nullopt, std::move(pCtx));
		OnInvokeProcessed(telemetry);
		return ROSE_TE_SHUTDOWN;
	}

	auto& pendingOP = AddPendingOperation(pInvoke->invokeID, pInvoke->operationID, szResolvedOperationName);

	size_t stRequestData = 0;
	long lRoseResult = Send(pInvoke, szOperationName, ctx, &stRequestData);
	pendingOP.m_pTelemetry = SnaccTelemetryData::Create(SnaccTelemetryData::Direction::OUTBOUND, pInvoke->operationID, szResolvedOperationName, stRequestData, chronoCreated);

	if (lRoseResult == 0)
	{
		// Wait for Answer...
		if (iTimeout == -1)
			iTimeout = m_lMaxInvokeWait;

		if (iTimeout)
		{
			if (pendingOP.WaitForComplete(iTimeout))
			{
				const bool bHasResponseMessage = pendingOP.m_pAnswerMessage != nullptr;
				if (bHasResponseMessage)
					lRoseResult = ROSE_NOERROR;
				else
					lRoseResult = pendingOP.m_lRoseResult;
			}
			else
			{
				// not completed
				lRoseResult = ROSE_TE_TIMEOUT;
				pendingOP.m_lRoseResult = lRoseResult;
			}
		}
		else
		{
			lRoseResult = ROSE_NOERROR;
			pendingOP.m_lRoseResult = lRoseResult;
		}
	}
	else
		pendingOP.m_lRoseResult = lRoseResult;

	if (pendingOP.m_pAnswerMessage)
		lRoseResult = HandleInvokeResult(lRoseResult, *pendingOP.m_pAnswerMessage, pResult, pError, ctx);

	pendingOP.FinalizeTelemetry(lRoseResult, pCtx);
	OnInvokeProcessed(pendingOP.m_pTelemetry);

	RemovePendingOperation(pendingOP.m_lInvokeID);

	return lRoseResult;
}

void SnaccROSEBase::FinishAsyncInvoke(long invokeID, long lRoseResult, std::unique_ptr<SNACC::ROSEMessage> pMessage, size_t stResponseData)
{
	std::unique_ptr<SnaccROSEPendingOperation> pending;
	{
		std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

		const auto it = m_PendingOperations.find(invokeID);
		if (it == m_PendingOperations.end() || !it->second->m_bAsyncInvoke)
			return;

		if (!it->second->TryClaimAsyncCompletion())
			return;

		if (it->second->m_asyncDeadline.has_value())
		{
			if (m_asyncDeadlineCount > 0)
				--m_asyncDeadlineCount;
		}

		pending = std::move(it->second);
		m_PendingOperations.erase(it);
	}

	pending->m_lRoseResult = lRoseResult;
	pending->m_stResponseData = stResponseData;

	// Fire-and-forget (iTimeout==0): match sync SendInvoke — finalize dispatch telemetry only,
	// discard any response that arrives before or after Send() returns, never call the callback.
	if (pending->m_bFireAndForgetAsync)
	{
		pending->EnsureOutboundTelemetry();
		if (pending->m_pTelemetry)
		{
			pending->FinalizeTelemetry(lRoseResult, pending->m_pAsyncContext);
			OnInvokeProcessed(pending->m_pTelemetry);
			pending->m_pTelemetry.reset();
		}

		NotifyWatchdog();
		return;
	}

	if (pMessage)
		pending->m_pAnswerMessage = std::move(pMessage);

	long lFinalRoseResult = lRoseResult;
	std::shared_ptr<SnaccInvokeContext> pCtx = pending->m_pAsyncContext;
	if (pending->m_pAnswerMessage && pCtx)
	{
		lFinalRoseResult = HandleInvokeResult(lFinalRoseResult, *pending->m_pAnswerMessage, pCtx->AsyncResultBuffer(), pCtx->AsyncErrorBuffer(), *pCtx);
	}

	pending->EnsureOutboundTelemetry();

	if (pending->m_pTelemetry)
	{
		pending->FinalizeTelemetry(lFinalRoseResult, pCtx);
		OnInvokeProcessed(pending->m_pTelemetry);
		pending->m_pTelemetry.reset();
	}

	SnaccInvokeAsyncCallback callback;
	if (pCtx && pCtx->HasAsyncCompletion())
		callback = pCtx->AsyncCallback();

	NotifyWatchdog();

	if (callback && pCtx)
		callback(lFinalRoseResult, *pCtx);
}

void SnaccROSEBase::ProcessAsyncTimeouts()
{
	const auto now = std::chrono::steady_clock::now();
	std::vector<long> timedOutInvokeIds;
	{
		std::lock_guard<std::mutex> guard(m_InternalProtectMutex);
		for (const auto& entry : m_PendingOperations)
		{
			const SnaccROSEPendingOperation& pending = *entry.second;
			if (pending.m_bAsyncInvoke && pending.m_asyncDeadline.has_value() && now >= pending.m_asyncDeadline.value())
				timedOutInvokeIds.push_back(pending.m_lInvokeID);
		}
	}

	for (const long invokeID : timedOutInvokeIds)
		FinishAsyncInvoke(invokeID, ROSE_TE_TIMEOUT, {}, 0);
}

void SnaccROSEBase::NotifyWatchdog()
{
	bool stopWatchdog = false;
	{
		std::lock_guard<std::mutex> guard(m_InternalProtectMutex);
		stopWatchdog = (m_asyncDeadlineCount == 0);
	}

	std::lock_guard<std::mutex> guard(m_watchdogMutex);
	if (stopWatchdog)
		m_watchdogStopRequested = true;
	m_watchdogCv.notify_all();
}

void SnaccROSEBase::EnsureWatchdogRunning()
{
	std::lock_guard<std::mutex> guard(m_watchdogMutex);
	if (m_watchdogThreadRunning)
		return;

	m_watchdogStopRequested = false;
	m_watchdogThreadRunning = true;
	m_watchdogThread = std::thread([this]() {
		for (;;)
		{
			std::chrono::steady_clock::time_point nextDeadline = std::chrono::steady_clock::time_point::max();
			{
				std::lock_guard<std::mutex> pendingGuard(m_InternalProtectMutex);
				for (const auto& entry : m_PendingOperations)
				{
					const SnaccROSEPendingOperation& pending = *entry.second;
					if (pending.m_bAsyncInvoke && pending.m_asyncDeadline.has_value() && pending.m_asyncDeadline.value() < nextDeadline)
						nextDeadline = pending.m_asyncDeadline.value();
				}
			}

			std::unique_lock<std::mutex> watchdogLock(m_watchdogMutex);
			if (nextDeadline == std::chrono::steady_clock::time_point::max())
			{
				m_watchdogCv.wait(watchdogLock, [this]() {
					if (m_watchdogStopRequested)
						return true;
					std::lock_guard<std::mutex> pendingGuard(m_InternalProtectMutex);
					return m_asyncDeadlineCount > 0;
				});
				if (m_watchdogStopRequested)
				{
					std::lock_guard<std::mutex> pendingGuard(m_InternalProtectMutex);
					if (m_asyncDeadlineCount == 0)
						break;
				}
				continue;
			}

			if (m_watchdogCv.wait_until(watchdogLock, nextDeadline, [this]() { return m_watchdogStopRequested; }))
			{
				std::lock_guard<std::mutex> pendingGuard(m_InternalProtectMutex);
				if (m_asyncDeadlineCount == 0)
					break;
			}
			else
			{
				watchdogLock.unlock();
				ProcessAsyncTimeouts();
			}
		}

		std::lock_guard<std::mutex> guard(m_watchdogMutex);
		m_watchdogThreadRunning = false;
	});
}

void SnaccROSEBase::StopWatchdogThread()
{
	{
		std::lock_guard<std::mutex> guard(m_watchdogMutex);
		m_watchdogStopRequested = true;
		m_watchdogCv.notify_all();
	}

	if (m_watchdogThread.joinable())
		m_watchdogThread.join();
}

long SnaccROSEBase::SendInvokeAsync(SNACC::ROSEInvoke* pInvoke, SNACC::AsnType* pResult, SNACC::AsnType* pError, const char* szOperationName /*= nullptr*/, int iTimeout /*= -1*/, std::shared_ptr<SnaccInvokeContext> pCtx /*= {}*/)
{
	const auto chronoCreated = std::chrono::steady_clock::now();
	const char* szResolvedOperationName = SnaccRoseOperationLookup::LookUpName(pInvoke->operationID);

	if (!pCtx)
	{
		SnaccInvokeContextInit init(SnaccInvokeDirection::OUTBOUND, pInvoke, szResolvedOperationName ? szResolvedOperationName : szOperationName);
		pCtx = CreateInvokeContext(init);
	}

	if (iTimeout == -1)
		iTimeout = m_lMaxInvokeWait;

	const bool bFireAndForget = (iTimeout == 0);

	// Fire-and-forget is dispatch-only; a completion callback contradicts iTimeout==0.
	ASSERT(!bFireAndForget || !pCtx->HasAsyncCompletion());

	if (!bFireAndForget && !pCtx->HasAsyncCompletion())
	{
		pCtx->SetAsyncCompletion([](long, SnaccInvokeContext&) {}, pResult, pError);
	}

	auto& ctx = *pCtx;

	if (!IsProcessingAllowed())
	{
		SnaccInvokeAsyncCallback shutdownCallback;
		if (!bFireAndForget && pCtx->HasAsyncCompletion())
			shutdownCallback = pCtx->AsyncCallback();

		auto telemetry = SnaccTelemetryData::Create(SnaccTelemetryData::Direction::OUTBOUND, pInvoke->operationID, szResolvedOperationName, 0, chronoCreated);
		telemetry->finalize(SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::OUTBOUND_SEND, SnaccTelemetryData::Reason::SHUTDOWN, ROSE_TE_SHUTDOWN, std::nullopt, pCtx);
		OnInvokeProcessed(telemetry);
		if (shutdownCallback)
			shutdownCallback(ROSE_TE_SHUTDOWN, *pCtx);
		return ROSE_TE_SHUTDOWN;
	}

	auto& pendingOP = AddPendingOperation(pInvoke->invokeID, pInvoke->operationID, szResolvedOperationName);
	pendingOP.m_bAsyncInvoke = true;
	pendingOP.m_bFireAndForgetAsync = bFireAndForget;
	pendingOP.m_pAsyncContext = pCtx;
	pendingOP.m_chronoTelemetryCreated = chronoCreated;

	if (iTimeout > 0)
	{
		pendingOP.m_asyncDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(iTimeout);
		{
			std::lock_guard<std::mutex> guard(m_InternalProtectMutex);
			++m_asyncDeadlineCount;
		}
		EnsureWatchdogRunning();
		NotifyWatchdog();
	}

	size_t stRequestData = 0;
	const long lRoseResult = Send(pInvoke, szOperationName, ctx, &stRequestData);
	(void)stRequestData;

	if (bFireAndForget)
	{
		// Finish even when the response was already consumed inside Send(); second call is a no-op.
		FinishAsyncInvoke(pendingOP.m_lInvokeID, lRoseResult, {}, 0);
		return lRoseResult;
	}

	if (lRoseResult != ROSE_NOERROR)
	{
		FinishAsyncInvoke(pendingOP.m_lInvokeID, lRoseResult, {}, 0);
		return lRoseResult;
	}

	{
		std::lock_guard<std::mutex> guard(m_InternalProtectMutex);
		if (m_PendingOperations.find(pInvoke->invokeID) == m_PendingOperations.end())
			return ROSE_NOERROR;
	}

	return ROSE_NOERROR;
}

long SnaccROSEBase::DecodeResponse(const SNACC::ROSEMessage& response, SNACC::ROSEResult*& pResult, SNACC::ROSEError*& pError, SnaccInvokeContext& ctx)
{
	long lRoseResult = ROSE_RE_INVALID_ANSWER;
	pResult = nullptr;
	pError = nullptr;

	switch (response.choiceId)
	{
		case ROSEMessage::notinitialized:
			throw std::runtime_error("response ROSEMessage choiceId is notinitialized");
		case ROSEMessage::invokeCid:
			break;
		case ROSEMessage::resultCid:
			lRoseResult = ROSE_NOERROR;
			pResult = response.result;
			break;
		case ROSEMessage::errorCid:
			lRoseResult = ROSE_ERROR_VALUE;
			pError = response.error;
			break;
		case ROSEMessage::rejectCid:
			lRoseResult = GetRejectResultCode(response.reject, ctx);
			break;
	}

	return lRoseResult;
}

long SnaccROSEBase::EncodeResult(unsigned int uiInvokeID, const SNACC::AsnType* pResult, std::string& strResponse, const wchar_t* szSessionID /*= nullptr*/)
{
	long lRoseResult = ROSE_NOERROR;
	RoseEncodeResultEnvelope encode(uiInvokeID, pResult, szSessionID);

	if (m_eTransportEncoding == SNACC::TransportEncoding::BER)
	{
		AsnBuf OutBuf;
		AsnLen BytesEncoded = encode.message().BEnc(OutBuf);

		OutBuf.ResetMode();
		OutBuf.GetSeg(strResponse, BytesEncoded);

		LogTransportData(true, m_eTransportEncoding, nullptr, strResponse.c_str(), BytesEncoded, &encode.message(), nullptr);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		auto value = encode.message().JEnc();

		int logLevel = (int)GetLogLevel(true);
		if (logLevel & (int)EAsnLogLevel::JSON || logLevel & (int)EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED)
			strResponse = getPrettyPrinted(value);
		else
		{
			SJson::FastWriter writer;
			strResponse = writer.write(value);
		}

		std::string strPrefix;
		if (m_eTransportEncoding == SNACC::TransportEncoding::JSON)
			lRoseResult = GetJsonLengthPrefix(strResponse, strPrefix);
		if (lRoseResult == ROSE_NOERROR)
		{
			LogTransportData(true, m_eTransportEncoding, nullptr, strResponse.c_str(), strResponse.length(), &encode.message(), nullptr);
			if (!strPrefix.empty())
				strResponse.insert(0, strPrefix);
		}
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	return lRoseResult;
}

long SnaccROSEBase::EncodeError(unsigned int uiInvokeID, const SNACC::AsnType* pError, std::string& strResponse, const wchar_t* szSessionID /*= nullptr*/)
{
	long lRoseResult = ROSE_NOERROR;
	RoseEncodeErrorEnvelope encode(uiInvokeID, pError, szSessionID);

	if (m_eTransportEncoding == SNACC::TransportEncoding::BER)
	{
		AsnBuf OutBuf;
		AsnLen BytesEncoded = encode.message().BEnc(OutBuf);

		OutBuf.ResetMode();
		OutBuf.GetSeg(strResponse, BytesEncoded);

		LogTransportData(true, m_eTransportEncoding, nullptr, strResponse.c_str(), strResponse.length(), &encode.message(), nullptr);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		auto value = encode.message().JEnc();

		int logLevel = (int)GetLogLevel(true);
		if (logLevel & (int)EAsnLogLevel::JSON || logLevel & (int)EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED)
			strResponse = getPrettyPrinted(value);
		else
		{
			SJson::FastWriter writer;
			strResponse = writer.write(value);
		}

		std::string strPrefix;
		if (m_eTransportEncoding == SNACC::TransportEncoding::JSON)
			lRoseResult = GetJsonLengthPrefix(strResponse, strPrefix);
		if (lRoseResult == ROSE_NOERROR)
		{
			LogTransportData(true, m_eTransportEncoding, nullptr, strResponse.c_str(), strResponse.length(), &encode.message(), nullptr);
			if (!strPrefix.empty())
				strResponse.insert(0, strPrefix);
		}
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	return lRoseResult;
}

long SnaccROSEBase::SendBinaryDataBlockEx(const char* lpBytes, size_t size, SnaccInvokeContext& ctx)
{
	if (m_pTransport)
		return m_pTransport->SendBinaryDataBlockEx(lpBytes, size, ctx);
	return ROSE_TE_TRANSPORTFAILED;
}

void SnaccROSEBase::SetSnaccROSETransport(ISnaccROSETransport* pTransport)
{
	m_pTransport = pTransport;
}

bool SnaccROSEBase::SetTransportEncoding(const SNACC::TransportEncoding transportEncoding)
{
	if (transportEncoding == SNACC::TransportEncoding::BER || transportEncoding == SNACC::TransportEncoding::JSON || transportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		m_eTransportEncoding = transportEncoding;
		return true;
	}
	return false;
}

SNACC::TransportEncoding SnaccROSEBase::GetTransportEncoding() const
{
	return m_eTransportEncoding;
}

long SnaccROSEBase::HandleInvokeResult(long lRoseResult, const SNACC::ROSEMessage& responseMsg, SNACC::AsnType* pResult, SNACC::AsnType* pError, SnaccInvokeContext& ctx)
{
	// In case of transport errors, we hand that value back as we have no response to parse
	if (ISROSE_TE(lRoseResult))
		return lRoseResult;

	SNACC::ROSEError* pRoseError = nullptr;
	SNACC::ROSEResult* pRoseResult = nullptr;
	lRoseResult = DecodeResponse(responseMsg, pRoseResult, pRoseError, ctx);

	if (lRoseResult == ROSE_NOERROR)
	{
		if (pRoseResult && pRoseResult->result && pResult)
		{
			AsnLen len;
			try
			{
				if (pRoseResult->result->result.anyBuf)
				{
					pResult->BDec(*pRoseResult->result->result.anyBuf, len);
					// Special to log the *full* ROSE Message in json
					// While for JSON Transport we can simply decode the full message on BER we need to decode the envelop and the payload
					//
					// We log the plain received BER payload in OnBinaryDataBlock and the fully decoded message here
					// To be able to do this we need to put the decoded payload (here result) into the pResponseMsg and then log it
					// We only do this if JSON logging is enabled
					int logLevel = (int)GetLogLevel(false);
					if (logLevel & ((int)SNACC::EAsnLogLevel::JSON | (int)SNACC::EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED))
					{
						// LogTransportData needs the decoded payload inside the ROSE tree for JEnc().
						// Graft temporarily, then restore below — the owned message is unchanged afterward.
						// In-place graft avoids copying ROSEMessage and duplicating the BER anyBuf.
						auto& logMsg = const_cast<SNACC::ROSEMessage&>(responseMsg);
						// Backup the original response object inside the response message
						ROSEResultSeq* pOriginalResponse = logMsg.result->result;

						// Set the decoded result object into the response to be able to fully log the whole message
						logMsg.result->result = new ROSEResultSeq();
						logMsg.result->result->result.value = pResult;
						LogTransportData(false, SNACC::TransportEncoding::BER, nullptr, nullptr, 0, &logMsg, nullptr);
						// As we hand back the result object to the outer world (function argument) we need to set it to NULL to prevent deletion if we discard the inserted object
						logMsg.result->result->result.value = NULL;

						// Delete the inserted result object and reset to the original response object
						delete logMsg.result->result;
						logMsg.result->result = pOriginalResponse;
					}
				}
				else if (pRoseResult->result->result.jsonBuf)
				{
					if (!pResult->JDec(*pRoseResult->result->result.jsonBuf))
						lRoseResult = ROSE_RE_DECODE_FAILED;
					// No logging here as the full object has already been logged in OnBinaryDataBlock
				}
			}
			catch (const SnaccException& ex)
			{
				SJson::Value err;
				err["exception"] = ex.what();
				err["method"] = __FUNCTION__;
				err["error"] = (int)ex.m_errorCode;
				std::string strError = getPrettyPrinted(err);
				PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

				lRoseResult = ROSE_RE_DECODE_FAILED;
			}
		}
	}
	else if (lRoseResult == ROSE_ERROR_VALUE)
	{
		if (pRoseError && pRoseError->error && pError)
		{
			AsnLen len;
			try
			{
				if (pRoseError->error->anyBuf)
				{
					pError->BDec(*pRoseError->error->anyBuf, len);
					// Special to log the *full* ROSE Message in json
					// While for JSON Transport we can simply decode the full message on BER we need to decode the envelop and the payload
					//
					// We log the plain received BER payload in OnBinaryDataBlock and the fully decoded message here
					// To be able to do this we need to put the decoded payload (here error) into the pResponseMsg and then log it
					// We only do this if JSON logging is enabled
					int logLevel = (int)GetLogLevel(false);
					if (logLevel & ((int)SNACC::EAsnLogLevel::JSON | (int)SNACC::EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED))
					{
						// LogTransportData needs the decoded payload inside the ROSE tree for JEnc().
						// Graft temporarily, then restore below — the owned message is unchanged afterward.
						// In-place graft avoids copying ROSEMessage and duplicating the BER anyBuf.
						auto& logMsg = const_cast<SNACC::ROSEMessage&>(responseMsg);
						// Backup the original error object inside the response message
						AsnAny* pOriginalError = logMsg.error->error;

						// Set the decoded error object into the response to be able to fully log the whole message
						logMsg.error->error = new AsnAny();
						logMsg.error->error->value = pError;
						LogTransportData(false, SNACC::TransportEncoding::BER, nullptr, nullptr, 0, &logMsg, nullptr);
						// As we hand back the error object to the outer world (function argument) we need to set it to NULL to prevent deletion if we discard the inserted object
						logMsg.error->error->value = NULL;

						// Delete the inserted error object and reset to the original response object
						delete logMsg.error->error;
						logMsg.error->error = pOriginalError;
					}
				}
				else if (pRoseError->error->jsonBuf)
				{
					if (!pError->JDec(*pRoseError->error->jsonBuf))
						lRoseResult = ROSE_RE_DECODE_FAILED;
					// No logging here as the full object has already been logged in OnBinaryDataBlock
				}
			}
			catch (const SnaccException& ex)
			{
				SJson::Value err;
				err["exception"] = ex.what();
				err["method"] = __FUNCTION__;
				err["error"] = (int)ex.m_errorCode;
				std::string strError = getPrettyPrinted(err);
				PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

				lRoseResult = ROSE_RE_DECODE_FAILED;
			}
		}
	}

	return lRoseResult;
}

long SnaccROSEBase::HandleOnInvokeResult(SNACC::InvokeResult invokeResult, const SNACC::ROSEInvoke& invoke, SnaccInvokeContext& ctx, std::string& strResponse, SNACC::AsnType* pResult, SNACC::AsnType* pError)
{
	if ((AsnIntType)invoke.invokeID == 99999)
	{
		strResponse.clear();
		ctx.m_bResponseIsError = false;
		return ROSE_NOERROR;
	}

	switch (invokeResult)
	{
		case InvokeResult::returnResult:
			return EncodeResult(invoke.invokeID, pResult, strResponse);
		case InvokeResult::returnError:
			ctx.m_bResponseIsError = true;
			return EncodeError(invoke.invokeID, pError, strResponse);
		default:
			return ctx.m_lRejectResult ? ctx.m_lRejectResult : ROSE_REJECT_FUNCTIONMISSING;
	}
}

long SnaccROSEBase::DecodeInvoke(const SNACC::ROSEMessage& invokeMessage, SNACC::AsnType* pArgument)
{
	auto& invoke = *invokeMessage.invoke;
	if (!invoke.argument)
	{
		if (pArgument->mayBeEmpty())
			return ROSE_NOERROR;
		else
			return ROSE_REJECT_ARGUMENT_MISSING;
	}

	long lRoseResult = ROSE_NOERROR;

	try
	{
		if (invoke.argument->anyBuf)
		{
			AsnLen len = 0;
			pArgument->BDec(*invoke.argument->anyBuf, len);
			// Special to log the *full* ROSE Message in json
			// While for JSON Transport we can simply decode the full message on BER we need to decode the envelop and the payload
			//
			// We log the plain received BER payload in OnBinaryDataBlock and the fully decoded message here
			// To be able to do this we need to put the decoded payload (here result) into the response message and then log it
			// We only do this if JSON logging is enabled
			int logLevel = (int)GetLogLevel(false);
			if (logLevel & ((int)SNACC::EAsnLogLevel::JSON | (int)SNACC::EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED))
			{
				// LogTransportData needs the decoded payload inside the ROSE tree for JEnc().
				// Graft temporarily, then restore below — the owned message is unchanged afterward.
				// In-place graft avoids copying ROSEMessage and duplicating the BER anyBuf.
				auto& logMsg = const_cast<SNACC::ROSEMessage&>(invokeMessage);
				auto& logInvoke = *logMsg.invoke;
				// Backup the original response object inside the response message
				AsnAny* pOriginalArgument = logInvoke.argument;

				// Set the decoded result object into the response to be able to fully log the whole message
				logInvoke.argument = new AsnAny();
				logInvoke.argument->value = pArgument;

				// Get the name of the called operation for logging
				std::string strOperationName;
				const char* szOperationName = nullptr;
				if (logInvoke.operationName)
				{
					strOperationName = logInvoke.operationName->getUTF8();
					szOperationName = strOperationName.c_str();
				}
				if (!szOperationName)
					szOperationName = SnaccRoseOperationLookup::LookUpName(logInvoke.operationID);
				LogTransportData(false, SNACC::TransportEncoding::BER, szOperationName, nullptr, 0, &logMsg, nullptr);
				// As we hand back the result object to the outer world (function argument) we need to set it to NULL to prevent deletion if we discard the inserted object
				logInvoke.argument->value = NULL;

				// Delete the inserted result object and reset to the original response object
				delete logInvoke.argument;
				logInvoke.argument = pOriginalArgument;
			}
		}
		else if (invoke.argument->jsonBuf)
		{
			pArgument->JDec(*invoke.argument->jsonBuf);
			// No logging here as the full object has already been logged in OnBinaryDataBlock
		}
		else
			lRoseResult = ROSE_RE_DECODE_FAILED;
	}
	catch (const SnaccException& ex)
	{
		SJson::Value error;
		error["exception"] = ex.what();
		error["method"] = __FUNCTION__;
		error["error"] = (int)ex.m_errorCode;
		std::string strError = getPrettyPrinted(error);
		PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

		lRoseResult = ROSE_RE_DECODE_FAILED;
	}

	return lRoseResult;
}

#ifdef HAS_WCHAR_T
#define STRINGLEN(sz) wcslen(sz)
#else
#define STRINGLEN(sz) strlen(sz)
#endif

int SnaccROSEBase::ConfigureFileLogging(const LOG_CHARTYPE* szPath, bool bAppend /*= true*/, const bool bFlushEveryWrite /* = true */)
{
	std::lock_guard<std::mutex> lock(m_mtxLogFile);

	if (szPath && STRINGLEN(szPath) && !m_pAsnLogFile)
	{
		m_bFlushEveryWrite = bFlushEveryWrite;

		std::filesystem::path filePath(szPath);
		if (!std::filesystem::exists(filePath))
			bAppend = false;

#ifdef HAS_WCHAR_T
		const wchar_t* szMode = bAppend ? L"r+b" : L"wb";
#else
		const char* szMode = bAppend ? "r+b" : "wb";
#endif
#ifdef _MSC_VER
		m_pAsnLogFile = _wfsopen(szPath, szMode, _SH_DENYWR);
#else
		m_pAsnLogFile = fopen(szPath, szMode);
#endif
		if (!m_pAsnLogFile)
		{
			int iErr = 0;
#ifdef _WIN32
			_get_errno(&iErr);
#else
			iErr = errno;
#endif
			return iErr;
		}
		else
		{
			fseek(m_pAsnLogFile, 0, SEEK_END);
			m_bAsnLogFileContainsData = ftell(m_pAsnLogFile) > 0;
		}
	}
	else if ((!szPath || !STRINGLEN(szPath)) && m_pAsnLogFile)
	{
		fclose(m_pAsnLogFile);
		m_pAsnLogFile = nullptr;
		m_bAsnLogFileContainsData = false;
	}
	return 0;
}

bool SnaccROSEBase::PrintJSONToLog(const bool bOutbound, const bool bException, const char* szOperationName, const char* szData, const size_t size)
{
	if (!m_pAsnLogFile || !szData)
		return false;

	// The ASN.1 functions may be called from different threads as well.
	// Writing to roseout.log therefore has to be serialized.
	// The lock should not hurt, as only the file itself is locked and no other object (PAIM-1732).
	std::lock_guard<std::mutex> lock(m_mtxLogFile);

	// Securly check the logfile pointer once more after aquiring the lock
	if (!m_pAsnLogFile)
		return false;

	try
	{
		auto currentTime = std::chrono::system_clock::now();
		std::time_t currentTimeT = std::chrono::system_clock::to_time_t(currentTime);
		auto duration = currentTime.time_since_epoch();
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;
		std::tm timeInfo;
#ifdef _WIN32
		gmtime_s(&timeInfo, &currentTimeT);
#else
		gmtime_r(&currentTimeT, &timeInfo);
#endif
		std::ostringstream strTime;
		strTime << std::put_time(&timeInfo, "%Y-%m-%dT%H:%M:%S.");
		strTime << std::setfill('0') << std::setw(3) << milliseconds.count();

		if (!m_bAsnLogFileContainsData)
		{
			fprintf(m_pAsnLogFile, "[");
			m_bAsnLogFileContainsData = true;
		}
		else
		{
			fseek(m_pAsnLogFile, -1, SEEK_END);
			fprintf(m_pAsnLogFile, ",\n");
		}

		fprintf(m_pAsnLogFile, "{\n\t\"%s\" : \"%s\",\n", bOutbound ? "OUT" : "IN", strTime.str().c_str());

		if (szOperationName)
			fprintf(m_pAsnLogFile, "\t\"OPERATION\" : \"%s\",\n", szOperationName);

		// if the data is encapsulated json we need a name
		if (*szData == '{' || *szData == '[')
			fprintf(m_pAsnLogFile, "\t\"%s\" : \n", bException ? "ERROR" : "ROSE");

		size_t stPrintLength = size;
		if (!stPrintLength)
		{
			try
			{
				stPrintLength = strlen(szData);
			}
			catch (...)
			{
				// No length was handed over
				// When trying to get the length from a zero terminated string it looks like we ran into memory we are not allowed to read
				ASSERT(false);
			}
		}

		// Remove potential null characters at the end of the string
		while (stPrintLength > 0)
			if (szData[stPrintLength - 1])
				break;
			else
				stPrintLength--;

		const char* start = szData;
		const char* end = szData;
		size_t stCount = 0;
		while (stCount < stPrintLength)
		{
			if (*end == '\n')
			{
				fprintf(m_pAsnLogFile, "\t");
				fwrite(start, sizeof(char), end - start + 1, m_pAsnLogFile);
				start = end + 1;
			}
			stCount++;
			end++;
		}

		if (start != end)
		{
			fprintf(m_pAsnLogFile, "\t");
			fwrite(start, sizeof(char), end - start, m_pAsnLogFile);
		}

		if (szData[stPrintLength - 1] != '\n')
			fprintf(m_pAsnLogFile, "\n");
		fprintf(m_pAsnLogFile, "}]");

		if (m_bFlushEveryWrite)
			fflush(m_pAsnLogFile);

		return true;
	}
	catch (...)
	{
		ASSERT(false);
	}
	return false;
}

void SnaccROSEBase::OnInvokeProcessed(std::shared_ptr<const SnaccTelemetryData> data)
{
	if (m_pTelemetryCallback)
		m_pTelemetryCallback->OnInvokeProcessed(data);
}
