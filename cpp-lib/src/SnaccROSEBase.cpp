#include "../include/SnaccROSEBase.h"
#include "../include/SNACCROSE.h"
#include <assert.h>
#include <iomanip>
#include <locale>

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
	// Checking whether an ASN1 operationID was assigned twice
	assert(LookUpName(iOpID) == "unknown");
#endif

	m_mapOpToID[szOpName] = iOpID;
	m_mapIDToOp[iOpID] = szOpName;
	m_mapIDToInterface[iOpID] = iInterfaceID;
}

int SnaccRoseOperationLookup::LookUpInterfaceID(int iOpID)
{
	std::map<int, int>::iterator it = m_mapIDToInterface.find(iOpID);
	if (it != m_mapIDToInterface.end())
		return it->second;

	return 0;
}

std::string SnaccRoseOperationLookup::LookUpName(int iOpID)
{
	std::map<int, std::string>::iterator it = m_mapIDToOp.find(iOpID);
	if (it != m_mapIDToOp.end())
		return it->second;

	return "unknown";
}

int SnaccRoseOperationLookup::LookUpID(const char* szOpName)
{
	std::map<std::string, int>::iterator it = m_mapOpToID.find(szOpName);
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

	SnaccROSEPendingOperationMap::iterator it;
	for (it = m_PendingOperations.begin(); it != m_PendingOperations.end(); it++)
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

	SnaccROSEPendingOperationMap::iterator it;
	it = m_PendingOperations.find(invokeID);
	if (it != m_PendingOperations.end())
		m_PendingOperations.erase(it);
}

bool SnaccROSEBase::CompletePendingOperation(int invokeID, SNACC::ROSEMessage* pMessage)
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	SnaccROSEPendingOperationMap::iterator it;
	it = m_PendingOperations.find(invokeID);
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
			catch (SnaccException& e)
			{
				LogTransportData(false, m_eTransportEncoding, lpBytes, lSize, nullptr);
				std::string strError;
				strError = "SnaccException SnaccROSEBase::OnBinaryDataBlock: ";
				strError += e.what();
				PrintToErrorLog(strError);
				OnRoseDecodeError(lpBytes, lSize, e.what());

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
			LogTransportData(false, m_eTransportEncoding, lpBytes, lSize, pmessage);

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
			}
			else
				m_eTransportEncoding = SNACC::TransportEncoding::JSON_NO_HEADING;

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
					LogTransportData(false, m_eTransportEncoding, lpBytes, lSize, pmessage);
				}
				catch (SnaccException& e)
				{
					LogTransportData(false, m_eTransportEncoding, lpBytes, lSize, nullptr);
					std::string strError;
					strError = "JDec Exception: ";
					strError += e.what();
					PrintToErrorLog(strError);
					OnRoseDecodeError(lpBytes, lSize, e.what());

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
				// pmessage will be deleted inside
				OnROSEMessage(pmessage, true);
			}
			else
			{
				LogTransportData(false, m_eTransportEncoding, lpBytes, lSize, nullptr);

				// Error invalid json
				std::string strError;
				strError = "SnaccROSEBase::OnBinaryDataBlock: Invalid JSON: ";
				strError += reader.getFormattedErrorMessages();
				std::stringstream strContent;

				const unsigned long maxSIZE = 50;
				strContent << "- First Bytes (max " << maxSIZE << "/" << lSize << "): ";
				strContent << std::hex << std::setfill('0');

				std::locale loc;
				for (unsigned long ul = 0; ul < lSize && ul < maxSIZE; ul++)
				{
					// others in \HEX Code
					strContent << std::setw(2) << (unsigned int)lpBytes[ul];
				}
				strError += strContent.str();

				PrintToErrorLog(strError);

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
			LogTransportData(false, m_eTransportEncoding, lpBytes, lSize, nullptr);
			// unknown Encoding
			std::string strError;
			strError = "SnaccROSEBase::OnBinaryDataBlock: Unknown Encoding: ";

			std::stringstream strContent;

			const unsigned long maxSIZE = 50;
			strContent << "- First Bytes (max " << maxSIZE << "/" << lSize << "): ";
			strContent << std::hex << std::setfill('0');

			std::locale loc;
			for (unsigned long ul = 0; ul < lSize && ul < maxSIZE; ul++)
			{
				// others in \HEX Code
				strContent << std::setw(2) << (unsigned int)lpBytes[ul];
			}
			strError += strContent.str();

			PrintToErrorLog(strError);

			OnRoseDecodeError(lpBytes, lSize, "Unknown Encoding");
		}
	}
	catch (SnaccException& e)
	{
		std::string strError;
		strError = "SnaccException SnaccROSEBase::OnBinaryDataBlock: ";
		strError += e.what();
		PrintToErrorLog(strError);
		OnRoseDecodeError(lpBytes, lSize, e.what());
	}
	// Kein catch all, verschleppt nur die Fehler.
	// catch (...)
	//{
	//	std::string strError;
	//	strError = "Exception SnaccROSEBase::OnBinaryDataBlock: general";
	//	PrintToErrorLog(strError);
	//}
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
					}
					else
					{
						// Can happen if you send e.g. a1 00 00 as a BER Encoded packet
						delete pmessage;
					}
				}
				else
				{
					delete pmessage;
				}
				return true;
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

	EAsnLogLevel level = GetLogLevel(true);
	if (level != EAsnLogLevel::DISABLED)
	{
		std::stringstream str;
		if (preject->invokedID.choiceId == ROSERejectChoice::invokedIDCid)
		{
			str << "REJECT InvokeID ";
			preject->invokedID.invokedID->Print(str);
			if (preject->reject)
			{
				str << ", Problem ";
				if (preject->reject->invokeProblem)
					preject->reject->invokeProblem->Print(str);
			}
			str << std::endl;
		}
		else
		{
			str << "REJECT" << std::endl;
		}

		auto data = str.rdbuf()->str();
		PrintToLog(data.c_str());
	}

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

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), BytesEncoded, nullptr);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		SJson::FastWriter writer;
		SJson::Value value;
		rejectMsg.JEnc(value);
		std::string strData = writer.write(value);
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

	std::string strError;
	try
	{
		if (pInvoke->operationName && pInvoke->operationID == 0)
			pInvoke->operationID = SnaccRoseOperationLookup::LookUpID(pInvoke->operationName->getASCII().c_str());

		lRoseResult = OnInvoke(pMsg, &invokeContext);
	}
	catch (SnaccException& ex)
	{
		// decode of invoke detail failed.
		lRoseResult = ROSE_REJECT_MISTYPEDARGUMENT;

		strError = ex.what();
		std::stringstream str;
		str << "SnaccException InvokeID " << pInvoke->invokeID << " : " << ex.m_errorCode << " - " << ex.what() << std::endl;

		auto data = str.rdbuf()->str();
		PrintToLog(data.c_str());
	}

	// if the Result is ROSE_NOERROR, then the Result has been sent.
	// if the result is ROSE_REJECT_ASYNCOPERATION, the result will be sent async
	if (lRoseResult != ROSE_NOERROR && lRoseResult != ROSE_REJECT_ASYNCOPERATION)
	{
		// Send reject
		if (lRoseResult == ROSE_REJECT_UNKNOWNOPERATION)
			SendRejectInvoke(pInvoke->invokeID, InvokeProblem::unrecognisedOperation, "unrecognisedOperation", pInvoke->sessionID ? pInvoke->sessionID->c_str() : 0);
		else if (lRoseResult == ROSE_REJECT_MISTYPEDARGUMENT)
			SendRejectInvoke(pInvoke->invokeID, InvokeProblem::mistypedArgument, strError.c_str(), pInvoke->sessionID ? pInvoke->sessionID->c_str() : 0);
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

long SnaccROSEBase::SendEvent(SNACC::ROSEInvoke* pinvoke, SnaccInvokeContext* pCtx /*= nullptr*/)
{
	long lRoseResult = ROSE_NOERROR;
	ROSEMessage InvokeMsg;

	InvokeMsg.choiceId = ROSEMessage::invokeCid;
	InvokeMsg.invoke = pinvoke;

	// encode now.
	if (m_eTransportEncoding == SNACC::TransportEncoding::BER)
	{
		AsnBuf OutBuf;
		AsnLen BytesEncoded = InvokeMsg.BEnc(OutBuf);

		std::string strData;
		OutBuf.ResetMode();
		OutBuf.GetSeg(strData, BytesEncoded);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), BytesEncoded, pCtx);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		InvokeMsg.invoke->operationName = UTF8String::CreateNewFromASCII(SnaccRoseOperationLookup::LookUpName((int)InvokeMsg.invoke->operationID));
		SJson::FastWriter writer;
		SJson::Value value;
		InvokeMsg.JEnc(value);
		std::string strData = writer.write(value);
		if (m_eTransportEncoding == SNACC::TransportEncoding::JSON)
			strData = GetJsonAsnPrefix(strData) + strData;
		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), (unsigned long)strData.length(), pCtx);
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	// prevent autodelete of pinvoke
	InvokeMsg.invoke = NULL;

	return lRoseResult;
}

EAsnLogLevel SnaccROSEBase::GetLogLevel(const bool bOutbound)
{
	return EAsnLogLevel::DISABLED;
}

void SnaccROSEBase::LogTransportData(const bool bOutbound, const SNACC::TransportEncoding encoding, const char* szData, const size_t size, const SNACC::ROSEMessage* pMSg)
{
	EAsnLogLevel level = GetLogLevel(bOutbound);
	if (level != EAsnLogLevel::DISABLED)
	{
		// We need to log something...
		;
		if (level == EAsnLogLevel::JSON || level == EAsnLogLevel::JSON_AND_BER)
		{
			// JSON logging is requested
			if (encoding == SNACC::TransportEncoding::JSON || encoding == SNACC::TransportEncoding::JSON_NO_HEADING)
			{
				// The payload is already json -> we can directly log it
				PrintToLog(szData);
			}
			else if (pMSg)
			{
				// The strTransportData is not already JSON encoded...
				// So we encode the ROSEMessage as JSON
				SJson::Value value;
				pMSg->JEnc(value);
				SJson::StyledWriter writer;
				std::string strLogData = writer.write(value);
				PrintToLog(strLogData.c_str());
			}
			else
			{
				PrintToLog("ERROR: Message was not decodable");
			}
		}
		if (level == EAsnLogLevel::BER || level == EAsnLogLevel::JSON_AND_BER)
		{
			// BER logging is requested
			// Only makes sense if the payload is BER
			if (encoding == SNACC::TransportEncoding::BER)
			{
				bool bFirst = true;
				std::string strLogData;
				strLogData.reserve(size * 3);
				const char* szCur = szData;
				for(size_t pos = 0; pos < size; pos++)
				{
					char szHex[5] = {0};
					if (bFirst)
						bFirst = false;
					else
						strLogData += " ";
#if _WIN32
					sprintf_s(szHex, 5, "%02X", *szCur);
#else
					snprintf(szHex, 5, "%02X", *szCur);
#endif
					szCur++;
					strLogData += szHex;
				}
				strLogData += "\n";
				PrintToLog(strLogData.c_str());
			}
		}
	}
}

long SnaccROSEBase::SendInvoke(SNACC::ROSEInvoke* pinvoke, SNACC::ROSEResult** ppresult, SNACC::ROSEError** pperror, int iTimeout /*= -1*/, SnaccInvokeContext* pCtx /*= nullptr*/)
{
	long lRoseResult = ROSE_NOERROR;
	ROSEMessage InvokeMsg;

	InvokeMsg.choiceId = ROSEMessage::invokeCid;
	InvokeMsg.invoke = pinvoke;

	// encode now.

	SnaccROSEPendingOperation* pendingOP = new SnaccROSEPendingOperation;
	pendingOP->m_lInvokeID = pinvoke->invokeID;
	AddPendingOperation(pendingOP->m_lInvokeID, pendingOP);

	if (m_eTransportEncoding == SNACC::TransportEncoding::BER)
	{
		AsnBuf OutBuf;
		AsnLen BytesEncoded = InvokeMsg.BEnc(OutBuf);

		std::string strData;
		OutBuf.ResetMode();
		OutBuf.GetSeg(strData, BytesEncoded);

		LogTransportData(true, m_eTransportEncoding, strData.c_str(), strData.length(), &InvokeMsg);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), BytesEncoded, pCtx);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		InvokeMsg.invoke->operationName = UTF8String::CreateNewFromASCII(SnaccRoseOperationLookup::LookUpName((int)InvokeMsg.invoke->operationID));

		SJson::Value value;
		InvokeMsg.JEnc(value);
		std::string strData;

		auto logLevel = GetLogLevel(true);
		if (logLevel == EAsnLogLevel::JSON || logLevel == EAsnLogLevel::JSON_AND_BER)
		{
			SJson::StyledWriter writer;
			strData = writer.write(value);
		}
		else
		{
			SJson::FastWriter writer;
			strData = writer.write(value);
		}
		if (m_eTransportEncoding == SNACC::TransportEncoding::JSON)
			strData = GetJsonAsnPrefix(strData) + strData;

		LogTransportData(true, m_eTransportEncoding, strData.c_str(), strData.length(), &InvokeMsg);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), (unsigned long)strData.length(), pCtx);
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

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
					// decode result
					switch (pendingOP->m_pAnswerMessage->choiceId)
					{
						case ROSEMessage::resultCid:
							{
								lRoseResult = ROSE_NOERROR;
								if (pendingOP->m_pAnswerMessage->result && ppresult)
								{
									// the result is passed to the caller, who then has to deallocate it.
									*ppresult = pendingOP->m_pAnswerMessage->result;
									pendingOP->m_pAnswerMessage->result = NULL;
								}
							}
							break;
						case ROSEMessage::errorCid:
							{
								lRoseResult = ROSE_ERROR_VALUE;
								if (pendingOP->m_pAnswerMessage->error && pperror)
								{
									// pass the error to the caller, who then has to deallocate it.
									*pperror = pendingOP->m_pAnswerMessage->error;
									pendingOP->m_pAnswerMessage->error = NULL;
								}
							}
							break;
						case ROSEMessage::rejectCid:
							{
								lRoseResult = ROSE_REJECT_UNKNOWN;
								if (pendingOP->m_pAnswerMessage->reject && pendingOP->m_pAnswerMessage->reject->reject)
								{
									if (pendingOP->m_pAnswerMessage->reject->reject->choiceId == RejectProblem::invokeProblemCid)
									{
										if (*pendingOP->m_pAnswerMessage->reject->reject->invokeProblem == InvokeProblem::unrecognisedOperation)
											lRoseResult = ROSE_REJECT_UNKNOWNOPERATION;
										else if (*pendingOP->m_pAnswerMessage->reject->reject->invokeProblem == InvokeProblem::mistypedArgument)
											lRoseResult = ROSE_REJECT_MISTYPEDARGUMENT;
										else if (*pendingOP->m_pAnswerMessage->reject->reject->invokeProblem == InvokeProblem::resourceLimitation)
											lRoseResult = ROSE_REJECT_FUNCTIONMISSING;
										else if (*pendingOP->m_pAnswerMessage->reject->reject->invokeProblem == InvokeProblem::authenticationIncomplete)
										{
											lRoseResult = ROSE_REJECT_AUTHENTICATIONINCOMPLETE;
											if (pCtx)
											{
												if (pendingOP->m_pAnswerMessage->reject->authentication)
													pCtx->pRejectAuth = (SNACC::ROSEAuthResult*)pendingOP->m_pAnswerMessage->reject->authentication->Clone();
												pCtx->lRejectResult = ROSE_REJECT_AUTHENTICATIONINCOMPLETE;
											}
										}
										else if (*pendingOP->m_pAnswerMessage->reject->reject->invokeProblem == InvokeProblem::authenticationFailed)
										{
											lRoseResult = ROSE_REJECT_AUTHENTICATIONFAILED;
											if (pCtx)
											{
												if (pendingOP->m_pAnswerMessage->reject->authentication)
													pCtx->pRejectAuth = (SNACC::ROSEAuthResult*)pendingOP->m_pAnswerMessage->reject->authentication->Clone();
												pCtx->lRejectResult = ROSE_REJECT_AUTHENTICATIONFAILED;
											}
										}
									}
								}
							}
							break;
						default:
							{
								lRoseResult = ROSE_RE_INVALID_ANSWER;
							}
							break;
					}
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

	// prevent autodelete of pinvoke
	InvokeMsg.invoke = NULL;

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

		LogTransportData(true, m_eTransportEncoding, strData.c_str(), BytesEncoded, nullptr);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), BytesEncoded, nullptr);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		SJson::FastWriter writer;
		SJson::Value value;
		ResultMsg.JEnc(value);
		std::string strData = writer.write(value);
		if (m_eTransportEncoding == SNACC::TransportEncoding::JSON)
			strData = GetJsonAsnPrefix(strData) + strData;

		LogTransportData(true, m_eTransportEncoding, strData.c_str(), strData.length(), nullptr);

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

	ROSEMessage ResultMsg;
	ResultMsg.choiceId = ROSEMessage::errorCid;
	ResultMsg.error = perror;

	// encode now.
	if (m_eTransportEncoding == SNACC::TransportEncoding::BER)
	{
		AsnBuf OutBuf;
		AsnLen BytesEncoded = ResultMsg.BEnc(OutBuf);

		std::string strData;
		OutBuf.ResetMode();
		OutBuf.GetSeg(strData, BytesEncoded);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), BytesEncoded, nullptr);
	}
	else if (m_eTransportEncoding == SNACC::TransportEncoding::JSON || m_eTransportEncoding == SNACC::TransportEncoding::JSON_NO_HEADING)
	{
		SJson::FastWriter writer;
		SJson::Value value;
		ResultMsg.JEnc(value);
		std::string strData = writer.write(value);
		if (m_eTransportEncoding == SNACC::TransportEncoding::JSON)
			strData = GetJsonAsnPrefix(strData) + strData;
		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), (unsigned long)strData.length(), nullptr);
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	// prevent delete of perror...
	ResultMsg.error = 0;

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
