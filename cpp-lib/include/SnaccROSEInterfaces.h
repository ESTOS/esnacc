#ifndef _SnaccROSEInterfaces_h_
#define _SnaccROSEInterfaces_h_

namespace SNACC
{
	class AsnType;
	class ROSEMessage;
	class ROSEInvoke;
	class ROSEResult;
	class ROSEError;
	class ROSEReject;
	class InvokeProblem;

	enum SnaccInvokeResult
	{
		returnResult = 0,
		returnError = 1,
		returnReject = 2
	};

} // namespace SNACC

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

//! ROSE Server side ROSEError answers:
//! ROSEError Message received
#define ISROSE_ERROR(num) (num & 0x0000F000)
const long ROSE_ERROR_VALUE = 0x00001000;

class SnaccROSEBase;
class SnaccInvokeContext;

/*! SnaccROSESender ist das Interface, über das abgehende Invokes und Events gesendet werden können
 */
class SnaccROSESender
{
public:
	/*! Increment invoke counter and return InvokeID */
	virtual long GetNextInvokeID() = 0;

	/* Log level 0 oder 1
		bout=true for outgoing messages,
		bout=false for incoming messages,
		Override to set a different log level */
	virtual long GetLogLevel(bool /*bOut*/) = 0;

	/* used for printing alle the messages */
	virtual void PrintAsnType(bool bOutbound, SNACC::AsnType* pType, SNACC::ROSEInvoke* pInvoke) = 0;

	/** The following function should only be called by the generated ROSE stub */
	/* Invoke a function.
		ppresult or pperror will be filled on return
		caller must delete returned result!
		returns NO_ERROR for Result
		iTimeout: Wartezeit für das Result.
		iTimeout : -1 default Wartezeit m_lMaxInvokeWait
		iTimeout : 0 keine Wartezeit
		iTimeout : 1 ... Wartezeit in Millisekunden
		*/
	virtual long SendInvoke(SNACC::ROSEInvoke* pinvoke, SNACC::ROSEResult** ppresult, SNACC::ROSEError** pperror, int iTimeout = -1, SnaccInvokeContext* pCtx = nullptr) = 0;

	/* Send a Event Message. */
	virtual long SendEvent(SNACC::ROSEInvoke* pinvoke, SnaccInvokeContext* pCtx = nullptr) = 0;

	/* Send a Result Message. */
	virtual long SendResult(int invokeID, SNACC::AsnType* value, const wchar_t* szSessionID = 0) = 0;

	/* Send a Error Message. */
	virtual long SendError(int invokeID, SNACC::AsnType* value, const wchar_t* szSessionID = 0) = 0;

	/*! Helper Funktion zur Behandlung des Result nach einem SendInvoke */
	long HandleInvokeResult(long lRoseResult, SNACC::ROSEInvoke* pInvokeMsg, SNACC::ROSEResult* pResultMsg, SNACC::ROSEError* pErrorMsg, SNACC::AsnType* result, SNACC::AsnType* error, SnaccInvokeContext* cxt);
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