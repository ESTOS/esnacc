#ifndef _SnaccROSEBase_h_
#define _SnaccROSEBase_h_

/*! Requires std::map */
#include <set>
#include <map>
#include <memory>
#include <functional>
#include <string>
#include <mutex>
#include <wchar.h>
#include <optional>
#include "SnaccROSEInterfaces.h"
#include "SnaccTelemetry.h"
#include "syncevent.h"

#if defined(_MSC_VER)
#define HAS_WCHAR_T
#define LOG_CHARTYPE wchar_t
#else
#define LOG_CHARTYPE char
#endif

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
	SnaccROSEPendingOperation(long lInvokeID, unsigned int uiOperationID, const char* szOperationName);

public:
	static std::unique_ptr<SnaccROSEPendingOperation> Create(long lInvokeID, unsigned int uiOperationID, const char* szOperationName)
	{
		return std::unique_ptr<SnaccROSEPendingOperation>(new SnaccROSEPendingOperation(lInvokeID, uiOperationID, szOperationName));
	}

	~SnaccROSEPendingOperation();

	const long m_lInvokeID;
	const unsigned int m_uiOperationID{};
	const std::string m_strOperationName;

	/*! The answer message. */
	const SNACC::ROSEMessage* m_pAnswerMessage;

	/*! Error code (one of the ROSE_ error codes. */
	long m_lRoseResult;

	/*! Encoded size of the received response payload. */
	size_t m_stResponseData{};

	/*! Outbound invoke telemetry tracked alongside the pending completion state. */
	std::shared_ptr<SnaccTelemetryData> m_pTelemetry;

	/*! Async Operation completed.
		Attention: The AnswerMessage will not be copied.
		The AnswerMessage must be new allocated and will be deleted
		when processed. */
	void CompleteOperation(long lRoseResult, const SNACC::ROSEMessage* pAnswerMessage, size_t stResponseData = 0);
	void FinalizeTelemetry(long lFinalRoseResult, std::shared_ptr<SnaccInvokeContext> pctx);

	/*! Wait for answer received. */
	bool WaitForComplete(long lTimeOut = -1);
};

typedef std::map<long, std::unique_ptr<SnaccROSEPendingOperation>> SnaccROSEPendingOperationMap;

class SnaccROSEBase;

// Helper class for OperationID / Name lookup
// All generated interfaces register their OperationIDs in this class
class SnaccRoseOperationLookup
{
public:
	static const char* LookUpName(unsigned int uiOpID);
	static unsigned int LookUpID(const char* szOpName);
	static void RegisterOperation(unsigned int uiOpID, const char* szOpName, unsigned int uiInterfaceID);
	static unsigned int LookUpInterfaceID(unsigned int uiOpID);
	// check if at least one Operation has been registerd
	static bool Initialized();
	// Cleanup all registered Operations (can be called during shutdown)
	static void CleanUp();

private:
	static inline std::map<std::string, unsigned int> m_mapOpToID;
	static inline std::map<unsigned int, std::string> m_mapIDToOp;
	static inline std::map<unsigned int, unsigned int> m_mapIDToInterface;
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
class SnaccROSEBase : public SnaccROSESender, public SnaccTelemetryCallback
{
public:
	SnaccROSEBase(const wchar_t* szClassName);
	SnaccROSEBase(const wchar_t* szClassName, const std::set<int>& multithreadInvokeIDs);
	virtual ~SnaccROSEBase(void);

	/*
	 * Allows to set a central callback for telemetry data the SnaccROSEBase class is providing
	 * The implementer can either overwrite OnInvokeProcessed in this class or set
	 * the central callback to gather telemetry data for inbound and outbound messages.
	 */
	static void SetTelemetryCallback(SnaccTelemetryCallback* pCallBack);

	/*! Set timeout for function invokes.
		Wait time in milliseconds */
	void SetMaxInvokeWaitTime(long lMaxInvokeWait);

	/*! Shutdown.
		Call this function to stop processing any more Invokes.
		All pending operations will be completed and new function calls will be blocked.
		All Functions return a ROSE_TE_SHUTDOWN */
	void StopProcessing(bool bStop = true);

	/*! Input of the binary data
		This processes results and Invokes that are in the list of MultiThreadedInvokeIDs.
		It is used for the Single thread mode.
		First call this function to complete pending invokes and if not processed
		call OnBinaryDataBlock.
		The data should be pure ASN.1 data without header.
		There must be one call for each ROSEMessage.*/
	bool OnBinaryDataBlockResult(const char* lpBytes, unsigned long lSize, bool bLogTransportData = true);

	/*! Input of the binary data.
		The data should be pure ASN.1 data without header.
		There must be one call for each ROSEMessage. */
	void OnBinaryDataBlock(const char* lpBytes, unsigned long lSize, bool bLogTransportData = true);

	/* Output of the binary data.
		Override this function to send the data to the transport layer.
		Optional you can set a ISnaccROSETransport Interface to receive the outbound data*/
	virtual long SendBinaryDataBlockEx(const char* lpBytes, size_t Size, SnaccInvokeContext& ctx);

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
	int ConfigureFileLogging(const LOG_CHARTYPE* szPath, const bool bAppend = true, const bool bFlushEveryWrite = true);

	/* Retrieve the log level - do we need to log something */
	virtual SNACC::EAsnLogLevel GetLogLevel(const bool bOutbound) override = 0;

	/*! Writes JSON encoded log messages to the log file
		bOutbound = true in case the log entry is related to an outbound message
		bException = true in case this is an exception based error message
		szOperationName = the operationName beeing called (if available)
		szData = the JSON (!) encoded log message. (!) The message is no necessarily null terminated (!)
		length = length of the szData (as it is not necessarily null terminated respect the length!)
		returns true if data was logged
	*/
	virtual bool PrintJSONToLog(const bool bOutbound, const bool bException, const char* szOperationName, const char* szData, const size_t size = 0);

	/* Set the Transport Encoding to be used */
	bool SetTransportEncoding(const SNACC::TransportEncoding transportEncoding);
	SNACC::TransportEncoding GetTransportEncoding() const;

	/*
	 * Encodes a result as repsonse to an invoke
	 *
	 * uiInvokeID - the inbound invoke that we send the error for
	 * pResult - the actual result object
	 * strResponse - the encoded response data to send via the transport layer
	 * szSessionID - the SessionID (this propery is filled by subclassing from the concrete class in case we are handling multiple clients via one connection)
	 */
	virtual long EncodeResult(unsigned int uiInvokeID, SNACC::AsnType* pResult, std::string& strResponse, const wchar_t* szSessionID = nullptr) override;

	/* Send a Reject Message. */
	long EncodeReject(SNACC::ROSEReject* preject, std::string& strResponse);
	long SendRejectEx(SNACC::ROSEReject* preject, SnaccInvokeContext& ctx);

	/*
	 * Encodes a reject as repsonse to an invoke
	 *
	 * uiInvokeID - the inbound invoke that we send the error for
	 * problem - the reject cause
	 * strResponse - the encoded response data to send via the transport layer
	 * szError - an optional error description (basically the problem as text, used for json encoded results)
	 * szSessionID - the SessionID as taken from the invoke
	 * pAuthHeader - an optional Auth header
	 */
	long EncodeRejectInvoke(unsigned int uiInvokeID, SNACC::InvokeProblem problem, std::string& strResponse, const char* szError = nullptr, const wchar_t* szSessionID = nullptr, SNACC::ROSEAuthResult* pAuthHeader = nullptr);
	long EncodeInvokeRejectResponse(const SNACC::ROSEInvoke* pInvoke, long lProtocolResult, SnaccInvokeContext& ctx, std::string& strResponse);

	/*
	 * Encodes an error as repsonse to an invoke
	 *
	 * uiInvokeID - the inbound invoke that we send the error for
	 * pError - the actual error object
	 * strResponse - the encoded response data to send via the transport layer
	 * szSessionID - the SessionID (this propery is filled by subclassing from the concrete class in case we are handling multiple clients via one connection)
	 */
	virtual long EncodeError(unsigned int uiInvokeID, SNACC::AsnType* pError, std::string& strResponse, const wchar_t* szSessionID = nullptr) override;

	/*! Increment invoke counter
		Override from SnaccRoseSender*/
	virtual long GetNextInvokeID() override;

	/**
	 * Print the object pType to the log output
	 *
	 * bOutbound = true if the data is sent, or false if it was received
	 * encoding = the encoding that will be used for sending or was detected while receiving
	 * szOperationName = name of the operation beeing called (if available)
	 * szData = the plain transport data, pretty much uninterpreted (only the length heading is removed)
	 * size = the size of the plain transport data
	 * pMSg = the asn1 message that was encoded or decoded from the transport payload
	 * pParsedValue = only for inboud data, the parsed json object (required if the logging shall get pretty printed, saves another parsing of the payload), only available inbound and if the transport is using json
	 * Returns true if data was logged
	 */
	virtual bool LogTransportData(const bool bOutbound, const SNACC::TransportEncoding encoding, const char* szOperationName, const char* szData, const size_t size, const SNACC::ROSEMessage* pMSg, const SJson::Value* pParsedValue) override;

	/**
	 * An invoke that is send to the other side. Should only be called by the ROSE stub itself generated files
	 *
	 * pinvoke - the invoke payload (it is put into a ROSEMessage in the function)
	 * result - decoded result payload in case a result response is received
	 * error - decoded error payload in case an error response is received
	 * szOperationName - the operationName (for logging purposes)
	 * iTimeout - the timeout in milliseconds (-1 uses default m_lMaxInvokeWait, 0 returns immediately without waiting for the result)
	 * pCtx - contextual data for the invoke. The caller may keep another shared reference
	 *        to inspect changes after the call.
	 */
	virtual long SendInvoke(SNACC::ROSEInvoke* pinvoke, SNACC::AsnType* result, SNACC::AsnType* error, const char* szOperationName, int iTimeout = -1, std::shared_ptr<SnaccInvokeContext> pCtx = {}) override;

	/**
	 * Encodes the result or error from an OnInvoke request. Retrieves the result or error from the response
	 *
	 * invokeResult - the result of the OnInvoke method
	 * pInvoke - the original invoke
	 * ctx - contextual data for the invoke
	 * strResponse - the encoded result to put it on the transport
	 * pResult - the result object (Base type pointer, the caller of the invoke provides the proper type)
	 * pError - the error object (Base type pointer, the caller of the invoke provides the proper type)
	 */
	virtual long HandleOnInvokeResult(SNACC::InvokeResult invokeResult, const SNACC::ROSEInvoke* pInvoke, SnaccInvokeContext& ctx, std::string& strResponse, SNACC::AsnType* pResult, SNACC::AsnType* pError) override;

	/**
	 * Allows implementers to customize the decoded invoke response before it is handed back
	 * to the caller, for example to propagate connection-specific session data.
	 *
	 * lRoseResult - the current ROSE result code
	 * pResponseMsg - the raw response message that was received
	 * result - the decoded result payload in case a result response is received
	 * error - the decoded error payload in case an error response is received
	 * ctx - contextual data for the invoke
	 */
	virtual long HandleInvokeResult(long lRoseResult, const SNACC::ROSEMessage* pResponseMsg, SNACC::AsnType* result, SNACC::AsnType* error, SnaccInvokeContext& ctx) override;

	/**
	 * An event (invoke without result) that is send to the other side. Should only be called by the ROSE stub itself generated files
	 *
	 * pinvoke - the invoke payload (it is put into a ROSEMessage in the function)
	 * pCtx - contextual data for the invoke. The caller may keep another shared reference
	 *        to inspect changes after the call.
	 */
	virtual long SendEvent(SNACC::ROSEInvoke* pinvoke, const char* szOperationName, std::shared_ptr<SnaccInvokeContext> pCtx = {}) override;

	/**
	 * Creates the invoke context for inbound and outbound invoke lifecycles.
	 */
	std::shared_ptr<SnaccInvokeContext> CreateInvokeContext(const SnaccInvokeContextInit& init) override;

	/**
	 * Decodes an invoke and properly handles logging for it
	 *
	 * pInvokeMessage - the invoke message as provided from the other side
	 * argument - the argument object (Base type pointer, the caller of the provides the proper type)
	 */
	long DecodeInvoke(const SNACC::ROSEMessage* pInvokeMessage, SNACC::AsnType* argument) override;

protected:
	// Get the length prefix for a given strJson payload
	// In case the message is longer than 9999999 which is the longest possible length returns ROSE_TE_ENCODE_FAILED
	long GetJsonLengthPrefix(std::string_view strJson, std::string& strLenghtPrefix) const;

	/*! The functions and events.
		The implementation of this functions is contained in the generated code from the
		esnacc. */
	virtual long OnInvoke(SNACC::ROSEMessage* pMsg, SnaccInvokeContext& ctx, std::string& strResponse) = 0;

	/* Function is called when a received data Packet cannot be decoded (invalid Rose Message)
	 * bAlreadyTransportLogged	- true if the transport data has already been logged in the transport log (so we do not need to log it again)
	 * encoding	- the encoding that was used for the transport
	 * lpBytes - the data that could not get decoded
	 * ulSize - the size of the data that could not get decoded
	 * strWhat - the error message that was generated by the decoder
	 */
	virtual void OnRoseDecodeError(const bool bAlreadyTransportLogged, const SNACC::TransportEncoding encoding, const char* lpBytes, unsigned long ulSize, const std::string& strWhat)
	{
	}

	/*! The decoded message.
		pmessage is new allocated and must be deleted inside this function.
		returns true if the message was processed.
		set bAllowInvokes to false, if invokes are not processed. */
	virtual bool OnROSEMessage(SNACC::ROSEMessage* pmessage, bool bAllowInvokes, unsigned long ulMessageSize);

	/*
	 * This callback provides telemetry data for processed ROSE messages.
	 */
	void OnInvokeProcessed(std::shared_ptr<const SnaccTelemetryData> data) override;

private:
	/*! The ROSE component messages.
		These are called from the OnROSEMessage.
		Do not dete the parameters. The functions are called
		before the CompleteOperation */
	virtual void OnInvokeMessage(SNACC::ROSEMessage* pMessage, unsigned long lMessageSize);
	virtual void OnResultMessage(SNACC::ROSEResult* pResult, unsigned long lMessageSize);
	virtual void OnErrorMessage(SNACC::ROSEError* pError, unsigned long lMessageSize);
	virtual void OnRejectMessage(SNACC::ROSEReject* pReject, unsigned long lMessageSize);

	// The central process wide telemetry callback
	static inline SnaccTelemetryCallback* m_pTelemetryCallback{};

	/* The name of the concrete class. This is used for the static global telemetry callback when reporting telemetry to differ between different SnaccROSEBase inheriting classes */
	const std::wstring m_strClassName;
	/*! Maximum wait time (milliseconds) for a function to return default: 20000 ms*/
	long m_lMaxInvokeWait{20000};
	/*! Are we currently active or was StopProcessing called */
	bool m_bProcessingAllowed{true};
	/*! The counter for the InvokeIds */
	long m_lInvokeCounter{};
	/*! The guard for m_bProcessingAllowed and SnaccROSEPendingOperationMap */
	std::mutex m_InternalProtectMutex;
	/*! If set, the open file where we append log data to. The pointer is created with ConfigureFileLogging() */
	FILE* m_pAsnLogFile{};
	/*! true if the logfile already contains data, false if not */
	bool m_bAsnLogFileContainsData{};
	/*! true if every write to the logger shall get flushed */
	bool m_bFlushEveryWrite{};
	/*! Mutex to access the logging function (serialize multiple threads to it) */
	std::mutex m_mtxLogFile;

	SnaccROSEPendingOperation& AddPendingOperation(int invokeID, unsigned int uiOperationID, const char* szOperationName);
	void RemovePendingOperation(int invokeID);
	/*! Pending Operation result has been received.
		Attention: The pMessage Objekt will be taken over from the
		SnaccROSEPendingOperation Object and deleted in the end. */
	bool CompletePendingOperation(int invokeID, const SNACC::ROSEMessage* pMessage, unsigned long ulMessageSize);
	bool GetPendingOperationTelemetryInfo(int invokeID, unsigned int& uiOperationID, std::string& strOperationName);
	static long GetRejectResultCode(const SNACC::ROSEReject* pReject);
	static long GetRejectResultCode(const SNACC::ROSEReject* pReject, SnaccInvokeContext& ctx);
	static long DecodeResponse(const SNACC::ROSEMessage* pResponse, SNACC::ROSEResult** ppResult, SNACC::ROSEError** ppError, SnaccInvokeContext& ctx);
	void CompleteAllPendingOperations();
	bool IsProcessingAllowed();

	SnaccROSEPendingOperationMap m_PendingOperations;

	// Operations that are handled multithreaded, event the application itself is single threaded
	// This member has to be set while initializing the class as it is not thread save
	const std::set<int> m_multithreadInvokeIDs;

	// Outbound Data Interface (optional)
	ISnaccROSETransport* m_pTransport{};

	// Transport Encoding to be used
	SNACC::TransportEncoding m_eTransportEncoding{SNACC::TransportEncoding::UNKNOWN};

	// Detects the encoding from the transport data (used for inbound data)
	SNACC::TransportEncoding DetectEncoding(const char* lpBytes, unsigned long ulSize) const;

	// Get Length of JSON Header J1235{} )
	int GetJsonHeaderLen(const char* lpBytes, unsigned long iLength);

	// Encodes a value based on the transportencoding and loglevel
	std::string GetEncoded(const SNACC::TransportEncoding encoding, SNACC::AsnType* pValue, unsigned long* pUlSize = nullptr);

	/**
	 * The combined method that takes care about encoding and sending an event or invoke
	 *
	 * pInvoke - the invoke to send
	 * szOperationName - the operationName (for logging purposes)
	 * ctx - contextual data for the invoke
	 */
	long Send(SNACC::ROSEInvoke* pInvoke, const char* szOperationName, SnaccInvokeContext& ctx, size_t* pstRequestData = nullptr);
};

#endif //_SnaccROSEBase_h_