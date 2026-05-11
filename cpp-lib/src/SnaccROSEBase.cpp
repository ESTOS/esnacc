#include "../include/SnaccROSEBase.h"
#include "../include/SNACCROSE.h"
#include "snacc-assert.h"
#include <stdio.h>
#include <wchar.h>
#include <iomanip>
#include <locale>
#include <chrono>
#include <filesystem>

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

SnaccScopedInvokeMessage::SnaccScopedInvokeMessage(long invokeID, unsigned int uiOperationID, SNACC::AsnType* argument)
	: m_pInvoke(new SNACC::ROSEInvoke())
{
	m_pInvoke->invokeID = invokeID;
	m_pInvoke->operationID = uiOperationID;
	m_pInvoke->argument = new AsnAny;
	m_pInvoke->argument->value = argument;
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

std::string getPrettyPrinted(const SJson::Value& value)
{
	SJson::StreamWriterBuilder wbuilder;
	wbuilder["indentation"] = "\t";
	return SJson::writeString(wbuilder, value);
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
	  m_pAnswerMessage(nullptr),
	  m_lRoseResult(0),
	  m_stResponseData(0)
{
}

SnaccROSEPendingOperation::~SnaccROSEPendingOperation()
{
	if (m_pAnswerMessage)
		delete m_pAnswerMessage;
}

bool SnaccROSEPendingOperation::WaitForComplete(long ulTimeOut /*= -1*/)
{
	return m_CompletedEvent.waitfor(ulTimeOut);
}

void SnaccROSEPendingOperation::CompleteOperation(long lRoseResult, const SNACC::ROSEMessage* pAnswerMessage, size_t stResponseData)
{
	m_lRoseResult = lRoseResult;
	m_stResponseData = stResponseData;
	if (pAnswerMessage)
		m_pAnswerMessage = pAnswerMessage;
	m_CompletedEvent.signal();
}

void SnaccROSEPendingOperation::FinalizeTelemetry(std::shared_ptr<SnaccInvokeContext> pctx)
{
	if (!m_pTelemetry)
		return;

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

	m_pTelemetry->finalize(SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::OUTBOUND_WAIT, SnaccTelemetryData::Reason::WAIT_SKIPPED, m_lRoseResult, std::nullopt, std::move(pctx));
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
	ConfigureFileLogging(nullptr);
}

void SnaccROSEBase::StopProcessing(bool bStop /*= true*/)
{
	{
		std::lock_guard<std::mutex> guard(m_InternalProtectMutex);
		m_bProcessingAllowed = bStop ? false : true;
	}

	CompleteAllPendingOperations();
}

void SnaccROSEBase::SetMaxInvokeWaitTime(long lMaxInvokeWait)
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);
	m_lMaxInvokeWait = lMaxInvokeWait;
}

void SnaccROSEBase::CompleteAllPendingOperations()
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	for (auto it = m_PendingOperations.begin(); it != m_PendingOperations.end(); it++)
		it->second->CompleteOperation(ROSE_TE_SHUTDOWN, NULL);
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

bool SnaccROSEBase::CompletePendingOperation(int invokeID, const SNACC::ROSEMessage* pMessage, unsigned long ulMessageSize)
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	const auto it = m_PendingOperations.find(invokeID);
	if (it != m_PendingOperations.end())
	{
		// found...
		long lRoseResult = ROSE_RE_INVALID_ANSWER;
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

		it->second->CompleteOperation(lRoseResult, pMessage, ulMessageSize);
		return true;
	}
	else
	{
		delete pMessage;
	}

	return false;
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
					ROSEMessage* pmessage = new ROSEMessage;
					AsnLen bytesDecoded = 0;
					try
					{
						pmessage->BDec(buffer, bytesDecoded);
					}
					catch (const SnaccException& ex)
					{
						if (bLogTransportData)
							LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

						SJson::Value error;
						error["exception"] = ex.what();
						error["method"] = __FUNCTION__;
						error["error"] = (int)ex.m_errorCode;
						std::string strError = getPrettyPrinted(error);
						PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

						unsigned int uiOperationID = 0;
						const char* szOperationName = nullptr;
						auto outcome = SnaccTelemetryData::Outcome::UNHANDLED;
						long lTelemetryResult = ROSE_RE_DECODE_FAILED;
						if (pmessage->choiceId == ROSEMessage::invokeCid && pmessage->invoke)
						{
							auto* pInvoke = pmessage->invoke;
							uiOperationID = pInvoke->operationID;
							szOperationName = SnaccRoseOperationLookup::LookUpName(uiOperationID);
							if ((AsnIntType)pInvoke->invokeID != 99999)
							{
								ROSEReject reject;
								if ((AsnIntType)pInvoke->invokeID)
								{
									reject.invokedID.choiceId = ROSERejectChoice::invokedIDCid;
									reject.invokedID.invokedID = new AsnInt(pInvoke->invokeID);
								}
								else
								{
									reject.invokedID.choiceId = ROSERejectChoice::invokednullCid;
									reject.invokedID.invokednull = new AsnNull;
								}

								const long lRejectResult = SendRejectEx(&reject);
								if (lRejectResult == ROSE_NOERROR)
								{
									outcome = SnaccTelemetryData::Outcome::REJECT;
									lTelemetryResult = ROSE_REJECT_MISTYPEDARGUMENT;
								}
								else
									lTelemetryResult = lRejectResult;
							}
						}
						OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, uiOperationID, szOperationName, lSize, outcome, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, lTelemetryResult));
						delete pmessage;
						return true;
					}

					if (bLogTransportData)
						LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

					// pmessage will be deleted inside
					bReturn = OnROSEMessage(pmessage, false, lSize);
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
						bool bSuccess = false;
						ROSEMessage* pmessage = new ROSEMessage;
						try
						{
							bSuccess = pmessage->JDec(value);
							if (!bSuccess)
								throw InvalidTagException("ROSEMessage", "decode failed: ROSEMessage", STACK_ENTRY);
						}
						catch (const SnaccException& ex)
						{
							if (bLogTransportData)
								LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

							SJson::Value error;
							error["exception"] = ex.what();
							error["method"] = __FUNCTION__;
							error["error"] = (int)ex.m_errorCode;
							std::string strError = getPrettyPrinted(error);
							PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

							unsigned int uiOperationID = 0;
							const char* szOperationName = nullptr;
							auto outcome = SnaccTelemetryData::Outcome::UNHANDLED;
							long lTelemetryResult = ROSE_RE_DECODE_FAILED;
							if (pmessage->choiceId == ROSEMessage::invokeCid && pmessage->invoke)
							{
								auto* pInvoke = pmessage->invoke;
								uiOperationID = pInvoke->operationID;
								szOperationName = SnaccRoseOperationLookup::LookUpName(uiOperationID);
								if ((AsnIntType)pInvoke->invokeID != 99999)
								{
									ROSEReject reject;
									if ((AsnIntType)pInvoke->invokeID)
									{
										reject.invokedID.choiceId = ROSERejectChoice::invokedIDCid;
										reject.invokedID.invokedID = new AsnInt(pInvoke->invokeID);
									}
									else
									{
										reject.invokedID.choiceId = ROSERejectChoice::invokednullCid;
										reject.invokedID.invokednull = new AsnNull;
									}
									reject.reject = new RejectProblem;
									reject.reject->choiceId = RejectProblem::invokeProblemCid;
									reject.reject->invokeProblem = new InvokeProblem;
									*reject.reject->invokeProblem = InvokeProblem::mistypedArgument;
									reject.details = UTF8String::CreateNewFromASCII(strError.c_str());

									const long lRejectResult = SendRejectEx(&reject);
									if (lRejectResult == ROSE_NOERROR)
									{
										outcome = SnaccTelemetryData::Outcome::REJECT;
										lTelemetryResult = ROSE_REJECT_MISTYPEDARGUMENT;
									}
									else
										lTelemetryResult = lRejectResult;
								}
							}
							OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, uiOperationID, szOperationName, lSize, outcome, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, lTelemetryResult));
							delete pmessage;
							return true;
						}

						if (bLogTransportData)
							LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, pmessage, &value);

						// pmessage will be deleted inside
						bReturn = OnROSEMessage(pmessage, false, lSize);
					}
					else
					{
						if (bLogTransportData)
							LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

#ifdef _DEBUG
						std::string strPayLoad;
						strPayLoad.assign(lpBytes, lSize);
#endif
						SJson::Value error;
						error["exception"] = reader.getFormattedErrorMessages();
						error["where"] = __FUNCTION__;
						std::string strError = getPrettyPrinted(error);
						PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());
						OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, 0, nullptr, lSize, SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, ROSE_RE_DECODE_FAILED));
					}
					break;
				}
			default:
				throw std::runtime_error("invalid m_eTransportEncoding");
				break;
		}
	}
	catch (const SnaccException& ex)
	{
		SJson::Value error;
		error["exception"] = ex.what();
		error["method"] = __FUNCTION__;
		error["error"] = (int)ex.m_errorCode;
		std::string strError = getPrettyPrinted(error);
		PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());
		OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, 0, nullptr, lSize, SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, ROSE_RE_DECODE_FAILED));
	}
	catch (const std::exception& ex)
	{
		SJson::Value error;
		error["exception"] = ex.what();
		error["method"] = __FUNCTION__;
		std::string strError = getPrettyPrinted(error);
		PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());
		OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, 0, nullptr, lSize, SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, ROSE_RE_DECODE_FAILED));
	}
	catch (...)
	{
		SJson::Value error;
		error["exception"] = "...";
		error["method"] = __FUNCTION__;
		std::string strError = getPrettyPrinted(error);
		PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());
		OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, 0, nullptr, lSize, SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, ROSE_RE_DECODE_FAILED));
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

std::string SnaccROSEBase::GetEncoded(const SNACC::TransportEncoding encoding, AsnType* pValue, unsigned long* pUlSize /* = nullptr */)
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
					ROSEMessage* pmessage = new ROSEMessage;
					AsnLen bytesDecoded = 0;
					try
					{
						pmessage->BDec(buffer, bytesDecoded);
						if (bLogTransportData)
							LogTransportData(false, SNACC::TransportEncoding::BER, nullptr, lpBytes, ulSize, nullptr, nullptr);

						// pmessage will be deleted inside
						OnROSEMessage(pmessage, true, ulSize);
					}
					catch (const SnaccException& ex)
					{
						if (bLogTransportData)
							bLogTransportData = LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, ulSize, nullptr, nullptr);

						SJson::Value error;
						error["exception"] = ex.what();
						error["method"] = __FUNCTION__;
						error["error"] = (int)ex.m_errorCode;
						std::string strError = getPrettyPrinted(error);
						PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

						OnRoseDecodeError(bLogTransportData, SNACC::TransportEncoding::BER, lpBytes, ulSize, ex.what());

						unsigned int uiOperationID = 0;
						const char* szOperationName = nullptr;
						auto outcome = SnaccTelemetryData::Outcome::UNHANDLED;
						long lTelemetryResult = ROSE_RE_DECODE_FAILED;
						if (pmessage->choiceId == ROSEMessage::invokeCid && pmessage->invoke)
						{
							auto* pInvoke = pmessage->invoke;
							uiOperationID = pInvoke->operationID;
							szOperationName = SnaccRoseOperationLookup::LookUpName(uiOperationID);
							if ((AsnIntType)pInvoke->invokeID != 99999)
							{
								// Provide the detail that the argument was non decodable to the client
								ROSEReject reject;
								reject.reject = new RejectProblem();
								reject.reject->choiceId = RejectProblem::invokeProblemCid;
								reject.reject->invokeProblem = new InvokeProblem(SNACC::InvokeProblem::mistypedArgument);
								reject.details = new UTF8String();
								reject.details->setASCII(ex.what());
								if ((AsnIntType)pInvoke->invokeID)
								{
									reject.invokedID.choiceId = ROSERejectChoice::invokedIDCid;
									reject.invokedID.invokedID = new AsnInt(pInvoke->invokeID);
								}
								else
								{
									reject.invokedID.choiceId = ROSERejectChoice::invokednullCid;
									reject.invokedID.invokednull = new AsnNull;
								}

								const long lRejectResult = SendRejectEx(&reject);
								if (lRejectResult == ROSE_NOERROR)
								{
									outcome = SnaccTelemetryData::Outcome::REJECT;
									lTelemetryResult = ROSE_REJECT_MISTYPEDARGUMENT;
								}
								else
									lTelemetryResult = lRejectResult;
							}
						}
						OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, uiOperationID, szOperationName, ulSize, outcome, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, lTelemetryResult));
						delete pmessage;
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
						ROSEMessage* pmessage = new ROSEMessage;
						try
						{
							if (!pmessage->JDec(value))
								throw InvalidTagException("ROSEMessage", "decode failed: ROSEMessage", STACK_ENTRY);
							if (bLogTransportData)
								LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, ulSize, pmessage, &value);
							// pmessage will be deleted inside
							OnROSEMessage(pmessage, true, ulSize);
						}
						catch (const SnaccException& ex)
						{
							if (bLogTransportData)
								bLogTransportData = LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, ulSize, nullptr, nullptr);

							SJson::Value error;
							error["exception"] = ex.what();
							error["method"] = __FUNCTION__;
							error["error"] = (int)ex.m_errorCode;
							std::string strError = getPrettyPrinted(error);
							PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

							OnRoseDecodeError(bLogTransportData, m_eTransportEncoding, lpBytes, ulSize, ex.what());

							unsigned int uiOperationID = 0;
							const char* szOperationName = nullptr;
							auto outcome = SnaccTelemetryData::Outcome::UNHANDLED;
							long lTelemetryResult = ROSE_RE_DECODE_FAILED;
							if (pmessage->choiceId == ROSEMessage::invokeCid && pmessage->invoke)
							{
								auto* pInvoke = pmessage->invoke;
								uiOperationID = pInvoke->operationID;
								szOperationName = SnaccRoseOperationLookup::LookUpName(uiOperationID);
								if ((AsnIntType)pInvoke->invokeID != 99999)
								{
									ROSEReject reject;
									if ((AsnIntType)pInvoke->invokeID)
									{
										reject.invokedID.choiceId = ROSERejectChoice::invokedIDCid;
										reject.invokedID.invokedID = new AsnInt(pInvoke->invokeID);
									}
									else
									{
										reject.invokedID.choiceId = ROSERejectChoice::invokednullCid;
										reject.invokedID.invokednull = new AsnNull;
									}
									reject.reject = new RejectProblem;
									reject.reject->choiceId = RejectProblem::invokeProblemCid;
									reject.reject->invokeProblem = new InvokeProblem;
									*reject.reject->invokeProblem = InvokeProblem::mistypedArgument;
									reject.details = UTF8String::CreateNewFromASCII(strError.c_str());

									const long lRejectResult = SendRejectEx(&reject);
									if (lRejectResult == ROSE_NOERROR)
									{
										outcome = SnaccTelemetryData::Outcome::REJECT;
										lTelemetryResult = ROSE_REJECT_MISTYPEDARGUMENT;
									}
									else
										lTelemetryResult = lRejectResult;
								}
							}
							OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, uiOperationID, szOperationName, ulSize, outcome, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, lTelemetryResult));
							delete pmessage;
						}
					}
					else
					{
						if (bLogTransportData)
							bLogTransportData = LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, ulSize, nullptr, nullptr);

						SJson::Value error;
						error["exception"] = reader.getFormattedErrorMessages();
						error["method"] = __FUNCTION__;
						std::string strError = getPrettyPrinted(error);
						PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

						OnRoseDecodeError(bLogTransportData, m_eTransportEncoding, lpBytes, ulSize, reader.getFormattedErrorMessages());

						ROSEReject reject;
						reject.invokedID.choiceId = ROSERejectChoice::invokednullCid;
						reject.invokedID.invokednull = new AsnNull;

						reject.reject = new RejectProblem;
						reject.reject->choiceId = RejectProblem::invokeProblemCid;
						reject.reject->invokeProblem = new InvokeProblem;
						*reject.reject->invokeProblem = InvokeProblem::mistypedArgument;
						reject.details = UTF8String::CreateNewFromASCII(strError.c_str());
						auto outcome = SnaccTelemetryData::Outcome::UNHANDLED;
						long lTelemetryResult = ROSE_RE_DECODE_FAILED;
						const long lRejectResult = SendRejectEx(&reject);
						if (lRejectResult == ROSE_NOERROR)
						{
							outcome = SnaccTelemetryData::Outcome::REJECT;
							lTelemetryResult = ROSE_REJECT_MISTYPEDARGUMENT;
						}
						else
							lTelemetryResult = lRejectResult;
						OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, 0, nullptr, ulSize, outcome, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, lTelemetryResult));
					}
					break;
				}
			default:
				{
					// if we don't know the encoding, we need to log it binary to ensure proper readability in the logs (ensure payload is hex converted)
					if (bLogTransportData)
						bLogTransportData = LogTransportData(false, SNACC::TransportEncoding::BER, nullptr, lpBytes, ulSize, nullptr, nullptr);
					OnRoseDecodeError(bLogTransportData, SNACC::TransportEncoding::BER, lpBytes, ulSize, "unknown encoding");
					OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, 0, nullptr, ulSize, SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, ROSE_RE_DECODE_FAILED));
					break;
				}
		}
	}
	catch (const SnaccException& ex)
	{
		SJson::Value error;
		error["exception"] = ex.what();
		error["method"] = __FUNCTION__;
		error["error"] = (int)ex.m_errorCode;
		std::string strError = getPrettyPrinted(error);
		PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());
		OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, 0, nullptr, ulSize, SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, ROSE_RE_DECODE_FAILED));
	}
	catch (const std::exception& ex)
	{
		SJson::Value error;
		error["exception"] = ex.what();
		error["method"] = __FUNCTION__;
		std::string strError = getPrettyPrinted(error);
		PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());
		OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, 0, nullptr, ulSize, SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, ROSE_RE_DECODE_FAILED));
	}
	catch (...)
	{
		SJson::Value error;
		error["exception"] = L"...";
		error["method"] = __FUNCTION__;
		std::string strError = getPrettyPrinted(error);
		PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());
		OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, 0, nullptr, ulSize, SnaccTelemetryData::Outcome::UNHANDLED, SnaccTelemetryData::Stage::INBOUND_DECODE, SnaccTelemetryData::Reason::DECODE_FAILED, ROSE_RE_DECODE_FAILED));
	}
}

bool SnaccROSEBase::OnROSEMessage(SNACC::ROSEMessage* pMessage, bool bAllowAllInvokes, unsigned long ulMessageSize)
{
	bool bProcessed = false;
	switch (pMessage->choiceId)
	{
		case ROSEMessage::invokeCid:
			if (pMessage->invoke)
			{
				auto* pInvoke = pMessage->invoke;
				if (bAllowAllInvokes || m_multithreadInvokeIDs.find(pInvoke->operationID) != m_multithreadInvokeIDs.end())
				{
					if (pInvoke->operationName && pInvoke->operationID == 0)
						pInvoke->operationID = SnaccRoseOperationLookup::LookUpID(pInvoke->operationName->getASCII().c_str());

					if (pInvoke->operationID || pInvoke->operationName)
						OnInvokeMessage(pMessage, ulMessageSize);

					bProcessed = true;
				}
			}
			break;
		case ROSEMessage::resultCid:
			if (pMessage->result)
			{
				OnResultMessage(pMessage->result, ulMessageSize);
				// do not intercept anything if pmessage->result->result == nullptr is in place, because the missing invokeID already takes this into account
				CompletePendingOperation(pMessage->result->invokeID, pMessage, ulMessageSize);
				return true;
			}
			break;
		case ROSEMessage::errorCid:
			if (pMessage->error)
			{
				OnErrorMessage(pMessage->error, ulMessageSize);
				// do not intercept anything if pmessage->error->error == nullptr is in place, because the missing invokeID already takes this into account
				CompletePendingOperation(pMessage->error->invokedID, pMessage, ulMessageSize);
				return true;
			}
			break;
		case ROSEMessage::rejectCid:
			if (pMessage->reject)
			{
				OnRejectMessage(pMessage->reject, ulMessageSize);
				if (pMessage->reject->invokedID.choiceId == ROSERejectChoice::invokedIDCid)
				{
					// Test! with REJECT the InvokeID is a choice and therefore the ID itself is a pointer! and it can become nullptr.
					if (pMessage->reject->invokedID.invokedID != nullptr)
					{
						CompletePendingOperation(*pMessage->reject->invokedID.invokedID, pMessage, ulMessageSize);
						return true;
					}
				}
				bProcessed = true;
			}
			break;
		default:
			break;
	}
	delete pMessage;
	return bProcessed;
}

long SnaccROSEBase::EncodeReject(SNACC::ROSEReject* preject, std::string& strResponse)
{
	long lRoseResult = ROSE_NOERROR;

	ROSEMessage rejectMsg;
	rejectMsg.choiceId = ROSEMessage::rejectCid;
	rejectMsg.reject = preject;

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

	// prevent delete of preject...
	rejectMsg.reject = 0;

	return lRoseResult;
}

long SnaccROSEBase::SendRejectEx(SNACC::ROSEReject* preject)
{
	std::string strResponse;
	auto lResult = EncodeReject(preject, strResponse);
	if (lResult)
		return lResult;

	auto ctx = SnaccInvokeContext::Create(SnaccInvokeContextInit(SnaccInvokeDirection::OUTBOUND, nullptr));

	return SendBinaryDataBlockEx(strResponse.c_str(), strResponse.length(), *ctx);
}

long SnaccROSEBase::GetJsonLengthPrefix(std::string_view strJson, std::string& strLenghtPrefix) const
{
	const auto len = strJson.length();
	if (len > 9999999)
		return ROSE_TE_ENCODE_FAILED;

	// Calc prefix z.B. (J1234567)
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

void SnaccROSEBase::OnInvokeMessage(SNACC::ROSEMessage* pMessage, unsigned long ulMessageSize)
{
	std::string strResponse;
	long lResult = ROSE_REJECT_UNKNOWNOPERATION;
	auto* pInvoke = pMessage->invoke;
	const char* szOperationName = SnaccRoseOperationLookup::LookUpName(pInvoke->operationID);

	SnaccInvokeContextInit init(SnaccInvokeDirection::INBOUND, pInvoke, szOperationName);
	auto pCtx = SnaccInvokeContext::Create(init);
	auto telemetry = SnaccTelemetryData::Create(SnaccTelemetryData::Direction::INBOUND, pInvoke->operationID, szOperationName, ulMessageSize);
	auto telemetryResult = SnaccTelemetryData::Outcome::UNHANDLED;
	auto telemetryReason = SnaccTelemetryData::Reason::UNKNOWN_FAILURE;
	bool bInvokeException = false;

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
		error["invokeID"] = (int)pInvoke->invokeID;
		std::string jsonString = getPrettyPrinted(error);
		PrintJSONToLog(false, true, nullptr, jsonString.c_str(), jsonString.length());
	}

	const bool bIsInvoke = (AsnIntType)pInvoke->invokeID != 99999;
	const bool bIsRejectResponse = lResult != ROSE_NOERROR && bIsInvoke;

	// if the Result is ROSE_NOERROR the request has been processed with a result or an error (the invoke context points out if it was replied with an error)
	// if the result is ROSE_REJECT_ASYNCOPERATION, the result will be sent async
	if (bIsRejectResponse)
	{
		lResult = EncodeInvokeRejectResponse(pInvoke, lResult, *pCtx, strResponse);
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

void SnaccROSEBase::OnResultMessage(SNACC::ROSEResult* presult, unsigned long ulMessageSize)
{
	unsigned int uiOperationID = 0;
	std::string strOperationName;
	GetPendingOperationTelemetryInfo(presult->invokeID, uiOperationID, strOperationName);
	OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, uiOperationID, strOperationName.c_str(), ulMessageSize, SnaccTelemetryData::Outcome::RESULT, SnaccTelemetryData::Stage::INBOUND_RESPONSE, SnaccTelemetryData::Reason::REMOTE_RESULT, ROSE_NOERROR));
}

void SnaccROSEBase::OnErrorMessage(SNACC::ROSEError* perror, unsigned long ulMessageSize)
{
	unsigned int uiOperationID = 0;
	std::string strOperationName;
	GetPendingOperationTelemetryInfo(perror->invokedID, uiOperationID, strOperationName);
	OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, uiOperationID, strOperationName.c_str(), ulMessageSize, SnaccTelemetryData::Outcome::ERR, SnaccTelemetryData::Stage::INBOUND_RESPONSE, SnaccTelemetryData::Reason::REMOTE_ERROR, ROSE_ERROR_VALUE));
}

void SnaccROSEBase::OnRejectMessage(SNACC::ROSEReject* preject, unsigned long ulMessageSize)
{
	unsigned int uiOperationID = 0;
	std::string strOperationName;
	if (preject->invokedID.choiceId == ROSERejectChoice::invokedIDCid && preject->invokedID.invokedID != nullptr)
		GetPendingOperationTelemetryInfo(*preject->invokedID.invokedID, uiOperationID, strOperationName);
	OnInvokeProcessed(SnaccTelemetryData::CreateFinalized(SnaccTelemetryData::Direction::INBOUND, uiOperationID, strOperationName.c_str(), ulMessageSize, SnaccTelemetryData::Outcome::REJECT, SnaccTelemetryData::Stage::INBOUND_RESPONSE, SnaccTelemetryData::Reason::REMOTE_REJECT, GetRejectResultCode(preject)));
}

long SnaccROSEBase::GetNextInvokeID()
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	m_lInvokeCounter++;
	if (m_lInvokeCounter >= 99998)
		m_lInvokeCounter = 1;

	return m_lInvokeCounter;
}

long SnaccROSEBase::Send(SNACC::ROSEInvoke* pInvoke, const char* szOperationName, SnaccInvokeContext& ctx, size_t* pstRequestData /*= nullptr*/)
{
	long lRoseResult = ROSE_NOERROR;
	if (pstRequestData)
		*pstRequestData = 0;

	ROSEMessage invokeMsg;
	invokeMsg.choiceId = ROSEMessage::invokeCid;
	invokeMsg.invoke = pInvoke;

	if (m_eTransportEncoding == SNACC::TransportEncoding::BER)
	{
		unsigned long ulSize;
		std::string strData = GetEncoded(m_eTransportEncoding, &invokeMsg, &ulSize);
		if (pstRequestData)
			*pstRequestData = ulSize;
		LogTransportData(true, m_eTransportEncoding, szOperationName, strData.c_str(), ulSize, &invokeMsg, nullptr);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), ulSize, ctx);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		// The mobiles currently rely on the operationName so we need to fill it if it is missing here
		if (!pInvoke->operationName)
			pInvoke->operationName = UTF8String::CreateNewFromASCII(szOperationName);

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
			lRoseResult = SendBinaryDataBlockEx(strData.c_str(), strData.length(), ctx);
		}
		else if (pstRequestData)
			*pstRequestData = strData.length();
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	// prevent autodelete of pInvoke
	invokeMsg.invoke = NULL;

	return lRoseResult;
}

long SnaccROSEBase::SendEvent(SNACC::ROSEInvoke* pinvoke, const char* szOperationName, std::shared_ptr<SnaccInvokeContext> pCtx /*= {}*/)
{
	const auto chronoCreated = std::chrono::steady_clock::now();
	const char* szResolvedOperationName = SnaccRoseOperationLookup::LookUpName(pinvoke->operationID);
	if (!pCtx)
	{
		SnaccInvokeContextInit init(SnaccInvokeDirection::OUTBOUND, pinvoke, szResolvedOperationName ? szResolvedOperationName : szOperationName);
		pCtx = SnaccInvokeContext::Create(init);
	}
	auto& ctx = *pCtx;
	size_t stRequestData = 0;
	const long lRoseResult = Send(pinvoke, szOperationName, ctx, &stRequestData);
	auto telemetry = SnaccTelemetryData::Create(SnaccTelemetryData::Direction::OUTBOUND, pinvoke->operationID, szResolvedOperationName, stRequestData, chronoCreated);
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

long SnaccROSEBase::SendInvoke(SNACC::ROSEInvoke* pinvoke, SNACC::AsnType* result, SNACC::AsnType* error, const char* szOperationName /*= nullptr*/, int iTimeout /*= -1*/, std::shared_ptr<SnaccInvokeContext> pCtx /*= {}*/)
{
	const auto chronoCreated = std::chrono::steady_clock::now();
	const char* szResolvedOperationName = SnaccRoseOperationLookup::LookUpName(pinvoke->operationID);

	// Ensure that we always have a ctx
	if (!pCtx)
	{
		SnaccInvokeContextInit init(SnaccInvokeDirection::OUTBOUND, pinvoke, szResolvedOperationName ? szResolvedOperationName : szOperationName);
		pCtx = SnaccInvokeContext::Create(init);
	}
	auto& ctx = *pCtx;

	auto& pendingOP = AddPendingOperation(pinvoke->invokeID, pinvoke->operationID, szResolvedOperationName);

	size_t stRequestData = 0;
	long lRoseResult = Send(pinvoke, szOperationName, ctx, &stRequestData);
	pendingOP.m_pTelemetry = SnaccTelemetryData::Create(SnaccTelemetryData::Direction::OUTBOUND, pinvoke->operationID, szResolvedOperationName, stRequestData, chronoCreated);

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
		lRoseResult = HandleInvokeResult(lRoseResult, pendingOP.m_pAnswerMessage, result, error, ctx);

	pendingOP.FinalizeTelemetry(pCtx);
	OnInvokeProcessed(pendingOP.m_pTelemetry);

	RemovePendingOperation(pendingOP.m_lInvokeID);

	return lRoseResult;
}

long SnaccROSEBase::DecodeResponse(const SNACC::ROSEMessage* pResponse, SNACC::ROSEResult** ppResult, SNACC::ROSEError** ppError, SnaccInvokeContext& ctx)
{
	long lRoseResult = ROSE_RE_INVALID_ANSWER;
	if (!pResponse)
		return lRoseResult;

	switch (pResponse->choiceId)
	{
		case ROSEMessage::notinitialized:
			throw std::runtime_error("response ROSEMessage choiceId is notinitialized");
		case ROSEMessage::invokeCid:
			break;
		case ROSEMessage::resultCid:
			lRoseResult = ROSE_NOERROR;
			if (pResponse->result && ppResult)
				*ppResult = pResponse->result;
			break;
		case ROSEMessage::errorCid:
			lRoseResult = ROSE_ERROR_VALUE;
			if (pResponse->error && ppError)
				*ppError = pResponse->error;
			break;
		case ROSEMessage::rejectCid:
			lRoseResult = GetRejectResultCode(pResponse->reject, ctx);
			break;
	}

	return lRoseResult;
}

long SnaccROSEBase::EncodeResult(unsigned int uiInvokeID, SNACC::AsnType* pResult, std::string& strResponse, const wchar_t* szSessionID /*= nullptr*/)
{
	long lRoseResult = ROSE_NOERROR;

	ROSEResult result;
	result.invokeID = uiInvokeID;
	result.result = new ROSEResultSeq;
	result.result->resultValue = 0;
	result.result->result.value = pResult;
	if (szSessionID)
		result.sessionID = new UTF8String(szSessionID);

	{
		ROSEMessage ResultMsg;
		ResultMsg.choiceId = ROSEMessage::resultCid;
		ResultMsg.result = &result;

		// encode now.
		if (m_eTransportEncoding == SNACC::TransportEncoding::BER)
		{
			AsnBuf OutBuf;
			AsnLen BytesEncoded = ResultMsg.BEnc(OutBuf);

			OutBuf.ResetMode();
			OutBuf.GetSeg(strResponse, BytesEncoded);

			LogTransportData(true, m_eTransportEncoding, nullptr, strResponse.c_str(), BytesEncoded, &ResultMsg, nullptr);
		}
		else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
		{
			auto value = ResultMsg.JEnc();

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
				LogTransportData(true, m_eTransportEncoding, nullptr, strResponse.c_str(), strResponse.length(), &ResultMsg, nullptr);
				if (!strPrefix.empty())
					strResponse.insert(0, strPrefix);
			}
		}
		else
		{
			throw std::runtime_error("invalid m_eTransportEncoding");
		}

		// prevent delete of presult...
		ResultMsg.result = nullptr;
	}

	// prevent delete of value...
	result.result->result.value = nullptr;

	return lRoseResult;
}

long SnaccROSEBase::EncodeError(unsigned int uiInvokeID, SNACC::AsnType* pError, std::string& strResponse, const wchar_t* szSessionID /*= nullptr*/)
{
	long lRoseResult = ROSE_NOERROR;

	ROSEError error;
	error.invokedID = uiInvokeID;
	error.error_value = 0;
	error.error = new AsnAny();
	error.error->value = pError;
	if (szSessionID)
		error.sessionID = new UTF8String(szSessionID);

	{
		ROSEMessage errorMsg;
		errorMsg.choiceId = ROSEMessage::errorCid;
		errorMsg.error = &error;

		// encode now.
		if (m_eTransportEncoding == SNACC::TransportEncoding::BER)
		{
			AsnBuf OutBuf;
			AsnLen BytesEncoded = errorMsg.BEnc(OutBuf);

			OutBuf.ResetMode();
			OutBuf.GetSeg(strResponse, BytesEncoded);

			LogTransportData(true, m_eTransportEncoding, nullptr, strResponse.c_str(), strResponse.length(), &errorMsg, nullptr);
		}
		else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
		{
			auto value = errorMsg.JEnc();

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
				LogTransportData(true, m_eTransportEncoding, nullptr, strResponse.c_str(), strResponse.length(), &errorMsg, nullptr);
				if (!strPrefix.empty())
					strResponse.insert(0, strPrefix);
			}
		}
		else
		{
			throw std::runtime_error("invalid m_eTransportEncoding");
		}
		// prevent delete of perror...
		errorMsg.error = nullptr;
	}

	// prevent delete of value...
	error.error->value = nullptr;

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

long SnaccROSEBase::HandleInvokeResult(long lRoseResult, const SNACC::ROSEMessage* pResponseMsg, SNACC::AsnType* result, SNACC::AsnType* error, SnaccInvokeContext& ctx)
{
	// In case of transport errors, we hand that value back as we have no response to parse
	if (ISROSE_TE(lRoseResult))
		return lRoseResult;

	SNACC::ROSEError* pError = nullptr;
	SNACC::ROSEResult* pResult = nullptr;
	lRoseResult = DecodeResponse(pResponseMsg, &pResult, &pError, ctx);

	if (lRoseResult == ROSE_NOERROR)
	{
		if (pResult && pResult->result && result)
		{
			AsnLen len;
			try
			{
				if (pResult->result->result.anyBuf)
				{
					result->BDec(*pResult->result->result.anyBuf, len);
					// Special to log the *full* ROSE Message in json
					// While for JSON Transport we can simply decode the full message on BER we need to decode the envelop and the payload
					//
					// We log the plain received BER payload in OnBinaryDataBlock and the fully decoded message here
					// To be able to do this we need to put the decoded payload (here result) into the pResponseMsg and then log it
					// We only do this if JSON logging is enabled
					int logLevel = (int)GetLogLevel(false);
					if (logLevel & ((int)SNACC::EAsnLogLevel::JSON | (int)SNACC::EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED))
					{
						// Backup the original response object inside the response message
						ROSEResultSeq* pOriginalResponse = pResponseMsg->result->result;

						// Set the decoded result object into the response to be able to fully log the whole message
						pResponseMsg->result->result = new ROSEResultSeq();
						pResponseMsg->result->result->result.value = result;
						LogTransportData(false, SNACC::TransportEncoding::BER, nullptr, nullptr, 0, pResponseMsg, nullptr);
						// As we hand back the result object to the outer world (function argument) we need to set it to NULL to prevent deletion if we discard the inserted object
						pResponseMsg->result->result->result.value = NULL;

						// Delete the inserted result object and reset to the original response object
						delete pResponseMsg->result->result;
						pResponseMsg->result->result = pOriginalResponse;
					}
				}
				else if (pResult->result->result.jsonBuf)
				{
					result->JDec(*pResult->result->result.jsonBuf);
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
		if (pError && pError->error && error)
		{
			AsnLen len;
			try
			{
				if (pError->error->anyBuf)
				{
					error->BDec(*pError->error->anyBuf, len);
					// Special to log the *full* ROSE Message in json
					// While for JSON Transport we can simply decode the full message on BER we need to decode the envelop and the payload
					//
					// We log the plain received BER payload in OnBinaryDataBlock and the fully decoded message here
					// To be able to do this we need to put the decoded payload (here error) into the pResponseMsg and then log it
					// We only do this if JSON logging is enabled
					int logLevel = (int)GetLogLevel(false);
					if (logLevel & ((int)SNACC::EAsnLogLevel::JSON | (int)SNACC::EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED))
					{
						// Backup the original error object inside the response message
						AsnAny* pOriginalError = pResponseMsg->error->error;

						// Set the decoded error object into the response to be able to fully log the whole message
						pResponseMsg->error->error = new AsnAny();
						pResponseMsg->error->error->value = error;
						LogTransportData(false, SNACC::TransportEncoding::BER, nullptr, nullptr, 0, pResponseMsg, nullptr);
						// As we hand back the error object to the outer world (function argument) we need to set it to NULL to prevent deletion if we discard the inserted object
						pResponseMsg->error->error->value = NULL;

						// Delete the inserted error object and reset to the original response object
						delete pResponseMsg->error->error;
						pResponseMsg->error->error = pOriginalError;
					}
				}
				else if (pError->error->jsonBuf)
				{
					error->JDec(*pError->error->jsonBuf);
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

long SnaccROSEBase::HandleOnInvokeResult(SNACC::InvokeResult invokeResult, const SNACC::ROSEInvoke* pInvoke, SnaccInvokeContext& ctx, std::string& strResponse, SNACC::AsnType* pResult, SNACC::AsnType* pError)
{
	if (pInvoke && (AsnIntType)pInvoke->invokeID == 99999)
	{
		strResponse.clear();
		ctx.m_bResponseIsError = false;
		return ROSE_NOERROR;
	}

	switch (invokeResult)
	{
		case InvokeResult::returnResult:
			return EncodeResult(pInvoke->invokeID, pResult, strResponse);
		case InvokeResult::returnError:
			ctx.m_bResponseIsError = true;
			return EncodeError(pInvoke->invokeID, pError, strResponse);
		default:
			return ctx.m_lRejectResult ? ctx.m_lRejectResult : ROSE_REJECT_FUNCTIONMISSING;
	}
}

long SnaccROSEBase::DecodeInvoke(const SNACC::ROSEMessage* pMessage, SNACC::AsnType* argument)
{
	if (!pMessage || !pMessage->invoke)
		return ROSE_RE_DECODE_FAILED;

	auto* pInvoke = pMessage->invoke;
	if (!pInvoke->argument)
	{
		if (argument->mayBeEmpty())
			return ROSE_NOERROR;
		else
			return ROSE_REJECT_ARGUMENT_MISSING;
	}

	long lRoseResult = ROSE_NOERROR;

	try
	{
		if (pInvoke->argument->anyBuf)
		{
			AsnLen len = 0;
			argument->BDec(*pInvoke->argument->anyBuf, len);
			// Special to log the *full* ROSE Message in json
			// While for JSON Transport we can simply decode the full message on BER we need to decode the envelop and the payload
			//
			// We log the plain received BER payload in OnBinaryDataBlock and the fully decoded message here
			// To be able to do this we need to put the decoded payload (here result) into the pResponseMsg and then log it
			// We only do this if JSON logging is enabled
			int logLevel = (int)GetLogLevel(false);
			if (logLevel & ((int)SNACC::EAsnLogLevel::JSON | (int)SNACC::EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED))
			{
				// Backup the original response object inside the response message
				AsnAny* pOriginalArgument = pInvoke->argument;

				// Set the decoded result object into the response to be able to fully log the whole message
				pInvoke->argument = new AsnAny();
				pInvoke->argument->value = argument;

				// Get the name of the called operation for logging
				std::string strOperationName;
				const char* szOperationName = nullptr;
				if (pInvoke->operationName)
				{
					strOperationName = pInvoke->operationName->getUTF8();
					szOperationName = strOperationName.c_str();
				}
				if (!szOperationName)
					szOperationName = SnaccRoseOperationLookup::LookUpName(pInvoke->operationID);
				LogTransportData(false, SNACC::TransportEncoding::BER, szOperationName, nullptr, 0, pMessage, nullptr);
				// As we hand back the result object to the outer world (function argument) we need to set it to NULL to prevent deletion if we discard the inserted object
				pInvoke->argument->value = NULL;

				// Delete the inserted result object and reset to the original response object
				delete pInvoke->argument;
				pInvoke->argument = pOriginalArgument;
			}
		}
		else if (pInvoke->argument->jsonBuf)
		{
			argument->JDec(*pInvoke->argument->jsonBuf);
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
