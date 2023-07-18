#ifndef _SnaccROSEBase_h_
#define _SnaccROSEBase_h_

/*! Requires std::map */
#include <map>
#include <functional>
#include <string>
#include <mutex>
#include "SnaccROSEInterfaces.h"
#include "syncevent.h"

namespace SNACC
{
	class AsnType;

	class ROSEMessage;
	class ROSEInvoke;
	class ROSEResult;
	class ROSEError;
	class ROSEReject;
	class InvokeProblem;
	class ROSEAuthRequest;
	class ROSEAuthResult;
} // namespace SNACC

/*! The SnapcROSEPendingOperation class is used to implement the function calls.
	If a function (Invoke) is called, a SnapcROSEPendingOperation created.
	If the corresponding answer now comes from the server, 	the instance is found
	based on the invokeID and the m_CompletedEvent is triggered. */
class SnaccROSEPendingOperation
{
private:
	SyncEvent m_CompletedEvent;

public:
	SnaccROSEPendingOperation();
	~SnaccROSEPendingOperation();

	long m_lInvokeID;

	/*! The answer message. */
	SNACC::ROSEMessage* m_pAnswerMessage;

	/*! Error code (one of the ROSE_ error codes. */
	long m_lRoseResult;

	/*! Async Operation completed.
		Attention: The AnswerMessage will not be copied.
		The AnswerMessage must be new allocated and will be deleted
		when processed. */
	void CompleteOperation(long lRoseResult, SNACC::ROSEMessage* pAnswerMessage);

	/*! Wait for answer received. */
	bool WaitForComplete(long lTimeOut = -1);
};

typedef std::map<long, SnaccROSEPendingOperation*> SnaccROSEPendingOperationMap;
typedef std::pair<long, SnaccROSEPendingOperation*> SnaccROSEPendingOperationPair;

class SnaccROSEBase;

// Helper class for OperationID / Name lookup
// All generated interfaces register their OperationIDs in this class
class SnaccRoseOperationLookup
{
public:
	static std::string LookUpName(int iOpID);
	static int LookUpID(const char* szOpName);
	static void RegisterOperation(int iOpID, const char* szOpName, int iInterfaceID);
	static int LookUpInterfaceID(int iOpID);
	// check if at least one Operation has been registerd
	static bool Initialized();
	// Cleanup all registered Operations (can be called during shutdown)
	static void CleanUp();

private:
	static std::map<std::string, int> m_mapOpToID;
	static std::map<int, std::string> m_mapIDToOp;
	static std::map<int, int> m_mapIDToInterface;
};

// Invoke Context
// This context is passed to all OnInvoke_ functions
// This context can be transferred to the Invoke_ functions
// The ROSEAuth members are automatically deleted, created with new
class SnaccInvokeContext
{
public:
	SnaccInvokeContext();
	virtual ~SnaccInvokeContext();

	// Meaning in OnInvoke_: Authentication Header from the ROSE Invoke (Pointer to the object in the invoke)
	// Meaning in Invoke_: Authentication Header that is dispatched along with the invoke (create it with new, cleanup is done inside)
	SNACC::ROSEAuthRequest* pInvokeAuth;

	// Meaning in OnInvoke_: Authentication Header, which is returned in the reject if the method returnReject returns must be created with new, is cleaned up itself
	// Meaning in Invoke_: If the method returnsReject, this member contains the optional reject authentication (only if SNACC_REJECT_AUTH_INCOMPLETE)
	SNACC::ROSEAuthResult* pRejectAuth;

	// Reject Result Code: 0 or ROSE_REJECT_AUTHENTICATIONFAILED or ROSE_REJECT_AUTHENTICATIONINCOMPLETE
	// Meaning in Invoke_: If the method returns returnReject, this member contains ROSE_REJECT_AUTHENTICATIONINCOMPLETE or ROSE_REJECT_AUTHENTICATIONFAILED if the authentication was not successful
	// Special: ROSE_REJECT_ASYNCOPERATION - no result is sent.
	long lRejectResult;

	// Meaning in OnInvoke_: Function that is called after the result has been returned.
	// Usage: cxt->funcAfterResult = [this, strCrossRefID]() -> void { /* executed after result has been sent. */ };
	std::function<void()> funcAfterResult;

	// Meaning in OnInvoke_: Originaler Invoke
	SNACC::ROSEInvoke* pInvoke;

	// A Custom void pointer that allows the caller to add custom data to the transport layer that is dispatching a request
	void* pCustom;
};

#define SNACC_TE_BER SNACC::TransportEncoding::BER
#define SNACC_TE_JSON SNACC::TransportEncoding::JSON

/*! SnaccROSEBase implements the ROSE (ASN.1) protocol.

	To use this class do one of the following:
	1. Derive it
		Usage:
		- OnBinaryDataBlock should be called with the binary data blocks, that have been received
							by the transport layer (e.g. TCP - Inbound)
		- virtual SendBinaryDataBlockEx should be overwritten and the data should be submitted (e.g. TCP Outbound)
	2. Set a ISnaccROSETransport Interface for sending outbound data
		call SetSnaccROSETransport
		Implement the ISnaccROSETransport:
		- virtual SendBinaryDataBlockEx should be overwritten and the data should be submitted (e.g. TCP Outbound)

	- During shutdown call StopProcessing to complete any pending operations.

	Dependencies:
	-Snacc lib

	This class is required for the automatically generated code for
	ROSE OPERATIONS. (command line -R using esnacc).

	The generated class overrides OnInvoke to implement the OnInvoke_XXX handlers.
	The generated class calls the Invoke function from the Invoke_XXX functions.
	*/
class SnaccROSEBase : public SnaccROSESender
{
public:
	SnaccROSEBase(void);
	virtual ~SnaccROSEBase(void);

	/*! Set timeout for function invokes.
		Wait time in milliseconds */
	void SetMaxInvokeWaitTime(long lMaxInvokeWait);

	/*! Shutdown.
		Call this function to stop processing any more Invokes.
		All pending operations will be completed and new function calls will be blocked.
		All Functions return a ROSE_TE_SHUTDOWN */
	void StopProcessing(bool bStop = true);

	/*! Set OperationsIDs of Invokes, that will be processed by OnBinaryDataBlockResult.
		Allows special Invokes to be processed Multi threaded even if SingleThread Operation is selected
		This is only allowed to be set once during initialization!
		member map is not protected by locks! */
	void SetMultithreadInvokeIds(std::map<int, int> mapIds);

	/*! Input of the binary data
		This processes results and Invokes that are in the list of MultiThreadedInvokeIDs.
		It is used for the Single thread mode.
		First call this function to complete pending invokes and if not processed
		call OnBinaryDataBlock.
		The data should be pure ASN.1 data without header.
		The must be one call for each ROSEMessage. */
	bool OnBinaryDataBlockResult(const char* lpBytes, unsigned long lSize);

	/*! Input of the binary data.
		The data should be pure ASN.1 data without header.
		The must be one call for each ROSEMessage. */
	void OnBinaryDataBlock(const char* lpBytes, unsigned long lSize);

	/* Output of the binary data.
		Override this function to send the data to the transport layer.
		Optional you can set a ISnaccROSETransport Interface to receive the outbound data*/
	virtual long SendBinaryDataBlockEx(const char* lpBytes, unsigned long lSize, SnaccInvokeContext* pCtx);

	/*! Set a ISnaccROSETransport Interface to be called when outbound Data must be sent.
		This is an alternative to overriding SendBinaryDataBlock */
	void SetSnaccROSETransport(ISnaccROSETransport* pTransport);

	/*! Enables or disables file logging for this instance of the SnaccROSEBase.
	 *  Call this function with filepath to enable logging from the library side
	 *  Call this function with a nullpointer to disable logging from the library side
	 *  Furthermore the call to GetLogLevel needs to provide the proper loglevel (what and how to write it)
	 *
	 *  szPath - the path to write to, also enabled logging, or a nullpointer to disable logging
	 *  bAppend - true to append data to an existing logfile, or false to overwrite any existing logfile
	 *  bFlushEveryWrite - true flushes every write operation (ideal for debugging), false to let the os decide when to flush
	 *
	 *  returns NO_ERROR on success (logfile opened, logfile closed, nothing to do) or an error value */
	int ConfigureFileLogging(const wchar_t* szPath, const bool bAppend = true, const bool bFlushEveryWrite = false);

	/* Retrieve the log level - do we need to log something */
	virtual SNACC::EAsnLogLevel GetLogLevel(const bool bOutbound) = 0;

	/*! Writes JSON encoded log messages to the log file
		bOutbound = true in case the log entry is related to an outbound message
		bError = true in case this is an error message
		szData = the JSON (!) encoded log message. (!) The message is no necessarily null terminated (!)
		length = length of the szData (as it is not necessarily null terminated respect the length!)
	*/
	virtual void PrintJSONToLog(const bool bOutbound, const bool bError, const char* szData, const size_t length = 0);

	/* Set the Transport Encoding to be used */
	bool SetTransportEncoding(const SNACC::TransportEncoding transportEncoding);
	SNACC::TransportEncoding GetTransportEncoding() const;

	/* Send a Result Message. */
	virtual long SendResult(SNACC::ROSEResult* presult);

	/* Send a Result Message.
		Override from SnaccRoseSender */
	virtual long SendResult(const SNACC::ROSEInvoke* pInvoke, SNACC::AsnType* pResult, const wchar_t* szSessionID = 0) override;

	/* Send a Reject Message. */
	long SendReject(SNACC::ROSEReject* preject);
	/* Send back reject message.
		Invoke Problem */
	long SendRejectInvoke(int invokeID, SNACC::InvokeProblem problem, const char* szError = NULL, const wchar_t* szSessionID = 0, SNACC::ROSEAuthResult* pAuthHeader = 0);

	/* Send a Error Message */
	long SendError(SNACC::ROSEError* perror);

	/* Send a Error Message.
		Override from SnaccRoseSender */
	virtual long SendError(const SNACC::ROSEInvoke* pInvoke, SNACC::AsnType* pError, const wchar_t* szSessionID = 0) override;

	/*! Increment invoke counter
		Override from SnaccRoseSender*/
	virtual long GetNextInvokeID() override;

	/**
	 * Print the object pType to the log output
	 *
	 * bOutbound = true if the data is sent, or false if it was received
	 * encoding = the encoding that will be used for sending or was detected while receiving
	 * szData = the plain transport data, pretty much uninterpreted (only the length heading is removed)
	 * size = the size of the plain transport data
	 * pMSg = the asn1 message that was encoded or decoded from the transport payload
	 * pParsedValue = only for inboud data, the parsed json object (required if the logging shall get pretty printed, saves another parsing of the payload), only available inbound and if the transport is using json
	 */
	virtual void LogTransportData(const bool bOutbound, const SNACC::TransportEncoding encoding, const char* szData, const size_t size, const SNACC::ROSEMessage* pMSg, const SJson::Value* pParsedValue) override;

	/**
	 * An invoke that is send to the other side. Should only be called by the ROSE stub itself generated files
	 *
	 * pInvoke - the invoke payload (it is put into a ROSEMessage in the function)
	 * pResponse - the response payload (is handled afterwards in the HandleInvokeResult method
	 * iTimeout - the timeout (-1 is default m_lMaxInvokeWait, 0 return immediately (don´t care about the result))
	 * iTimeout - the timeout (-1 is default m_lMaxInvokeWait, 0 return immediately (don´t care about the result))
	 * pCtx - contextual data for the invoke
	 */
	virtual long SendInvoke(SNACC::ROSEInvoke* pinvoke, SNACC::ROSEMessage** pResponse, int iTimeout = -1, SnaccInvokeContext* pCtx = nullptr) override;

	/**
	 * Handles the response payload of the SendInvoke method. Retrieves the result or error from the response
	 *
	 * lRoseResult - the result of the SendInvoke method
	 * pResponseMsg - the response message as provided by the SendInvoke method
	 * result - the result object (Base type pointer, the caller of the invoke provides the proper type)
	 * error - the error object (Base type pointer, the caller of the invoke provides the proper type)
	 * pCtx - contextual data for the invoke
	 */
	virtual long HandleInvokeResult(long lRoseResult, SNACC::ROSEMessage* pResponseMsg, SNACC::AsnType* result, SNACC::AsnType* error, SnaccInvokeContext* cxt) override;

	/** An event (invoke without result) that is send to the other side. Should only be called by the ROSE stub itself generated files
	 *
	 * pInvoke - the invoke payload (it is put into a ROSEMessage in the function)
	 * pCtx - contextual data for the invoke
	 */
	virtual long SendEvent(SNACC::ROSEInvoke* pinvoke, SnaccInvokeContext* cxt = nullptr) override;

	/**
	 * Helper method that retrieves the proper object (result, error, reject) from the repsonse Message
	 *
	 * pResponseMsg - the response message as provided by the SendInvoke method
	 * ppResult - filled with the result object in case we have received a result
	 * ppError - filled with the error object in case we have received an error
	 * pCtx - contextual data from the invoke
	 */
	static long DecodeResponse(const SNACC::ROSEMessage* pResponse, SNACC::ROSEResult** ppResult, SNACC::ROSEError** ppError, SnaccInvokeContext* pCtx);

	/**
	 * Decodes an invoke and properly handles logging for it
	 *
	 * pInvokeMessage - the invoke message as provided from the other side
	 * argument - the argument object (Base type pointer, the caller of the provides the proper type)
	 */
	long DecodeInvoke(SNACC::ROSEMessage* pInvokeMessage, SNACC::AsnType* argument);

protected:
	// ASN prefix with length prefix to build the JSON message
	std::string GetJsonAsnPrefix(std::string& strJson);

	/*! Die functions and events.
		The implementation of this functions is contained in the generated code from the
		esnacc. */
	virtual long OnInvoke(SNACC::ROSEMessage* pMsg, SnaccInvokeContext* cxt) = 0;

	/* Function is called when a received data Packet cannot be decoded (invalid Rose Message) */
	virtual void OnRoseDecodeError(const char* /*lpBytes*/, unsigned long /*lSize*/, const std::string& /*strWhat */)
	{
	}

	/*! The decoded message.
		pmessage is new allocated and must be deleted inside this function.
		returns true if the message was processed.
		set bAllowInvokes to false, if invokes are not processed. */
	virtual bool OnROSEMessage(SNACC::ROSEMessage* pmessage, bool bAllowInvokes);

	/*
	 * This callback allows the implementer to enrich the invokecontext with data in case it wants to add or tune it
	 *
	 * @param pInvokeContext - the context that has just been created (some properties are already filled such as the pInvoke and the pInvokeAuth)
	 * @return true in case you implement the fuction (the stub will then call)
	 */
	virtual bool OnInvokeContextCreated(SnaccInvokeContext* pInvokeContext)
	{
		return false;
	};

	/*
	 * This callback is called before the invokeContext runs out of scope.
	 * It's only called if the implementer implemented OnInvokeContextCreated and returned true there
	 *
	 * @param pInvokeContext - the context that is about to get deleted
	 */
	virtual void OnInvokeContextRunsOutOfScope(SnaccInvokeContext* pInvokeContext){};

private:
	/*! The ROSE component messages.
		These are called from the OnROSEMessage.
		Do not dete the parameters. The functions are called
		before the CompleteOperation */
	virtual void OnInvokeMessage(SNACC::ROSEMessage* pMsg);
	virtual void OnResultMessage(SNACC::ROSEResult* presult);
	virtual void OnErrorMessage(SNACC::ROSEError* perror);
	virtual void OnRejectMessage(SNACC::ROSEReject* preject);

	/*! Maximum wait time (milliseconds) for a function to return default: 20000 ms*/
	long m_lMaxInvokeWait = 20000;
	/*! Are we currently active or was StopProcessing called */
	bool m_bProcessingAllowed = false;
	/*! The counter for the InvokeIds */
	long m_lInvokeCounter = 0;
	/*! The guard for m_bProcessingAllowed and SnaccROSEPendingOperationMap */
	std::mutex m_InternalProtectMutex;
	/*! If set, the open file where we append log data to. The pointer is created with ConfigureFileLogging() */
	FILE* m_pAsnLogFile = nullptr;
	/*! true if the logfile already contains data, false if not */
	bool m_bAsnLogFileContainsData = false;
	/*! true if every write to the logger shall get flushed */
	bool m_bFlushEveryWrite = false;
	/*! Mutex to access the logging function (serialize multiple threads to it) */
	std::mutex m_mtxLogFile;

	void AddPendingOperation(int invokeID, SnaccROSEPendingOperation* pOperation);
	void RemovePendingOperation(int invokeID);
	/*! Pending Operation result has been received.
		Attention: The pMessage Objekt will be taken over from the
		SnaccROSEPendingOperation Object and deleted in the end. */
	bool CompletePendingOperation(int invokeID, SNACC::ROSEMessage* pMessage);
	void CompleteAllPendingOperations();

	SnaccROSEPendingOperationMap m_PendingOperations;

	std::map<int, int> m_mapMultithreadInvokeIDs;

	// Outbound Data Interface (optional)
	ISnaccROSETransport* m_pTransport = NULL;

	// Transport Encoding to be used
	SNACC::TransportEncoding m_eTransportEncoding = SNACC::TransportEncoding::BER;

	// Get Length of JSON Header J1235{} )
	int GetJsonHeaderLen(const char* lpBytes, unsigned long iLength);

	// Encodes a value based on the transportencoding and loglevel
	std::string GetEncoded(const SNACC::TransportEncoding encoding, SNACC::AsnType* pValue, unsigned long* pUlSize = nullptr);
};

#endif //_SnaccROSEBase_h_