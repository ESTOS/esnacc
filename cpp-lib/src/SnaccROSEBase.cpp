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
	if (pCustom) {
		// If you set data in pCustom ensure that it is deleted (freeed) before the object runs out of scope...
		assert(0);
	}
}


std::map<std::string, int> SnaccRoseOperationLookup::m_mapOpToID;
std::map<int, std::string> SnaccRoseOperationLookup::m_mapIDToOp;
std::map<int, int> SnaccRoseOperationLookup::m_mapIDToInterface;

//check if at least one Operation has been registerd
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
		//Prüfung, ob eine ASN1 operationID doppelt vergeben wurde
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
	{
		m_pAnswerMessage = pAnswerMessage;
	}
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
	{
		m_PendingOperations.erase(it);
	}
}

bool SnaccROSEBase::CompletePendingOperation(int invokeID, SNACC::ROSEMessage* pMessage)
{
	std::lock_guard<std::mutex> guard(m_InternalProtectMutex);

	SnaccROSEPendingOperationMap::iterator it;
	it = m_PendingOperations.find(invokeID);
	if (it != m_PendingOperations.end())
	{
		//found...
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
			case SnaccTransportEncoding::BER:
			{
				AsnBuf buffer((const char*)lpBytes, lSize);
				ROSEMessage* pmessage = new ROSEMessage;
				AsnLen bytesDecoded = 0;
				try
				{
					pmessage->BDec(buffer, bytesDecoded);
				}
				catch (SnaccException& e)
				{
					std::string strError;
					strError = "SnaccException SnaccROSEBase::OnBinaryDataBlockResult: ";
					strError += e.what();
					PrintToErrorLog(strError);

					if (pmessage->choiceId == ROSEMessage::invokeCid &&
						pmessage->invoke)
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
				//pmessage will be deleted inside
				bReturn = OnROSEMessage(pmessage, false);
				break;
			}
			case SnaccTransportEncoding::JSON:
			case SnaccTransportEncoding::JSON_NO_HEADING:
			{
				int iHeaderLen = GetJsonHeaderLen(lpBytes, lSize);
				EJson::Value value;
				EJson::Reader reader;
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
					catch (SnaccException& e)
					{
						std::string strError;
						strError = "JDec Exception: ";
						strError += e.what();
						PrintToErrorLog(strError);

						if (pmessage->choiceId == ROSEMessage::invokeCid &&
							pmessage->invoke)
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
					//pmessage will be deleted inside
					bReturn = OnROSEMessage(pmessage, false);
				}
				else
				{
					//Error invalid json
					std::string strError;
					strError = "SnaccROSEBase::OnBinaryDataBlockResult: Invalid JSON: ";
					strError += reader.getFormattedErrorMessages();
					PrintToErrorLog(strError);
				}
				break;
			}
			default:
				throw std::runtime_error("invalid m_eTransportEncoding");
				break;
			}
	}
	catch (SnaccException &e)
	{
		std::string strError;
		strError = "SnaccException SnaccROSEBase::OnBinaryDataBlockResult: ";
		strError += e.what();
		PrintToErrorLog(strError);
	}
	// Kein catch all, verschleppt nur die Fehler.
	//catch (...)
	//{
	//	std::string strError;
	//	strError = "Exception SnaccROSEBase::OnBinaryDataBlockResult: general";
	//	PrintToErrorLog(strError);
	//}
	return bReturn;
}

int SnaccROSEBase::GetJsonHeaderLen(const char* lpBytes, unsigned long iLength)
{
	unsigned int iLen = 0;
	if (lpBytes[0] == 'J')
	{
		iLen++;
		for (unsigned int i = 1; i < 10; i++)
		{
			if ((iLength >= iLen) && lpBytes[i] >= '0' && lpBytes[i] <= '9')
				iLen++;
			else
				break;
		}
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
			m_eTransportEncoding = SnaccTransportEncoding::BER;
			AsnBuf buffer((const char*)lpBytes, lSize);
			ROSEMessage* pmessage = new ROSEMessage;
			AsnLen bytesDecoded = 0;
			try
			{
				pmessage->BDec(buffer, bytesDecoded);
			}
			catch (SnaccException &e)
			{
				std::string strError;
				strError = "SnaccException SnaccROSEBase::OnBinaryDataBlock: ";
				strError += e.what();
				PrintToErrorLog(strError);
				OnRoseDecodeError(lpBytes, lSize, e.what());

				if (pmessage->choiceId == ROSEMessage::invokeCid &&
					pmessage->invoke)
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
			//pmessage will be deleted inside
			OnROSEMessage(pmessage, true);
		}
		else if (byFirst == 'J' || byFirst == '{' || byFirst == '[')
		{
			int iHeaderLen = 0;
			if (byFirst == 'J') {
				m_eTransportEncoding = SnaccTransportEncoding::JSON;
				iHeaderLen = GetJsonHeaderLen(lpBytes, lSize);
			}
			else
				m_eTransportEncoding = SnaccTransportEncoding::JSON_NO_HEADING;

			EJson::Value value;
			EJson::Reader reader;
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
				catch (SnaccException &e)
				{
					std::string strError;
					strError = "JDec Exception: ";
					strError += e.what();
					PrintToErrorLog(strError);
					OnRoseDecodeError(lpBytes, lSize, e.what());

					if (pmessage->choiceId == ROSEMessage::invokeCid &&
						pmessage->invoke)
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
				//pmessage will be deleted inside
				OnROSEMessage(pmessage, true);
			}
			else
			{
				//Error invalid json
				std::string strError;
				strError = "SnaccROSEBase::OnBinaryDataBlock: Invalid JSON: ";
				strError += reader.getFormattedErrorMessages();
				std::stringstream strContent;

				const unsigned long maxSIZE = 50;
				strContent << "- First Bytes (max "<< maxSIZE <<"/" << lSize << "): ";
				strContent << std::hex << std::setfill('0');

				std::locale loc;
				for(unsigned long ul=0; ul < lSize && ul < maxSIZE; ul++)
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
			//unknown Encoding
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
	catch (SnaccException &e)
	{
		std::string strError;
		strError = "SnaccException SnaccROSEBase::OnBinaryDataBlock: ";
		strError += e.what();
		PrintToErrorLog(strError);
		OnRoseDecodeError(lpBytes, lSize, e.what());
	}
	// Kein catch all, verschleppt nur die Fehler.
	//catch (...)
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
					{
						OnInvokeMessage(pmessage->invoke);
					}
					//else
					//{
					//  wird am Ende der Methode gemacht
					//	delete pmessage;
					//}
					bProcessed = true;
				}
			}
			break;
		case ROSEMessage::resultCid:
			if (pmessage->result)
			{
				OnResultMessage(pmessage->result);
				// nichts abfangen wenn an der Stelle pmessage->result->result == nullptr ist, da durch die fehlende invokeID das auch schon
				// berücksichtigt ist
				CompletePendingOperation(pmessage->result->invokeID, pmessage);
				return true;
			}
			break;
		case ROSEMessage::errorCid:
			if (pmessage->error)
			{
				OnErrorMessage(pmessage->error);
				// nichts abfangen wenn an der Stelle pmessage->error->error == nullptr ist, da durch die fehlende invokeID das auch schon
				// berücksichtigt ist
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
					// Prüfung! bei REJECT ist die InvokeID ein Choice und damit ist die ID selbst ein Pointer!
					// und der kann nullptr werden.
					if (pmessage->reject->invokedID.invokedID != nullptr)
					{
						CompletePendingOperation(*pmessage->reject->invokedID.invokedID, pmessage);
					}
					else
					{
						// Kann passieren, wenn man z.B. a1 00 00 als BER Encoded Paket schickt
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

	if(GetLogLevel(true))
	{
		std::stringstream str;
		if(preject->invokedID.choiceId == ROSERejectChoice::invokedIDCid)
		{
			str << "REJECT InvokeID ";
			preject->invokedID.invokedID->Print(str);
			if (preject->reject)
			{
				str << ", Problem ";
				if (preject->reject->invokeProblem)
				{
					preject->reject->invokeProblem->Print(str);
				}
			}
			str << std::endl;
		}
		else
		{
			str << "REJECT" << std::endl;
		}

		std::string outStr = str.str();
		PrintToLog(outStr);
	}

	ROSEMessage rejectMsg;
	rejectMsg.choiceId = ROSEMessage::rejectCid;
	rejectMsg.reject = preject;

	//encode now.
	if (m_eTransportEncoding == SnaccTransportEncoding::BER)
	{
		AsnBuf OutBuf;
		AsnLen BytesEncoded = rejectMsg.BEnc(OutBuf);

		std::string strData;
		OutBuf.ResetMode();
		OutBuf.GetSeg(strData, BytesEncoded);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), BytesEncoded, nullptr);
	}
	else if (m_eTransportEncoding == SnaccTransportEncoding::JSON || m_eTransportEncoding == SnaccTransportEncoding::JSON_NO_HEADING)
	{
		EJson::FastWriter writer;
		EJson::Value value;
		rejectMsg.JEnc(value);
		std::string strData = writer.write(value);
		if(m_eTransportEncoding == SnaccTransportEncoding::JSON)
			strData = GetJsonAsnPrefix(strData) + strData;
		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), (unsigned long)strData.length(), nullptr);
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	//prevent delete of preject...
	rejectMsg.reject = 0;

	return lRoseResult;
}

std::string SnaccROSEBase::GetJsonAsnPrefix(std::string& strJson)
{
	//Calc prefix z.B. (J1234567)
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
		//Events do not get an answer
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
	{
		reject.authentication = (SNACC::ROSEAuthResult*)pAuthHeader->Clone();
	}

	if ((m_eTransportEncoding == SnaccTransportEncoding::JSON || m_eTransportEncoding == SnaccTransportEncoding::JSON_NO_HEADING) && szError != NULL)
	{
		reject.details = UTF8String::CreateNewFromUTF8(szError);
	}
	return SendReject(&reject);
}

void SnaccROSEBase::OnInvokeMessage(SNACC::ROSEInvoke* pinvoke)
{
	long lRoseResult = ROSE_REJECT_UNKNOWNOPERATION;

	SnaccInvokeContext invokeContext;
	invokeContext.pInvokeAuth = pinvoke->authentication;
	invokeContext.pInvoke = pinvoke;

	bool bNotifyRunningOutOfScope = OnInvokeContextCreated(&invokeContext);

	std::string strError;
	try
	{
		if (pinvoke->operationName && pinvoke->operationID == 0)
		{
			pinvoke->operationID = SnaccRoseOperationLookup::LookUpID(pinvoke->operationName->getASCII().c_str());
		}

		lRoseResult = OnInvoke(pinvoke, &invokeContext);
	}
	catch (SnaccException& ex)
	{
		//decode of invoke detail failed.
		lRoseResult = ROSE_REJECT_MISTYPEDARGUMENT;

		strError = ex.what();
		std::stringstream str;
		str << "SnaccException InvokeID " << pinvoke->invokeID << " : " << ex.m_errorCode << " - " << ex.what() << std::endl;

		std::string outStr = str.str();
		PrintToLog(outStr);
	}

	//if the Result is ROSE_NOERROR, then the Result has been sent.
	//if the result is ROSE_REJECT_ASYNCOPERATION, the result will be sent async
	if (lRoseResult != ROSE_NOERROR && lRoseResult != ROSE_REJECT_ASYNCOPERATION)
	{
		//Send reject
		if (lRoseResult == ROSE_REJECT_UNKNOWNOPERATION)
			SendRejectInvoke(pinvoke->invokeID, InvokeProblem::unrecognisedOperation, "unrecognisedOperation", pinvoke->sessionID ? pinvoke->sessionID->c_str() : 0);
		else if (lRoseResult == ROSE_REJECT_MISTYPEDARGUMENT)
			SendRejectInvoke(pinvoke->invokeID, InvokeProblem::mistypedArgument, strError.c_str(), pinvoke->sessionID ? pinvoke->sessionID->c_str() : 0);
		else if (lRoseResult == ROSE_REJECT_FUNCTIONMISSING)
			SendRejectInvoke(pinvoke->invokeID, InvokeProblem::resourceLimitation, "functionMissing", pinvoke->sessionID ? pinvoke->sessionID->c_str() : 0);
		else if (lRoseResult == ROSE_REJECT_INVALIDSESSIONID)
			SendRejectInvoke(pinvoke->invokeID, InvokeProblem::invalidSessionID, "invalidSessionID", pinvoke->sessionID ? pinvoke->sessionID->c_str() : 0);
		else if (lRoseResult == ROSE_REJECT_AUTHENTICATIONFAILED)
			SendRejectInvoke(pinvoke->invokeID, InvokeProblem::authenticationFailed, "authenticationFailed", pinvoke->sessionID ? pinvoke->sessionID->c_str() : 0, invokeContext.pRejectAuth);
		else if (lRoseResult == ROSE_REJECT_AUTHENTICATIONINCOMPLETE)
			SendRejectInvoke(pinvoke->invokeID, InvokeProblem::authenticationIncomplete, "authenticationIncomplete", pinvoke->sessionID ? pinvoke->sessionID->c_str() : 0, invokeContext.pRejectAuth);
		else if (lRoseResult == ROSE_REJECT_AUTHENTICATION_USER_TEMPORARY_LOCKED_OUT)
			SendRejectInvoke(pinvoke->invokeID, InvokeProblem::authenticationFailed, "temporaryLockedOut", pinvoke->sessionID ? pinvoke->sessionID->c_str() : 0, invokeContext.pRejectAuth);
		else if (lRoseResult == ROSE_REJECT_AUTHENTICATION_USER_LOCKED_OUT)
			SendRejectInvoke(pinvoke->invokeID, InvokeProblem::authenticationFailed, "lockedOut", pinvoke->sessionID ? pinvoke->sessionID->c_str() : 0, invokeContext.pRejectAuth);
		else if (lRoseResult == ROSE_REJECT_AUTHENTICATION_SERVER_BUSY)
			SendRejectInvoke(pinvoke->invokeID, InvokeProblem::authenticationFailed, "serverBusy", pinvoke->sessionID ? pinvoke->sessionID->c_str() : 0, invokeContext.pRejectAuth);
		else
			SendRejectInvoke(pinvoke->invokeID, InvokeProblem::unrecognisedOperation, "otherError", pinvoke->sessionID ? pinvoke->sessionID->c_str() : 0);
	}
	if (invokeContext.funcAfterResult)
		invokeContext.funcAfterResult();

	if (bNotifyRunningOutOfScope)
		OnInvokeContextRunsOutOfScope(&invokeContext);

	//prevent autodelete
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

	//encode now.
	if (m_eTransportEncoding == SnaccTransportEncoding::BER)
	{
		AsnBuf OutBuf;
		AsnLen BytesEncoded = InvokeMsg.BEnc(OutBuf);

		std::string strData;
		OutBuf.ResetMode();
		OutBuf.GetSeg(strData, BytesEncoded);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), BytesEncoded, pCtx);
	}
	else if (m_eTransportEncoding == SnaccTransportEncoding::JSON || m_eTransportEncoding == SnaccTransportEncoding::JSON_NO_HEADING)
	{
		InvokeMsg.invoke->operationName = UTF8String::CreateNewFromASCII(SnaccRoseOperationLookup::LookUpName((int)InvokeMsg.invoke->operationID));
		EJson::FastWriter writer;
		EJson::Value value;
		InvokeMsg.JEnc(value);
		std::string strData = writer.write(value);
		if (m_eTransportEncoding == SnaccTransportEncoding::JSON)
			strData = GetJsonAsnPrefix(strData) + strData;
		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), (unsigned long)strData.length(), pCtx);
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	//prevent autodelete of pinvoke
	InvokeMsg.invoke = NULL;

	return lRoseResult;
}

void SnaccROSEBase::PrintAsnType(bool bOutbound, SNACC::AsnType* pType, SNACC::ROSEInvoke* pInvoke)
{
	if (GetLogLevel(true))
	{
		EJson::StyledStreamWriter writer;
		EJson::Value value;
		pType->JEnc(value);

		std::stringstream strOut;
		strOut << pType->typeName() << std::endl;
		if (pInvoke)
			AddInvokeHeaderLog(strOut, pInvoke);

		writer.write(strOut, value);
		PrintToLog(strOut.str());
	}
}

long SnaccROSEBase::SendInvoke(SNACC::ROSEInvoke* pinvoke, SNACC::ROSEResult** ppresult, SNACC::ROSEError** pperror, int iTimeout /*= -1*/, SnaccInvokeContext* pCtx /*= nullptr*/)
{
	long lRoseResult = ROSE_NOERROR;
	ROSEMessage InvokeMsg;

	InvokeMsg.choiceId = ROSEMessage::invokeCid;
	InvokeMsg.invoke = pinvoke;

	//encode now.

	SnaccROSEPendingOperation* pendingOP = new SnaccROSEPendingOperation;
	pendingOP->m_lInvokeID = pinvoke->invokeID;
	AddPendingOperation(pendingOP->m_lInvokeID, pendingOP);

	if (m_eTransportEncoding == SnaccTransportEncoding::BER)
	{
		AsnBuf OutBuf;
		AsnLen BytesEncoded = InvokeMsg.BEnc(OutBuf);

		std::string strData;
		OutBuf.ResetMode();
		OutBuf.GetSeg(strData, BytesEncoded);
		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), BytesEncoded, pCtx);
	}
	else if (m_eTransportEncoding == SnaccTransportEncoding::JSON || m_eTransportEncoding == SnaccTransportEncoding::JSON_NO_HEADING)
	{
		InvokeMsg.invoke->operationName = UTF8String::CreateNewFromASCII(SnaccRoseOperationLookup::LookUpName((int)InvokeMsg.invoke->operationID));
		EJson::FastWriter writer;
		EJson::Value value;
		InvokeMsg.JEnc(value);
		std::string strData = writer.write(value);
		if (m_eTransportEncoding == SnaccTransportEncoding::JSON)
			strData = GetJsonAsnPrefix(strData) + strData;
		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), (unsigned long)strData.length(), pCtx);
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	if (lRoseResult == 0)
	{
		//Wait for Answer...
		if (iTimeout == -1)
			iTimeout = m_lMaxInvokeWait;

		if(iTimeout)
		{
			if (pendingOP->WaitForComplete(iTimeout))
			{
				lRoseResult = pendingOP->m_lRoseResult;
				if (pendingOP->m_pAnswerMessage)
				{
					//decode result
					switch (pendingOP->m_pAnswerMessage->choiceId)
					{
						case ROSEMessage::resultCid:
						{
							lRoseResult = ROSE_NOERROR;
							if (pendingOP->m_pAnswerMessage->result && ppresult)
							{
								//übergeben des Results an den caller,
								//der muss das dann deallokieren.
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
								//übergeben des Errors an den caller,
								//der muss das dann deallokieren.
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
				//not completed
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

	//prevent autodelete of pinvoke
	InvokeMsg.invoke = NULL;

	return lRoseResult;
}

long SnaccROSEBase::SendResult(SNACC::ROSEResult* presult)
{
	long lRoseResult = ROSE_NOERROR;

	ROSEMessage ResultMsg;
	ResultMsg.choiceId = ROSEMessage::resultCid;
	ResultMsg.result = presult;

	//encode now.
	if (m_eTransportEncoding == SnaccTransportEncoding::BER)
	{
		AsnBuf OutBuf;
		AsnLen BytesEncoded = ResultMsg.BEnc(OutBuf);

		std::string strData;
		OutBuf.ResetMode();
		OutBuf.GetSeg(strData, BytesEncoded);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), BytesEncoded, nullptr);
	}
	else if (m_eTransportEncoding == SnaccTransportEncoding::JSON || m_eTransportEncoding == SnaccTransportEncoding::JSON_NO_HEADING)
	{
		EJson::FastWriter writer;
		EJson::Value value;
		ResultMsg.JEnc(value);
		std::string strData = writer.write(value);
		if(m_eTransportEncoding == SnaccTransportEncoding::JSON)
			strData = GetJsonAsnPrefix(strData) + strData;
		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), (unsigned long)strData.length(), nullptr);
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	//prevent delete of presult...
	ResultMsg.result = 0;

	return lRoseResult;
}

long SnaccROSEBase::SendResult(int invokeID, SNACC::AsnType *value, const wchar_t* szSessionID)
{
	long lRoseResult = ROSE_NOERROR;

	ROSEResult result;
	result.invokeID = invokeID;
	result.result = new ROSEResultSeq;
	result.result->resultValue = 0;
	result.result->result.value = value;
	if (szSessionID)
		result.sessionID = new UTF8String(szSessionID);

	lRoseResult = SendResult(&result);

	//prevent delete of value...
	result.result->result.value = 0;

	return lRoseResult;
}

long SnaccROSEBase::SendError(SNACC::ROSEError* perror)
{
	long lRoseResult = ROSE_NOERROR;

	ROSEMessage ResultMsg;
	ResultMsg.choiceId = ROSEMessage::errorCid;
	ResultMsg.error = perror;

	//encode now.
	if (m_eTransportEncoding == SnaccTransportEncoding::BER)
	{
		AsnBuf OutBuf;
		AsnLen BytesEncoded = ResultMsg.BEnc(OutBuf);

		std::string strData;
		OutBuf.ResetMode();
		OutBuf.GetSeg(strData, BytesEncoded);

		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), BytesEncoded, nullptr);
	}
	else if (m_eTransportEncoding == SnaccTransportEncoding::JSON || m_eTransportEncoding == SnaccTransportEncoding::JSON_NO_HEADING)
	{
		EJson::FastWriter writer;
		EJson::Value value;
		ResultMsg.JEnc(value);
		std::string strData = writer.write(value);
		if(m_eTransportEncoding == SnaccTransportEncoding::JSON)
			strData = GetJsonAsnPrefix(strData) + strData;
		lRoseResult = SendBinaryDataBlockEx(strData.c_str(), (unsigned long)strData.length(), nullptr);
	}
	else
	{
		throw std::runtime_error("invalid m_eTransportEncoding");
	}

	//prevent delete of perror...
	ResultMsg.error = 0;

	return lRoseResult;
}

long SnaccROSEBase::SendError(int invokeID, SNACC::AsnType *value, const wchar_t* szSessionID)
{
	long lRoseResult = ROSE_NOERROR;

	ROSEError error;
	error.invokedID = invokeID;
	error.error_value = 0;
	error.error = new AsnAny;
	error.error->value = value;
	if (szSessionID)
		error.sessionID = new UTF8String(szSessionID);


	lRoseResult = SendError(&error);

	//prevent delete of value...
	error.error->value = 0;

	return lRoseResult;
}

void SnaccROSEBase::AddInvokeHeaderLog(std::stringstream& strOut, SNACC::ROSEInvoke* pInvoke)
{
	strOut << "InvokeMsg.invokeID = " << (int)pInvoke->invokeID << std::endl;
	strOut << "InvokeMsg.operationID = " << (int)pInvoke->operationID << std::endl;
	strOut << "InvokeMsg.operationName = " << SnaccRoseOperationLookup::LookUpName((int)pInvoke->operationID) << std::endl;

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

bool SnaccROSEBase::SetTransportEncoding(const SnaccTransportEncoding transportEncoding)
{
	if (transportEncoding == SnaccTransportEncoding::BER ||
		transportEncoding == SnaccTransportEncoding::JSON ||
		transportEncoding == SnaccTransportEncoding::JSON_NO_HEADING)
	{
		m_eTransportEncoding = transportEncoding;
		return true;
	}
	return false;
}

SnaccTransportEncoding SnaccROSEBase::GetTransportEncoding() const
{
	return m_eTransportEncoding;
}
