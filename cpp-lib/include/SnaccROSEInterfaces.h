#ifndef _SnaccROSEInterfaces_h_
#define _SnaccROSEInterfaces_h_

#include <functional>
#include <memory>
#include <string>

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

	enum class InvokeResult
	{
		returnResult = 0,
		returnError = 1,
		returnReject = 2
	};

	enum class TransportEncoding
	{
		// An inbound connection initially has UNKNOWN until the stack has determined the encoding
		// An outbound connection must set the encoding, otherwise the invoke will get rejected
		UNKNOWN = 0,
		// Transport Encoding Basic Encoding Rules (Binary)
		BER = 1,
		// Transport Encoding JSON (Text)
		JSON = 2,
		// Transport Encoding JSON (Text), but without a framing header (J0000XXX{...})
		JSON_NO_HEADING = 3
	};

	enum class EAsnLogLevel
	{
		// No logging
		DISABLED = 0,
		// JSON pretty printed output
		JSON = 1,
		// BER encoded data (hex with spaces)
		BER = 2,
		// JSON and BER combined
		JSON_AND_BER = 3,
		// JSON (always pretty printed)
		// Special case for inbound plain json data. If no newline is found in the payload the data is parsed and pretty printed
		// TAKE CARE: This CHANGES (!) the inbound data as it is reintpreted. If you are looking for json issues you should NOT use this option
		JSON_ALWAYS_PRETTY_PRINTED = 4
	};

} // namespace SNACC

namespace SJson
{
	class Value;
}

//! Error definitions
/*! During Invoke NOERROR means that a ROSEResult has been received. */
const long ROSE_NOERROR = 0;

// Bits in Error Code
//! 0101 0101 0101 0101 0101 0101 0101 0101
//!                                    Transport Layer
//!                               ROSE Layer
//!                          Reject
//!                     Error

//! Errors for Transport Layer (message cannot be sent)
//! TE Errors are reserved in bits 1 to 3 (Values 1 to
#define ISROSE_TE(num) (num & 0x0000000F)
const long ROSE_TE_TRANSPORTFAILED = 0x00000001;
const long ROSE_TE_SHUTDOWN = 0x00000002;
const long ROSE_TE_TIMEOUT = 0x00000003;
// Encode has failed (e.g. JSON encoding and the message was large than possible in the J0000123 header (in case the stub uses a length prefix))
const long ROSE_TE_ENCODE_FAILED = 0x00000004;

//! Errors for ROSE (invalid answer or decoding failed)
//! Check for ROSE Error:
#define ISROSE_RE(num) (num & 0x000000F0)
const long ROSE_RE_INVALID_ANSWER = 0x00000010;
const long ROSE_RE_DECODE_FAILED = 0x00000020;

//! ROSE Server side ROSEReject answers:
//! Function not implemented
#define ISROSE_REJECT(num) (num & 0x00000F00)
const long ROSE_REJECT_UNKNOWNOPERATION = 0x00000100;
//! Function Argument invalid (decode failed)
const long ROSE_REJECT_MISTYPEDARGUMENT = 0x00000200;
//! Function is not implemented (virtual override missing)
const long ROSE_REJECT_FUNCTIONMISSING = 0x00000300;
//! Unknown Reject
const long ROSE_REJECT_UNKNOWN = 0x00000400;
//! Invalid SessionID
const long ROSE_REJECT_INVALIDSESSIONID = 0x00000500;
//! StartSSL required. Function called on a connection that requires StartSSL
const long ROSE_REJECT_STARTSSLREQUIRED = 0x00000600;
//! Authentication Required - Incomplete Authentication - second Step required
const long ROSE_REJECT_AUTHENTICATIONINCOMPLETE = 0x00000700;
//! Authentication Failed
const long ROSE_REJECT_AUTHENTICATIONFAILED = 0x00000800;

//! Async Operation - This is not really a reject, the result will be sent async by the implentor
//! NOT used therefore commented out
// const long ROSE_REJECT_ASYNCOPERATION = 0x00000900;

//! Server busy authentication rejected
const long ROSE_REJECT_AUTHENTICATION_SERVER_BUSY = 0x00000A00;
//! User temporary locked out due to to many failed authentication attempts
const long ROSE_REJECT_AUTHENTICATION_USER_TEMPORARY_LOCKED_OUT = 0x00000B00;
//! User locked out due to to many failed authentication attempts
const long ROSE_REJECT_AUTHENTICATION_USER_LOCKED_OUT = 0x00000C00;
//! The invoke requires an argument but it was not specified
const long ROSE_REJECT_ARGUMENT_MISSING = 0x00000D00;

//! ROSE Server side ROSEError answers:
//! ROSEError Message received
#define ISROSE_ERROR(num) (num & 0x0000F000)
const long ROSE_ERROR_VALUE = 0x00001000;

class SnaccROSEBase;

enum class SnaccInvokeDirection
{
	INBOUND = 0,
	OUTBOUND = 1
};

class SnaccInvokeContextInit
{
public:
	SnaccInvokeContextInit(SnaccInvokeDirection direction, SNACC::ROSEInvoke* pInvoke, const char* szOperationName = nullptr);

	const SnaccInvokeDirection m_direction{};
	const SNACC::ROSEInvoke* m_pInvoke{};
	const std::string m_strOperationName;
};

class SnaccInvokeContext
{
public:
	static std::shared_ptr<SnaccInvokeContext> Create(const SnaccInvokeContextInit& init);
	virtual std::shared_ptr<SnaccInvokeContext> Clone() const;

	SnaccInvokeContext& operator=(const SnaccInvokeContext&) = delete;
	SnaccInvokeContext(SnaccInvokeContext&&) = delete;
	SnaccInvokeContext& operator=(SnaccInvokeContext&&) = delete;
	virtual ~SnaccInvokeContext();

	// Called immediately before the context is transferred into telemetry retention.
	// Override if derived types keep borrowed data that must be detached or copied.
	virtual void PrepareForTelemetry();

	// Meaning in OnInvoke_: Authentication Header from the ROSE Invoke (Pointer to the object in the invoke)
	// Meaning in Invoke_: Authentication Header that is dispatched along with the invoke (create it with new, cleanup is done inside)
	// SNACC::ROSEAuthRequest* pInvokeAuth{};

	// Meaning in OnInvoke_: Authentication Header, which is returned in the reject if the method returnReject returns must be created with new, is cleaned up itself
	// Meaning in Invoke_: If the method returnsReject, this member contains the optional reject authentication (only if SNACC_REJECT_AUTH_INCOMPLETE)
	SNACC::ROSEAuthResult* m_pRejectAuth{};

	// Reject Result Code: 0 or ROSE_REJECT_AUTHENTICATIONFAILED or ROSE_REJECT_AUTHENTICATIONINCOMPLETE
	// Meaning in Invoke_: If the method returns returnReject, this member contains ROSE_REJECT_AUTHENTICATIONINCOMPLETE or ROSE_REJECT_AUTHENTICATIONFAILED if the authentication was not successful
	long m_lRejectResult{};

	// This flag is set to true in case the invoked method replied with an error instead of a result
	bool m_bResponseIsError{};

	// Meaning in OnInvoke_: Original invoke (borrowed, cleared before telemetry retention)
	// SNACC::ROSEInvoke* pInvoke{};

protected:
	explicit SnaccInvokeContext(const SnaccInvokeContextInit& init);
	// Allows derived context types to copy the base invoke-context state into a richer concrete type.
	SnaccInvokeContext(const SnaccInvokeContext& other);
};

/*! SnaccROSESender is the interface that is used to dispatch inbound invokes and events
 */
class SnaccROSESender
{
public:
	virtual std::shared_ptr<SnaccInvokeContext> CreateInvokeContext(const SnaccInvokeContextInit& init) = 0;

	/*! Increment invoke counter and return InvokeID */
	virtual long GetNextInvokeID() = 0;

	/* Retrieve the log level - do we need to log something */
	virtual SNACC::EAsnLogLevel GetLogLevel(const bool bOutbound) = 0;

	/* Print the object pType to the log output
	 *
	 * bOutbound = true if the data is sent, or false if it was received
	 * encoding = the encoding that will be used for sending or was detected while receiving
	 * szData = the plain transport data, pretty much uninterpreted (only the length heading is removed)
	 * szOperationName = name of the operation beeing called (if available)
	 * size = the size of the plain transport data
	 * pMSg = the asn1 message that was encoded or decoded from the transport payload
	 * pParsedValue = only for inboud data, the parsed json object (required if the logging shall get pretty printed, saves another parsing of the payload), only available inbound and if the transport is using json
	 * returns true if data was logged
	 */
	virtual bool LogTransportData(const bool bOutbound, const SNACC::TransportEncoding encoding, const char* szOperationName, const char* szData, const size_t size, const SNACC::ROSEMessage* pMSg, const SJson::Value* pParsedValue = nullptr) = 0;

	/** An invoke that is send to the other side. Should only be called by the ROSE stub itself generated files
	 *
	 * pInvoke - the invoke payload (it is put into a ROSEMessage in the function)
	 * result - decoded result payload in case a result response is received
	 * error - decoded error payload in case an error response is received
	 * szOperationName - the operationName (for logging purposes)
	 * iTimeout - the timeout (-1 is default m_lMaxInvokeWait, 0 return immediately (don't care about the result))
	 * pCtx - contextual data for the invoke. The caller may keep another shared reference
	 *        to inspect changes after the call.
	 */
	virtual long SendInvoke(SNACC::ROSEInvoke* pInvoke, SNACC::AsnType* result, SNACC::AsnType* error, const char* szOperationName, int iTimeout = -1, std::shared_ptr<SnaccInvokeContext> pCtx = {}) = 0;

	/**
	 * Handles the response payload of the SendInvoke method. Retrieves the result or error from the response
	 *
	 * lRoseResult - the result of the SendInvoke method
	 * pResponseMsg - the response message as provided by the SendInvoke method
	 * result - the result object (Base type pointer, the caller of the invoke provides the proper type)
	 * error - the error object (Base type pointer, the caller of the invoke provides the proper type)
	 * pCtx - contextual data for the invoke
	 */
	virtual long HandleInvokeResult(long lRoseResult, const SNACC::ROSEMessage* pResponseMsg, SNACC::AsnType* result, SNACC::AsnType* error, SnaccInvokeContext& ctx) = 0;

	/**
	 * Encodes the result or error from an OnInvoke request. Retrieves the result or error from the response
	 *
	 * invokeResult - the result of the OnInvoke method
	 * pInvoke - the original invoke
	 * ctx - contextual data for the invoke
	 * strResponse - the encoded result to put it on the transport
	 * result - the result object (Base type pointer, the caller of the invoke provides the proper type)
	 * error - the error object (Base type pointer, the caller of the invoke provides the proper type)
	 */
	virtual long HandleOnInvokeResult(SNACC::InvokeResult invokeResult, const SNACC::ROSEInvoke* pInvoke, SnaccInvokeContext& ctx, std::string& strResponse, SNACC::AsnType* pResult, SNACC::AsnType* pError) = 0;

	/**
	 * Decodes an invoke and properly handles logging for it
	 *
	 * pInvokeMessage - the invoke message as provided from the other side
	 * argument - the argument object (Base type pointer, the caller of the provides the proper type)
	 */
	virtual long DecodeInvoke(const SNACC::ROSEMessage* pInvokeMessage, SNACC::AsnType* argument) = 0;

	/** An event (invoke without result) that is send to the other side. Should only be called by the ROSE stub itself generated files
	 *
	 * pInvoke - the invoke payload (it is put into a ROSEMessage in the function)
	 * szOperationName - the operationName (for logging purposes)
	 * pCtx - contextual data for the invoke. The caller may keep another shared reference
	 *        to inspect changes after the call.
	 */
	virtual long SendEvent(SNACC::ROSEInvoke* pInvoke, const char* szOperationName, std::shared_ptr<SnaccInvokeContext> pCtx = {}) = 0;

	/*
	 * Encodes a result as repsonse to an invoke
	 *
	 * uiInvokeID - the inbound invoke that we send the error for
	 * pResult - the actual result object
	 * strResponse - the encoded response data to send via the transport layer
	 * szSessionID - the SessionID (this propery is filled by subclassing from the concrete class in case we are handling multiple clients via one connection)
	 */
	virtual long EncodeResult(unsigned int uiInvokeID, SNACC::AsnType* pResult, std::string& strResponse, const wchar_t* szSessionID = nullptr) = 0;

	/*
	 * Encodes an error as repsonse to an invoke
	 *
	 * uiInvokeID - the inbound invoke that we send the error for
	 * pError - the actual error object
	 * strResponse - the encoded response data to send via the transport layer
	 * szSessionID - the SessionID (this propery is filled by subclassing from the concrete class in case we are handling multiple clients via one connection)
	 */
	virtual long EncodeError(unsigned int uiInvokeID, SNACC::AsnType* pError, std::string& strResponse, const wchar_t* szSessionID = nullptr) = 0;
};

class SnaccScopedInvokeMessage
{
public:
	explicit SnaccScopedInvokeMessage(long invokeID, unsigned int uiOperationID, SNACC::AsnType* argument);
	SnaccScopedInvokeMessage(const SnaccScopedInvokeMessage&) = delete;
	SnaccScopedInvokeMessage& operator=(const SnaccScopedInvokeMessage&) = delete;
	SnaccScopedInvokeMessage(SnaccScopedInvokeMessage&&) = delete;
	SnaccScopedInvokeMessage& operator=(SnaccScopedInvokeMessage&&) = delete;
	~SnaccScopedInvokeMessage();

	SNACC::ROSEInvoke* GetPtr();
	const SNACC::ROSEInvoke* GetPtr() const;

private:
	std::unique_ptr<SNACC::ROSEInvoke> m_pInvoke;
};

/*! ISnaccROSEInvoke is the base class
	for all generated ROSE Protocol Handlers (V2) that need OnInvoke
	*/

/*! SnaccROSEComponent is the base class for all generated ROSE Protocol Handlers (V2)
 */
class SnaccROSEComponent
{
public:
	SnaccROSEComponent(SnaccROSESender* pSB)
	{
		m_pSB = pSB;
	}

protected:
	SnaccROSESender* m_pSB;
};

/*! SnaccROSETransport is Interface class for sending data to the TCP transport layer
 */
class ISnaccROSETransport
{
public:
	/* Output of the binary data.
		Override this function to send the data to the transport layer. */
	virtual long SendBinaryDataBlockEx(const char* lpBytes, size_t size, SnaccInvokeContext& ctx) = 0;
};

#endif //_SnaccROSEInterfaces_h_