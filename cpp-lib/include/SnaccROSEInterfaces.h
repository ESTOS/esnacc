#ifndef _SnaccROSEInterfaces_h_
#define _SnaccROSEInterfaces_h_

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

	enum class InvokeResult
	{
		returnResult = 0,
		returnError = 1,
		returnReject = 2
	};

	enum class TransportEncoding
	{
		// Transport Encoding Basic Encoding Rules (Binary)
		BER = 0,
		// Transport Encoding JSON (Text)
		JSON = 1,
		// Transport Encoding JSON (Text), but without a framing header (J0000XXX{...})
		JSON_NO_HEADING = 2
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
const long ROSE_REJECT_ASYNCOPERATION = 0x00000900;
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
class SnaccInvokeContext;

/*! SnaccROSESender is the interface that is used to dispatch inbound invokes and events
 */
class SnaccROSESender
{
public:
	/*! Increment invoke counter and return InvokeID */
	virtual long GetNextInvokeID() = 0;

	/* Retrieve the log level - do we need to log something */
	virtual SNACC::EAsnLogLevel GetLogLevel(const bool bOutbound) = 0;

	/* Print the object pType to the log output
	 *
	 * bOutbound = true if the data is sent, or false if it was received
	 * encoding = the encoding that will be used for sending or was detected while receiving
	 * szData = the plain transport data, pretty much uninterpreted (only the length heading is removed)
	 * size = the size of the plain transport data
	 * pMSg = the asn1 message that was encoded or decoded from the transport payload
	 * pParsedValue = only for inboud data, the parsed json object (required if the logging shall get pretty printed, saves another parsing of the payload), only available inbound and if the transport is using json
	 */
	virtual void LogTransportData(const bool bOutbound, const SNACC::TransportEncoding encoding, const char* szData, const size_t size, const SNACC::ROSEMessage* pMSg, const SJson::Value* pParsedValue = nullptr) = 0;

	/** An invoke that is send to the other side. Should only be called by the ROSE stub itself generated files
	 *
	 * pInvoke - the invoke payload (it is put into a ROSEMessage in the function)
	 * pResponse - the response payload (is handled afterwards in the HandleInvokeResult method
	 * iTimeout - the timeout (-1 is default m_lMaxInvokeWait, 0 return immediately (don´t care about the result))
	 * iTimeout - the timeout (-1 is default m_lMaxInvokeWait, 0 return immediately (don´t care about the result))
	 * pCtx - contextual data for the invoke
	 */
	virtual long SendInvoke(SNACC::ROSEInvoke* pInvoke, SNACC::ROSEMessage** pResponse, int iTimeout = -1, SnaccInvokeContext* pCtx = nullptr) = 0;

	/** Handles the response payload of the SendInvoke method. Retrieves the result or error from the response
	 *
	 * lRoseResult - the result of the SendInvoke method
	 * pResponseMsg - the response message as provided by the SendInvoke method
	 * result - the result object (Base type pointer, the caller of the invoke provides the proper type)
	 * error - the error object (Base type pointer, the caller of the invoke provides the proper type)
	 * pCtx - contextual data for the invoke
	 */
	virtual long HandleInvokeResult(long lRoseResult, SNACC::ROSEMessage* pResponseMsg, SNACC::AsnType* result, SNACC::AsnType* error, SnaccInvokeContext* cxt) = 0;

	/**
	 * Decodes an invoke and properly handles logging for it
	 *
	 * pInvokeMessage - the invoke message as provided from the other side
	 * argument - the argument object (Base type pointer, the caller of the provides the proper type)
	 */
	virtual long DecodeInvoke(SNACC::ROSEMessage* pInvokeMessage, SNACC::AsnType* argument) = 0;

	/** An event (invoke without result) that is send to the other side. Should only be called by the ROSE stub itself generated files
	 *
	 * pInvoke - the invoke payload (it is put into a ROSEMessage in the function)
	 * pCtx - contextual data for the invoke
	 */
	virtual long SendEvent(SNACC::ROSEInvoke* pInvoke, SnaccInvokeContext* pCtx = nullptr) = 0;

	/* Send a Result Message. */
	virtual long SendResult(const SNACC::ROSEInvoke* pInvoke, SNACC::AsnType* pResult, const wchar_t* szSessionID = 0) = 0;

	/* Send a Error Message. */
	virtual long SendError(const SNACC::ROSEInvoke* pInvoke, SNACC::AsnType* pError, const wchar_t* szSessionID = 0) = 0;
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
	virtual long SendBinaryDataBlockEx(const char* lpBytes, unsigned long lSize, SnaccInvokeContext* pCtx) = 0;
};

#endif //_SnaccROSEInterfaces_h_