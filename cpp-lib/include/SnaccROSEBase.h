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

/*! Die Klasse SnaccROSEPendingOperation
	dient zur Realisierung der Funktionsaufrufe.
	Wird eine Funktion (Invoke) gerufen, so wird eine SnaccROSEPendingOperation
	angelegt. Kommt nun die entsprechende Antwort vom Server, so
	wird Anhand der invokeID die Instanz gefunden und der m_CompletedEvent ausgelöst. */
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

// Hilfsklasse für OperationID / Name lookup
// Alle generierten Interfaces registrieren Ihre OperationIDs in dieser Klasse
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
// Dieser Context wird allen OnInvoke_ Funktionen mit übergeben
// Dieser Context kann den Invoke_ Funktionen mit übergeben werden
// Die ROSEAuth Member werden automatisch gelöscht, Angelegt mit new
class SnaccInvokeContext
{
public:
	SnaccInvokeContext();
	virtual ~SnaccInvokeContext();

	// Bedeutung im OnInvoke_: Authentication Header aus ROSE Invoke (Pointer auf das Objekt im Invoke)
	// Bedeutung im Invoke_: Authentication Header der im Invoke mitgeschickt wird (muss mit new erzeugt werden, wird selbst aufgeräumt)
	SNACC::ROSEAuthRequest* pInvokeAuth;

	// Bedeutung im OnInvoke_: Authentication Header, der im Reject zurückgegeben wird falls die Methode returnReject zurückliefert muss mit new erzeugt werden, wird selbst aufgeräumt
	// Bedeutung im Invoke_: Falls die Methode returnReject zurückliefert enthält dieser Member die Optionale Reject Authentication (nur wenn SNACC_REJECT_AUTH_INCOMPLETE)
	SNACC::ROSEAuthResult* pRejectAuth;

	// Reject Result Code: 0 oder ROSE_REJECT_AUTHENTICATIONFAILED oder ROSE_REJECT_AUTHENTICATIONINCOMPLETE
	// Bedeutung im Invoke_: Falls die Methode returnReject zurückliefert enthält dieser Member ROSE_REJECT_AUTHENTICATIONINCOMPLETE oder ROSE_REJECT_AUTHENTICATIONFAILED, falls die Authentication nicht erfolgreich war
	// Special: ROSE_REJECT_ASYNCOPERATION - es wird kein Result versendet.
	long lRejectResult;

	// Bedeutung im OnInvoke_: Funktion, die gerufen wird, nachdem das Result zurückgeschickt wurde.
	// Verwendung: cxt->funcAfterResult = [this, strCrossRefID]() -> void { /* executed after result has been sent. */ };
	std::function<void()> funcAfterResult;

	// Bedeutung im OnInvoke_: Originaler Invoke
	SNACC::ROSEInvoke* pInvoke;

	// A Custom void pointer that allows the caller to add custom data to the transport layer that is dispatching a request
	void* pCustom;
};

enum class SnaccTransportEncoding
{
	// Transport Encoding Basic Encoding Rules (Binary)
	BER = 0,
	// Transport Encoding JSON (Text)
	JSON = 1,
	// Transport Encoding JSON (Text), but without a framing header (J0000XXX{...})
	JSON_NO_HEADING = 2
};

#define SNACC_TE_BER SnaccTransportEncoding::BER
#define SNACC_TE_JSON SnaccTransportEncoding::JSON

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

	/* Log output.
		Override to print the log data out
		All messages (in and out will be decoded to the logger */
	virtual void PrintToLog(const std::string& /* strOutput*/)
	{
		return;
	}

	/* Log level 0 oder 1
		bout=true for outgoing messages,
		bout=false for incoming messages,
		Override to set a different log level */
	virtual long GetErrorLogLevel()
	{
		return 0;
	}

	/* Log Errors
		Override to print Errors
	*/
	virtual void PrintToErrorLog(const std::string& /* strOutput*/)
	{
		return;
	}

	/* Set the Transport Encoding to be used */
	bool SetTransportEncoding(const SnaccTransportEncoding transportEncoding);
	SnaccTransportEncoding GetTransportEncoding() const;

	/* Send a Result Message. */
	virtual long SendResult(SNACC::ROSEResult* presult);

	/* Send a Result Message.
		Override from SnaccRoseSender */
	virtual long SendResult(int invokeID, SNACC::AsnType* value, const wchar_t* szSessionID = 0);

	/* Send a Reject Message. */
	long SendReject(SNACC::ROSEReject* preject);
	/* Send back reject message.
		Invoke Problem */
	long SendRejectInvoke(int invokeID, SNACC::InvokeProblem problem, const char* szError = NULL, const wchar_t* szSessionID = 0, SNACC::ROSEAuthResult* pAuthHeader = 0);

	/* Send a Error Message */
	long SendError(SNACC::ROSEError* perror);

	/* Send a Error Message.
		Override from SnaccRoseSender */
	virtual long SendError(int invokeID, SNACC::AsnType* value, const wchar_t* szSessionID = 0);

	/*! Increment invoke counter
		Override from SnaccRoseSender*/
	virtual long GetNextInvokeID();

	/* Log level 0 oder 1
		bout=true for outgoing messages,
		bout=false for incoming messages,
		Override to set a different log level
		Override from SnaccRoseSender*/
	virtual long GetLogLevel(bool /*bOut*/)
	{
		return 0;
	}

	/* used for printing alle the messages
		Override from SnaccRoseSender*/
	virtual void PrintAsnType(bool bOutbound, SNACC::AsnType* pType, SNACC::ROSEInvoke* pInvoke);

	// protected:
	/** The following function should only be called by the generated ROSE stub */
	/* Invoke a function.
		ppresult or pperror will be filled on return
		caller must delete returned result!
		returns NO_ERROR for Result
		iTimeout: Wartezeit für das Result.
		iTimeout : -1 default Wartezeit m_lMaxInvokeWait
		iTimeout : 0 keine Wartezeit
		iTimeout : 1 ... Wartezeit in Millisekunden
		Override from SnaccRoseSender
	*/
	virtual long SendInvoke(SNACC::ROSEInvoke* pinvoke, SNACC::ROSEResult** ppresult, SNACC::ROSEError** pperror, int iTimeout = -1, SnaccInvokeContext* cxt = nullptr);

	/* Send a Event Message.
		Override from SnaccRoseSender*/
	virtual long SendEvent(SNACC::ROSEInvoke* pinvoke, SnaccInvokeContext* cxt = nullptr);

protected:
	// ASN prefix mit länge für JSON bauen
	std::string GetJsonAsnPrefix(std::string& strJson);

	/*! Die functions and events.
		The implementation of this functions is contained in the generated code from the
		esnacc. */
	virtual long OnInvoke(SNACC::ROSEInvoke* pinvoke, SnaccInvokeContext* cxt) = 0;

	/* Function is called when a received data Packet cannot be decoded (invalid Rose Message) */
	virtual void OnRoseDecodeError(const char* /*lpBytes*/, unsigned long /*lSize*/, const std::string& /*strWhat */)
	{
	}

	/*! Add the invokeid and operationid the the log stream */
	void AddInvokeHeaderLog(std::stringstream& strOut, SNACC::ROSEInvoke* pInvoke);

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
	 * It´s only called if the implementer implemented OnInvokeContextCreated and returned true there
	 *
	 * @param pInvokeContext - the context that is about to get deleted
	 */
	virtual void OnInvokeContextRunsOutOfScope(SnaccInvokeContext* pInvokeContext){};

private:
	/*! The ROSE component messages.
		These are called from the OnROSEMessage.
		Do not dete the parameters. The functions are called
		before the CompleteOperation */
	virtual void OnInvokeMessage(SNACC::ROSEInvoke* pinvoke);
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

	void AddPendingOperation(int invokeID, SnaccROSEPendingOperation* pOperation);
	void RemovePendingOperation(int invokeID);
	/*! Pending Operation result has been received.
		Attention: The pMessage Objekt will be taken over from the
		SnaccROSEPendingOperation Object übernommen und und am Ende deleted. */
	bool CompletePendingOperation(int invokeID, SNACC::ROSEMessage* pMessage);
	void CompleteAllPendingOperations();

	SnaccROSEPendingOperationMap m_PendingOperations;

	std::map<int, int> m_mapMultithreadInvokeIDs;

	// Outbound Data Interface (optional)
	ISnaccROSETransport* m_pTransport = NULL;

	// Transport Encoding to be used
	SnaccTransportEncoding m_eTransportEncoding = SnaccTransportEncoding::BER;

	// Get Length of JSON Header J1235{} )
	int GetJsonHeaderLen(const char* lpBytes, unsigned long iLength);
};

#endif //_SnaccROSEBase_h_