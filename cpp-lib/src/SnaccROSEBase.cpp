#include "../include/SnaccROSEBase.h"
#include "../include/SNACCROSE.h"
#include <assert.h>
#include <iomanip>
#include <locale>
#include <chrono>

using namespace SNACC;

SnaccInvokeContext::SnaccInvokeContext()
{
	pInvokeAuth = nullptr;
	pRejectAuth = nullptr;
	lRejectResult = 0;
	pInvoke = nullptr;
	pCustom = nullptr;
}

SnaccInvokeContext::~SnaccInvokeContext()
{
	if (pInvoke)
		delete pInvoke;
	if (pInvokeAuth)
		delete pInvokeAuth;
	if (pRejectAuth)
		delete pRejectAuth;
	if (pCustom)
	{
		// If you set data in pCustom ensure that it is deleted (freeed) before the object runs out of scope...
		assert(0);
	}
}

std::map<std::string, int> SnaccRoseOperationLookup::m_mapOpToID;
std::map<int, std::string> SnaccRoseOperationLookup::m_mapIDToOp;
std::map<int, int> SnaccRoseOperationLookup::m_mapIDToInterface;

std::string getPrettyPrinted(const SJson::Value& value)
{
	SJson::StreamWriterBuilder wbuilder;
	wbuilder["indentation"] = "\t";
	return SJson::writeString(wbuilder, value);
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

void SnaccRoseOperationLookup::RegisterOperation(int iOpID, const char* szOpName, int iInterfaceID)
{
#ifdef _DEBUG
	if (m_mapIDToOp.find(iOpID) != m_mapIDToOp.end())
	{
		// In case we land here we have two operations using the same operationID
		assert(0);
	}
#endif

	m_mapOpToID[szOpName] = iOpID;
	m_mapIDToOp[iOpID] = szOpName;
	m_mapIDToInterface[iOpID] = iInterfaceID;
}

int SnaccRoseOperationLookup::LookUpInterfaceID(int iOpID)
{
	const auto it = m_mapIDToInterface.find(iOpID);
	if (it != m_mapIDToInterface.end())
		return it->second;

	return 0;
}

const char* SnaccRoseOperationLookup::LookUpName(int iOpID)
{
	const auto it = m_mapIDToOp.find(iOpID);
	if (it != m_mapIDToOp.end())
		return it->second.c_str();

#ifdef _DEBUG
	// This may only happen if the other side calls a method we are not aware of
	// We handle it here as assert as it should not happen in development
	assert(0);
#endif

	return nullptr;
}

int SnaccRoseOperationLookup::LookUpID(const char* szOpName)
{
	const auto it = m_mapOpToID.find(szOpName);
	if (it != m_mapOpToID.end())
		return it->second;

	return 0;
}

SnaccROSEPendingOperation::SnaccROSEPendingOperation()
{
	m_pAnswerMessage = 0;
	m_lInvokeID = 0;
	m_lRoseResult = 0;
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

void SnaccROSEPendingOperation::CompleteOperation(long lRoseResult, SNACC::ROSEMessage* pAnswerMessage)
{
	m_lRoseResult = lRoseResult;
	if (pAnswerMessage)
		m_pAnswerMessage = pAnswerMessage;
	m_CompletedEvent.signal();
}

SnaccROSEBase::SnaccROSEBase(void)
{
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

void SnaccROSEBase::AddPendingOperation(int invokeID, SnaccROSEPendingOperation* pOperation)
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	m_PendingOperations.insert(SnaccROSEPendingOperationPair(invokeID, pOperation));
}

void SnaccROSEBase::RemovePendingOperation(int invokeID)
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	const auto it = m_PendingOperations.find(invokeID);
	if (it != m_PendingOperations.end())
		m_PendingOperations.erase(it);
}

bool SnaccROSEBase::CompletePendingOperation(int invokeID, SNACC::ROSEMessage* pMessage)
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	const auto it = m_PendingOperations.find(invokeID);
	if (it != m_PendingOperations.end())
	{
		// found...
		it->second->CompleteOperation(ROSE_NOERROR, pMessage);
		return true;
	}
	else
	{
		delete pMessage;
	}

	return false;
}

bool SnaccROSEBase::OnBinaryDataBlockResult(const char* lpBytes, unsigned long lSize)
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
						LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

						SJson::Value error;
						error["exception"] = ex.what();
						error["method"] = __FUNCTION__;
						error["error"] = (int)ex.m_errorCode;
						std::string strError = getPrettyPrinted(error);
						PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

						if (pmessage->choiceId == ROSEMessage::invokeCid && pmessage->invoke)
						{
							ROSEReject reject;
							if ((AsnIntType)pmessage->invoke->invokeID)
							{
								reject.invokedID.choiceId = ROSERejectChoice::invokedIDCid;
								reject.invokedID.invokedID = new AsnInt(pmessage->invoke->invokeID);
							}
							else
							{
								reject.invokedID.choiceId = ROSERejectChoice::invokednullCid;
								reject.invokedID.invokednull = new AsnNull;
							}

							SendReject(&reject);
						}
						delete pmessage;
						return true;
					}
					LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

					// pmessage will be deleted inside
					bReturn = OnROSEMessage(pmessage, false);
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
							LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

							SJson::Value error;
							error["exception"] = ex.what();
							error["method"] = __FUNCTION__;
							error["error"] = (int)ex.m_errorCode;
							std::string strError = getPrettyPrinted(error);
							PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

							if (pmessage->choiceId == ROSEMessage::invokeCid && pmessage->invoke)
							{
								ROSEReject reject;
								if ((AsnIntType)pmessage->invoke->invokeID)
								{
									reject.invokedID.choiceId = ROSERejectChoice::invokedIDCid;
									reject.invokedID.invokedID = new AsnInt(pmessage->invoke->invokeID);
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
								SendReject(&reject);
							}
							delete pmessage;
							return true;
						}

						LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, pmessage, &value);

						// pmessage will be deleted inside
						bReturn = OnROSEMessage(pmessage, false);
					}
					else
					{
						LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

						SJson::Value error;
						error["exception"] = reader.getFormattedErrorMessages();
						error["where"] = __FUNCTION__;
						std::string strError = getPrettyPrinted(error);
						PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());
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
				SJson::Value value;
				pValue->JEnc(value);
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
			assert(false);
			throw std::runtime_error("invalid encoding");
			break;
	}
	return strData;
}

void SnaccROSEBase::OnBinaryDataBlock(const char* lpBytes, unsigned long lSize)
{
	if (!lSize)
		return;

	try
	{
		unsigned char byFirst = (unsigned char)*lpBytes;

		if (byFirst == 0xA1 || byFirst == 0xA2 || byFirst == 0xA3 || byFirst == 0xA4)
		{
			m_eTransportEncoding = SNACC::TransportEncoding::BER;
			AsnBuf buffer((const char*)lpBytes, lSize);
			ROSEMessage* pmessage = new ROSEMessage;
			AsnLen bytesDecoded = 0;
			try
			{
				pmessage->BDec(buffer, bytesDecoded);
			}
			catch (const SnaccException& ex)
			{
				LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

				SJson::Value error;
				error["exception"] = ex.what();
				error["method"] = __FUNCTION__;
				error["error"] = (int)ex.m_errorCode;
				std::string strError = getPrettyPrinted(error);
				PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

				OnRoseDecodeError(lpBytes, lSize, ex.what());

				if (pmessage->choiceId == ROSEMessage::invokeCid && pmessage->invoke)
				{
					ROSEReject reject;
					if ((AsnIntType)pmessage->invoke->invokeID)
					{
						reject.invokedID.choiceId = ROSERejectChoice::invokedIDCid;
						reject.invokedID.invokedID = new AsnInt(pmessage->invoke->invokeID);
					}
					else
					{
						reject.invokedID.choiceId = ROSERejectChoice::invokednullCid;
						reject.invokedID.invokednull = new AsnNull;
					}
					SendReject(&reject);
				}
				delete pmessage;
				return;
			}
			LogTransportData(false, SNACC::TransportEncoding::BER, nullptr, lpBytes, lSize, nullptr, nullptr);

			// pmessage will be deleted inside
			OnROSEMessage(pmessage, true);
		}
		else if (byFirst == 'J' || byFirst == '{' || byFirst == '[')
		{
			int iHeaderLen = 0;
			if (byFirst == 'J')
			{
				m_eTransportEncoding = SNACC::TransportEncoding::JSON;
				iHeaderLen = GetJsonHeaderLen(lpBytes, lSize);
				lpBytes += iHeaderLen;
				lSize -= iHeaderLen;
			}
			else
				m_eTransportEncoding = SNACC::TransportEncoding::JSON_NO_HEADING;

			SJson::Value value;
			SJson::Reader reader;
			if (reader.parse((const char*)lpBytes, (const char*)lpBytes + lSize, value))
			{
				ROSEMessage* pmessage = new ROSEMessage;
				try
				{
					if (!pmessage->JDec(value))
						throw InvalidTagException("ROSEMessage", "decode failed: ROSEMessage", STACK_ENTRY);
				}
				catch (const SnaccException& ex)
				{
					LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

					SJson::Value error;
					error["exception"] = ex.what();
					error["method"] = __FUNCTION__;
					error["error"] = (int)ex.m_errorCode;
					std::string strError = getPrettyPrinted(error);
					PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

					OnRoseDecodeError(lpBytes, lSize, ex.what());

					if (pmessage->choiceId == ROSEMessage::invokeCid && pmessage->invoke)
					{
						ROSEReject reject;
						if ((AsnIntType)pmessage->invoke->invokeID)
						{
							reject.invokedID.choiceId = ROSERejectChoice::invokedIDCid;
							reject.invokedID.invokedID = new AsnInt(pmessage->invoke->invokeID);
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
						SendReject(&reject);
					}
					delete pmessage;
					return;
				}
				LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, pmessage, &value);

				// pmessage will be deleted inside
				OnROSEMessage(pmessage, true);
			}
			else
			{
				LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

				SJson::Value error;
				error["exception"] = reader.getFormattedErrorMessages();
				error["method"] = __FUNCTION__;
				std::string strError = getPrettyPrinted(error);
				PrintJSONToLog(false, true, nullptr, strError.c_str(), strError.length());

				OnRoseDecodeError(lpBytes, lSize, reader.getFormattedErrorMessages());

				ROSEReject reject;
				reject.invokedID.choiceId = ROSERejectChoice::invokednullCid;
				reject.invokedID.invokednull = new AsnNull;

				reject.reject = new RejectProblem;
				reject.reject->choiceId = RejectProblem::invokeProblemCid;
				reject.reject->invokeProblem = new InvokeProblem;
				*reject.reject->invokeProblem = InvokeProblem::mistypedArgument;
				reject.details = UTF8String::CreateNewFromASCII(strError.c_str());
				SendReject(&reject);
				return;
			}
		}
		else
		{
			LogTransportData(false, m_eTransportEncoding, nullptr, lpBytes, lSize, nullptr, nullptr);

			OnRoseDecodeError(lpBytes, lSize, "unknown encoding");
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
	}
}

bool SnaccROSEBase::OnROSEMessage(SNACC::ROSEMessage* pmessage, bool bAllowAllInvokes)
{
	bool bProcessed = false;
	switch (pmessage->choiceId)
	{
		case ROSEMessage::invokeCid:
			if (pmessage->invoke)
			{
				if (bAllowAllInvokes || m_mapMultithreadInvokeIDs.find(pmessage->invoke->operationID) != m_mapMultithreadInvokeIDs.end())
				{
					if (pmessage->invoke->operationID != 0 || pmessage->invoke->operationName != nullptr)
						OnInvokeMessage(pmessage);
					bProcessed = true;
				}
			}
			break;
		case ROSEMessage::resultCid:
			if (pmessage->result)
			{
				OnResultMessage(pmessage->result);
				// do not intercept anything if pmessage->result->result == nullptr is in place, because the missing invokeID already takes this into account
				CompletePendingOperation(pmessage->result->invokeID, pmessage);
				return true;
			}
			break;
		case ROSEMessage::errorCid:
			if (pmessage->error)
			{
				OnErrorMessage(pmessage->error);
				// do not intercept anything if pmessage->error->error == nullptr is in place, because the missing invokeID already takes this into account
				CompletePendingOperation(pmessage->error->invokedID, pmessage);
				return true;
			}
			break;
		case ROSEMessage::rejectCid:
			if (pmessage->reject)
			{
				OnRejectMessage(pmessage->reject);
				if (pmessage->reject->invokedID.choiceId == ROSERejectChoice::invokedIDCid)
				{
					// Test! with REJECT the InvokeID is a choice and therefore the ID itself is a pointer! and it can become nullptr.
					if (pmessage->reject->invokedID.invokedID != nullptr)
					{
						CompletePendingOperation(*pmessage->reject->invokedID.invokedID, pmessage);
						return true;
					}
				}
				bProcessed = true;
			}
			break;
		default:
			break;
	}
	delete pmessage;
	return bProcessed;
}

long SnaccROSEBase::SendReject(SNACC::ROSEReject* preject)
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

		std::string strData;
		OutBuf.ResetMode();
		OutBuf.GetSeg(strData, BytesEncoded);

		LogTransportData(true, m_eTransportEncoding, nullptr, strData.c_str(), strData.length(), &rejectMsg, nullptr);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), BytesEncoded, nullptr);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		SJson::Value value;
		rejectMsg.JEnc(value);

		std::string strData;
		int logLevel = (int)GetLogLevel(true);
		if (logLevel & (int)EAsnLogLevel::JSON || logLevel & (int)EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED)
			strData = getPrettyPrinted(value);
		else
		{
			SJson::FastWriter writer;
			strData = writer.write(value);
		}

		LogTransportData(true, m_eTransportEncoding, nullptr, strData.c_str(), strData.length(), &rejectMsg, nullptr);

		if (m_eTransportEncoding == SNACC::TransportEncoding::JSON)
			strData = GetJsonAsnPrefix(strData) + strData;

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), (unsigned long)strData.length(), nullptr);
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	// prevent delete of preject...
	rejectMsg.reject = 0;

	return lRoseResult;
}

std::string SnaccROSEBase::GetJsonAsnPrefix(std::string& strJson)
{
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

	return std::string((char*)szPrefix);
}

long SnaccROSEBase::SendRejectInvoke(int invokeID, SNACC::InvokeProblem problem, const char* szError /*=NULL*/, const wchar_t* szSessionID /*= 0*/, SNACC::ROSEAuthResult* pAuthHeader /*=0*/)
{
	if (invokeID == 99999)
	{
		// Events do not get an answer
		return 0;
	}
	ROSEReject reject;
	reject.invokedID.choiceId = ROSERejectChoice::invokedIDCid;
	reject.invokedID.invokedID = new AsnInt;
	*reject.invokedID.invokedID = invokeID;
	if (szSessionID)
		reject.sessionID = new UTF8String(szSessionID);
	reject.reject = new RejectProblem;
	reject.reject->choiceId = RejectProblem::invokeProblemCid;
	reject.reject->invokeProblem = new InvokeProblem;
	*reject.reject->invokeProblem = problem;

	if (pAuthHeader)
		reject.authentication = (SNACC::ROSEAuthResult*)pAuthHeader->Clone();

	if ((m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING) && szError != NULL)
		reject.details = UTF8String::CreateNewFromUTF8(szError);
	return SendReject(&reject);
}

void SnaccROSEBase::OnInvokeMessage(SNACC::ROSEMessage* pMsg)
{
	long lRoseResult = ROSE_REJECT_UNKNOWNOPERATION;

	ROSEInvoke* pInvoke = pMsg->invoke;

	SnaccInvokeContext invokeContext;
	invokeContext.pInvokeAuth = pInvoke->authentication;
	invokeContext.pInvoke = pInvoke;

	bool bNotifyRunningOutOfScope = OnInvokeContextCreated(&invokeContext);

	try
	{
		if (pInvoke->operationName && pInvoke->operationID == 0)
			pInvoke->operationID = SnaccRoseOperationLookup::LookUpID(pInvoke->operationName->getASCII().c_str());

		lRoseResult = OnInvoke(pMsg, &invokeContext);
	}
	catch (const SnaccException& ex)
	{
		// decode of invoke detail failed.
		lRoseResult = ROSE_REJECT_MISTYPEDARGUMENT;

		SJson::Value error;
		error["exception"] = ex.what();
		error["method"] = __FUNCTION__;
		error["error"] = (int)ex.m_errorCode;
		error["invokeID"] = (int)pInvoke->invokeID;
		std::string jsonString = getPrettyPrinted(error);
		PrintJSONToLog(false, true, nullptr, jsonString.c_str(), jsonString.length());
	}

	// if the Result is ROSE_NOERROR, then the Result has been sent.
	// if the result is ROSE_REJECT_ASYNCOPERATION, the result will be sent async
	if (lRoseResult != ROSE_NOERROR && lRoseResult != ROSE_REJECT_ASYNCOPERATION)
	{
		// Send reject
		if (lRoseResult == ROSE_REJECT_UNKNOWNOPERATION)
			SendRejectInvoke(pInvoke->invokeID, InvokeProblem::unrecognisedOperation, "unrecognisedOperation", pInvoke->sessionID ? pInvoke->sessionID->c_str() : 0);
		else if (lRoseResult == ROSE_REJECT_MISTYPEDARGUMENT)
			SendRejectInvoke(pInvoke->invokeID, InvokeProblem::mistypedArgument, "mistypedArgument", pInvoke->sessionID ? pInvoke->sessionID->c_str() : 0);
		else if (lRoseResult == ROSE_REJECT_FUNCTIONMISSING)
			SendRejectInvoke(pInvoke->invokeID, InvokeProblem::resourceLimitation, "functionMissing", pInvoke->sessionID ? pInvoke->sessionID->c_str() : 0);
		else if (lRoseResult == ROSE_REJECT_INVALIDSESSIONID)
			SendRejectInvoke(pInvoke->invokeID, InvokeProblem::invalidSessionID, "invalidSessionID", pInvoke->sessionID ? pInvoke->sessionID->c_str() : 0);
		else if (lRoseResult == ROSE_REJECT_AUTHENTICATIONFAILED)
			SendRejectInvoke(pInvoke->invokeID, InvokeProblem::authenticationFailed, "authenticationFailed", pInvoke->sessionID ? pInvoke->sessionID->c_str() : 0, invokeContext.pRejectAuth);
		else if (lRoseResult == ROSE_REJECT_AUTHENTICATIONINCOMPLETE)
			SendRejectInvoke(pInvoke->invokeID, InvokeProblem::authenticationIncomplete, "authenticationIncomplete", pInvoke->sessionID ? pInvoke->sessionID->c_str() : 0, invokeContext.pRejectAuth);
		else if (lRoseResult == ROSE_REJECT_AUTHENTICATION_USER_TEMPORARY_LOCKED_OUT)
			SendRejectInvoke(pInvoke->invokeID, InvokeProblem::authenticationFailed, "temporaryLockedOut", pInvoke->sessionID ? pInvoke->sessionID->c_str() : 0, invokeContext.pRejectAuth);
		else if (lRoseResult == ROSE_REJECT_AUTHENTICATION_USER_LOCKED_OUT)
			SendRejectInvoke(pInvoke->invokeID, InvokeProblem::authenticationFailed, "lockedOut", pInvoke->sessionID ? pInvoke->sessionID->c_str() : 0, invokeContext.pRejectAuth);
		else if (lRoseResult == ROSE_REJECT_AUTHENTICATION_SERVER_BUSY)
			SendRejectInvoke(pInvoke->invokeID, InvokeProblem::authenticationFailed, "serverBusy", pInvoke->sessionID ? pInvoke->sessionID->c_str() : 0, invokeContext.pRejectAuth);
		else if (lRoseResult == ROSE_REJECT_ARGUMENT_MISSING)
			SendRejectInvoke(pInvoke->invokeID, InvokeProblem::mistypedArgument, "argumentMissing", pInvoke->sessionID ? pInvoke->sessionID->c_str() : 0);
		else
			SendRejectInvoke(pInvoke->invokeID, InvokeProblem::unrecognisedOperation, "otherError", pInvoke->sessionID ? pInvoke->sessionID->c_str() : 0);
	}
	if (invokeContext.funcAfterResult)
		invokeContext.funcAfterResult();

	if (bNotifyRunningOutOfScope)
		OnInvokeContextRunsOutOfScope(&invokeContext);

	// prevent autodelete
	invokeContext.pInvokeAuth = NULL;
	invokeContext.pInvoke = NULL;
}

void SnaccROSEBase::OnResultMessage(SNACC::ROSEResult* presult)
{
}

void SnaccROSEBase::OnErrorMessage(SNACC::ROSEError* perror)
{
}

void SnaccROSEBase::OnRejectMessage(SNACC::ROSEReject* preject)
{
}

long SnaccROSEBase::GetNextInvokeID()
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	m_lInvokeCounter++;
	if (m_lInvokeCounter >= 99998)
		m_lInvokeCounter = 1;

	return m_lInvokeCounter;
}

long SnaccROSEBase::Send(SNACC::ROSEInvoke* pInvoke, const char* szOperationName, SnaccInvokeContext* pCtx /*= nullptr*/)
{
	long lRoseResult = ROSE_NOERROR;

	ROSEMessage invokeMsg;
	invokeMsg.choiceId = ROSEMessage::invokeCid;
	invokeMsg.invoke = pInvoke;

	if (m_eTransportEncoding == SNACC::TransportEncoding::BER)
	{
		unsigned long ulSize;
		std::string strData = GetEncoded(m_eTransportEncoding, &invokeMsg, &ulSize);
		LogTransportData(true, m_eTransportEncoding, szOperationName, strData.c_str(), ulSize, &invokeMsg, nullptr);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), ulSize, pCtx);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		// The mobiles currently rely on the operationName so we need to fill it if it is missing here
		if (!pInvoke->operationName)
			pInvoke->operationName = UTF8String::CreateNewFromASCII(szOperationName);

		std::string strData = GetEncoded(m_eTransportEncoding, &invokeMsg);
		LogTransportData(true, m_eTransportEncoding, szOperationName, strData.c_str(), strData.length(), &invokeMsg, nullptr);

		if (m_eTransportEncoding == SNACC::TransportEncoding::JSON)
			strData = GetJsonAsnPrefix(strData) + strData;

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), (unsigned long)strData.length(), pCtx);
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	// prevent autodelete of pInvoke
	invokeMsg.invoke = NULL;

	return lRoseResult;
}

long SnaccROSEBase::SendEvent(SNACC::ROSEInvoke* pinvoke, const char* szOperationName, SnaccInvokeContext * pCtx /*= nullptr*/)
{
	return Send(pinvoke, szOperationName, pCtx);
}

void SnaccROSEBase::LogTransportData(const bool bOutbound, const SNACC::TransportEncoding encoding, const char* szOperationName, const char* szData, const size_t size, const SNACC::ROSEMessage* pMsg, const SJson::Value* pParsedValue)
{
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
				{
					strOperationName = SnaccRoseOperationLookup::LookUpName(pMsg->invoke->operationID);
					if (!strOperationName.empty())
						szOperationName = strOperationName.c_str();
				}
			}

			// JSON logging is requested
			if (encoding == SNACC::TransportEncoding::JSON || encoding == SNACC::TransportEncoding::JSON_NO_HEADING)
			{
				if (strlen(szData))
				{
					// The payload is already json -> we can directly log it
					// in Case inbound data shall get pretty printed always we need to check if the payload is alrady pretty printed
					if (pParsedValue && (level & (int)EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED) && strstr(szData, "\n") == nullptr)
					{
						std::string strLogData = getPrettyPrinted(*pParsedValue);
						PrintJSONToLog(bOutbound, false, szOperationName, strLogData.c_str(), strLogData.length());
					}
					else
					{
						PrintJSONToLog(bOutbound, false, szOperationName, szData, size);
					}
				}
			}
			else if (pMsg)
			{
				// The strTransportData is not already JSON encoded...
				// So we encode the ROSEMessage as JSON
				SJson::Value value;
				pMsg->JEnc(value);
				std::string strLogData = getPrettyPrinted(value);
				PrintJSONToLog(bOutbound, false, szOperationName, strLogData.c_str(), strLogData.length());
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
				PrintJSONToLog(bOutbound, false, nullptr, strLogData.c_str(), strLogData.length());
			}
		}
	}
}

long SnaccROSEBase::SendInvoke(SNACC::ROSEInvoke* pinvoke, SNACC::ROSEMessage** pResponse, const char* szOperationName = nullptr, int iTimeout /*= -1*/, SnaccInvokeContext* pCtx /*= nullptr*/)
{
	SnaccROSEPendingOperation* pendingOP = new SnaccROSEPendingOperation;
	pendingOP->m_lInvokeID = pinvoke->invokeID;
	AddPendingOperation(pendingOP->m_lInvokeID, pendingOP);

	long lRoseResult = Send(pinvoke, szOperationName, pCtx);

	if (lRoseResult == 0)
	{
		// Wait for Answer...
		if (iTimeout == -1)
			iTimeout = m_lMaxInvokeWait;

		if (iTimeout)
		{
			if (pendingOP->WaitForComplete(iTimeout))
			{
				lRoseResult = pendingOP->m_lRoseResult;
				if (pendingOP->m_pAnswerMessage)
				{
					*pResponse = pendingOP->m_pAnswerMessage;
					pendingOP->m_pAnswerMessage = nullptr;
					lRoseResult = ROSE_NOERROR;
				}
			}
			else
			{
				// not completed
				lRoseResult = ROSE_TE_TIMEOUT;
			}
		}
		else
		{
			lRoseResult = ROSE_NOERROR;
		}
	}
	else
	{
		lRoseResult = ROSE_TE_TRANSPORTFAILED;
	}
	RemovePendingOperation(pendingOP->m_lInvokeID);
	delete pendingOP;

	return lRoseResult;
}

long SnaccROSEBase::DecodeResponse(const SNACC::ROSEMessage* pResponse, SNACC::ROSEResult** ppResult, SNACC::ROSEError** ppError, SnaccInvokeContext* pCtx)
{
	long lRoseResult = ROSE_RE_INVALID_ANSWER;
	if (!pResponse)
		return lRoseResult;

	switch (pResponse->choiceId)
	{
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
			lRoseResult = ROSE_REJECT_UNKNOWN;
			if (pResponse->reject && pResponse->reject->reject)
			{
				if (pResponse->reject->reject->choiceId == RejectProblem::invokeProblemCid)
				{
					if (*pResponse->reject->reject->invokeProblem == InvokeProblem::unrecognisedOperation)
						lRoseResult = ROSE_REJECT_UNKNOWNOPERATION;
					else if (*pResponse->reject->reject->invokeProblem == InvokeProblem::mistypedArgument)
						lRoseResult = ROSE_REJECT_MISTYPEDARGUMENT;
					else if (*pResponse->reject->reject->invokeProblem == InvokeProblem::resourceLimitation)
						lRoseResult = ROSE_REJECT_FUNCTIONMISSING;
					else if (*pResponse->reject->reject->invokeProblem == InvokeProblem::authenticationIncomplete)
					{
						lRoseResult = ROSE_REJECT_AUTHENTICATIONINCOMPLETE;
						if (pCtx)
						{
							if (pResponse->reject->authentication)
								pCtx->pRejectAuth = (SNACC::ROSEAuthResult*)pResponse->reject->authentication->Clone();
							pCtx->lRejectResult = ROSE_REJECT_AUTHENTICATIONINCOMPLETE;
						}
					}
					else if (*pResponse->reject->reject->invokeProblem == InvokeProblem::authenticationFailed)
					{
						lRoseResult = ROSE_REJECT_AUTHENTICATIONFAILED;
						if (pCtx)
						{
							if (pResponse->reject->authentication)
								pCtx->pRejectAuth = (SNACC::ROSEAuthResult*)pResponse->reject->authentication->Clone();
							pCtx->lRejectResult = ROSE_REJECT_AUTHENTICATIONFAILED;
						}
					}
				}
			}
			break;
	}

	return lRoseResult;
}

long SnaccROSEBase::SendResult(SNACC::ROSEResult* presult)
{
	long lRoseResult = ROSE_NOERROR;

	ROSEMessage ResultMsg;
	ResultMsg.choiceId = ROSEMessage::resultCid;
	ResultMsg.result = presult;

	// encode now.
	if (m_eTransportEncoding == SNACC::TransportEncoding::BER)
	{
		AsnBuf OutBuf;
		AsnLen BytesEncoded = ResultMsg.BEnc(OutBuf);

		std::string strData;
		OutBuf.ResetMode();
		OutBuf.GetSeg(strData, BytesEncoded);

		LogTransportData(true, m_eTransportEncoding, nullptr, strData.c_str(), BytesEncoded, &ResultMsg, nullptr);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), BytesEncoded, nullptr);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		SJson::Value value;
		ResultMsg.JEnc(value);

		std::string strData;
		int logLevel = (int)GetLogLevel(true);
		if (logLevel & (int)EAsnLogLevel::JSON || logLevel & (int)EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED)
			strData = getPrettyPrinted(value);
		else
		{
			SJson::FastWriter writer;
			strData = writer.write(value);
		}

		LogTransportData(true, m_eTransportEncoding, nullptr, strData.c_str(), strData.length(), &ResultMsg, nullptr);

		if (m_eTransportEncoding == SNACC::TransportEncoding::JSON)
			strData = GetJsonAsnPrefix(strData) + strData;

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), (unsigned long)strData.length(), nullptr);
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	// prevent delete of presult...
	ResultMsg.result = 0;

	return lRoseResult;
}

long SnaccROSEBase::SendResult(const SNACC::ROSEInvoke* pInvoke, SNACC::AsnType* pResult, const wchar_t* szSessionID /* = 0 */)
{
	long lRoseResult = ROSE_NOERROR;

	ROSEResult result;
	result.invokeID = pInvoke->invokeID;
	result.result = new ROSEResultSeq;
	result.result->resultValue = 0;
	result.result->result.value = pResult;
	if (szSessionID)
		result.sessionID = new UTF8String(szSessionID);

	lRoseResult = SendResult(&result);

	// prevent delete of value...
	result.result->result.value = 0;

	return lRoseResult;
}

long SnaccROSEBase::SendError(SNACC::ROSEError* perror)
{
	long lRoseResult = ROSE_NOERROR;

	ROSEMessage errorMsg;
	errorMsg.choiceId = ROSEMessage::errorCid;
	errorMsg.error = perror;

	// encode now.
	if (m_eTransportEncoding == SNACC::TransportEncoding::BER)
	{
		AsnBuf OutBuf;
		AsnLen BytesEncoded = errorMsg.BEnc(OutBuf);

		std::string strData;
		OutBuf.ResetMode();
		OutBuf.GetSeg(strData, BytesEncoded);

		LogTransportData(true, m_eTransportEncoding, nullptr, strData.c_str(), strData.length(), &errorMsg, nullptr);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), BytesEncoded, nullptr);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		SJson::Value value;
		errorMsg.JEnc(value);

		std::string strData;
		int logLevel = (int)GetLogLevel(true);
		if (logLevel & (int)EAsnLogLevel::JSON || logLevel & (int)EAsnLogLevel::JSON_ALWAYS_PRETTY_PRINTED)
			strData = getPrettyPrinted(value);
		else
		{
			SJson::FastWriter writer;
			strData = writer.write(value);
		}

		LogTransportData(true, m_eTransportEncoding, nullptr, strData.c_str(), strData.length(), &errorMsg, nullptr);

		if (m_eTransportEncoding == SNACC::TransportEncoding::JSON)
			strData = GetJsonAsnPrefix(strData) + strData;
		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), (unsigned long)strData.length(), nullptr);
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	// prevent delete of perror...
	errorMsg.error = 0;

	return lRoseResult;
}

long SnaccROSEBase::SendError(const SNACC::ROSEInvoke* pInvoke, SNACC::AsnType* pError, const wchar_t* szSessionID /* = 0 */)
{
	long lRoseResult = ROSE_NOERROR;

	ROSEError error;
	error.invokedID = pInvoke->invokeID;
	error.error_value = 0;
	error.error = new AsnAny;
	error.error->value = pError;
	if (szSessionID)
		error.sessionID = new UTF8String(szSessionID);

	lRoseResult = SendError(&error);

	// prevent delete of value...
	error.error->value = 0;

	return lRoseResult;
}

void SnaccROSEBase::SetMultithreadInvokeIds(std::map<int, int> mapIds)
{
	m_mapMultithreadInvokeIDs = mapIds;
}

long SnaccROSEBase::SendBinaryDataBlockEx(const char* lpBytes, unsigned long lSize, SnaccInvokeContext* pCtx)
{
	if (m_pTransport)
		return m_pTransport->SendBinaryDataBlockEx(lpBytes, lSize, pCtx);
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

long SnaccROSEBase::HandleInvokeResult(long lRoseResult, SNACC::ROSEMessage* pResponseMsg, SNACC::AsnType* result, SNACC::AsnType* error, SnaccInvokeContext* pCtx)
{
	SNACC::ROSEError* pError = nullptr;
	SNACC::ROSEResult* pResult = nullptr;
	lRoseResult = DecodeResponse(pResponseMsg, &pResult, &pError, pCtx);

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

	if (pResponseMsg)
		delete pResponseMsg;

	return lRoseResult;
}

long SnaccROSEBase::DecodeInvoke(SNACC::ROSEMessage* pInvokeMessage, SNACC::AsnType* argument)
{
	if (!pInvokeMessage || !pInvokeMessage->invoke)
		return ROSE_RE_DECODE_FAILED;
	if (!pInvokeMessage->invoke->argument)
	{
		if(argument->mayBeEmpty())
			return ROSE_NOERROR;
		else
			return ROSE_REJECT_ARGUMENT_MISSING;
	}

	long lRoseResult = ROSE_NOERROR;

	try
	{
		if (pInvokeMessage->invoke->argument->anyBuf)
		{
			AsnLen len;
			argument->BDec(*pInvokeMessage->invoke->argument->anyBuf, len);
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
				AsnAny* pOriginalArgument = pInvokeMessage->invoke->argument;

				// Set the decoded result object into the response to be able to fully log the whole message
				pInvokeMessage->invoke->argument = new AsnAny();
				pInvokeMessage->invoke->argument->value = argument;

				// Get the name of the called operation for logging
				std::string strOperationName;
				if (pInvokeMessage->invoke->operationName)
					strOperationName = pInvokeMessage->invoke->operationName->getUTF8().c_str();
				if (strOperationName.empty())
					strOperationName = SnaccRoseOperationLookup::LookUpName(pInvokeMessage->invoke->operationID);
				const char* szOperationName = strOperationName.empty() ? nullptr : strOperationName.c_str();
				LogTransportData(false, SNACC::TransportEncoding::BER, szOperationName, nullptr, 0, pInvokeMessage, nullptr);
				// As we hand back the result object to the outer world (function argument) we need to set it to NULL to prevent deletion if we discard the inserted object
				pInvokeMessage->invoke->argument->value = NULL;

				// Delete the inserted result object and reset to the original response object
				delete pInvokeMessage->invoke->argument;
				pInvokeMessage->invoke->argument = pOriginalArgument;
			}
		}
		else if (pInvokeMessage->invoke->argument->jsonBuf)
		{
			argument->JDec(*pInvokeMessage->invoke->argument->jsonBuf);
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

int SnaccROSEBase::ConfigureFileLogging(const char* szPath, const bool bAppend /*= true*/, const bool bFlushEveryWrite /* = false */)
{
	std::lock_guard<std::mutex> lock(m_mtxLogFile);

	if (szPath && strlen(szPath) && !m_pAsnLogFile)
	{
		m_bFlushEveryWrite = bFlushEveryWrite;
		const char* szMode = bAppend ? "a" : "w";
#ifdef WIN32
    #ifdef _MSC_VER
		m_pAsnLogFile = _fsopen(szPath, szMode, _SH_DENYWR);
    #else
        // MingW, Clang, GCC
        m_pAsnLogFile = fopen(szPath, szMode);
    #endif
#else
        m_pAsnLogFile = fopen(szPath, szMode);
#endif
		if (!m_pAsnLogFile)
		{
			int iErr = 0;
#ifdef WIN32
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
	else if ((!szPath || !strlen(szPath)) && m_pAsnLogFile)
	{
		fclose(m_pAsnLogFile);
		m_pAsnLogFile = nullptr;
		m_bAsnLogFileContainsData = false;
	}
	return 0;
}

void SnaccROSEBase::PrintJSONToLog(const bool bOutbound, const bool bError, const char* szOperationName, const char* szData, const size_t stLength)
{
	if (!m_pAsnLogFile)
		return;

	// Die ASN.1-Funktionen k�nnen auch aus verschiedenen Threads gerufen werden.
	// Das Schreiben in die roseout.log muss aber damit serialisiert werden.
	// Der Lock sollte nicht schaden, da nur die Datei selbst gelockt wird und kein anderes Objekt (PAIM-1732).
	std::lock_guard<std::mutex> lock(m_mtxLogFile);

	// Securly check the logfile pointer once more after aquiring the lock
	if (!m_pAsnLogFile)
		return;

	try
	{
		auto currentTime = std::chrono::system_clock::now();
		std::time_t currentTimeT = std::chrono::system_clock::to_time_t(currentTime);
		auto duration = currentTime.time_since_epoch();
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;
		std::tm timeInfo;
#ifdef WIN32
		gmtime_s(&timeInfo, &currentTimeT);
#else
        gmtime_r(&currentTimeT, &timeInfo);
#endif
		std::ostringstream strTime;
		strTime << std::put_time(&timeInfo, "%Y-%m-%dT%H:%M:%S.");
		strTime << std::setfill('0') << std::setw(3) << milliseconds.count();

		if (!m_bAsnLogFileContainsData)
		{
			fprintf(m_pAsnLogFile, "[\n");
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
			fprintf(m_pAsnLogFile, "\t\"%s\" : \n", bError ? "ERROR" : "ROSE");

		size_t stPrintLength = stLength;
		if (!stPrintLength)
			stPrintLength = strlen(szData);

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
	}
	catch (...)
	{
		assert(false);
	}
}
