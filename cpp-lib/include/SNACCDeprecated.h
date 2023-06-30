#if !defined(SNACC_DEPRECATED_H)
#define SNACC_DEPRECATED_H

#include <list>
#include <string>
#include "SnaccROSEBase.h"

#ifdef _WIN32
#pragma comment(lib, "Dbghelp.lib")
#endif

enum class SNACCDeprecatedNotifyCallDirection
{
	in = 0,
	out = 1
};

/**
 * This is the central deprecated callback
 *
 * The asn1 files allow to flag objects or methods as deprecated
 * If that is the case the compiler adds callbacks in case such an object is created or such a method is called
 */
class SNACCDeprecatedNotify
{
public:
	/**
	 * A deprecated object has been created
	 *
	 * @param i64DeprecatedSince - unix time when the object has been flagged deprecated
	 * @param szModuleName - the module in which the object is located
	 * @param szObjectName - the name of the object that is about to get created
	 * @param callStack - the call stack that shows where the object has been created
	 */
	virtual void DeprecatedASN1Object(const long long i64DeprecatedSince, const char* szModuleName, const char* szObjectName, const std::list<std::string>& callStack) = 0;

	/**
	 * A deprecated method has been called
	 *
	 * @param i64DeprecatedSince - unix time when the method has been flagged deprecated
	 * @param szModuleName - the module in which the object is located
	 * @param szMethodName - the name of the method that has been called
	 * @param direction - whether the call was inbound or outbound
	 * @param callStack - the call stack that shows where the object has been created
	 * @param pContext - The invoke Context that is associated with the invoke
	 */
	virtual void DeprecatedASN1Method(const long long i64DeprecatedSince, const char* szModuleName, const char* szMethodName, const SNACCDeprecatedNotifyCallDirection direction, const std::list<std::string>& callStack, const SnaccInvokeContext* pContext = NULL) = 0;
};

class SNACCDeprecated
{
public:
	/**
	 * Sets the callback
	 *
	 * @param pCallBack - the callback that should get notified
	 * @param bProvideStackTrace - true to get stack traces in DeprecatedASN1Method
	 */
	static void SetDeprecatedCallback(SNACCDeprecatedNotify* pCallBack, bool bProvideStackTrace);

	/**
	 * This method is called in case an object is created which is flagged deprecated
	 *
	 * @param i64DeprecatedSince - unix time when the object has been flagged deprecated
	 * @param szModuleName - the module in which the object is located
	 * @param szObjectName - the name of the object that is about to get created
	 */
	static void DeprecatedASN1Object(const long long i64DeprecatedSince, const char* szModuleName, const char* szObjectName);

	/**
	 * This method is called in case a method is called which is flagged deprecated
	 *
	 * @param i64DeprecatedSince - unix time when the method has been flagged deprecated
	 * @param szModuleName - the module in which the object is located
	 * @param szMethodName - the name of the method that has been called
	 * @param direction - whether the call was inbound or outbound
	 * @param pContext - the invokeContext that shows more details about the invoke
	 */
	static void DeprecatedASN1Method(const long long i64DeprecatedSince, const char* szModuleName, const char* szMethodName, const SNACCDeprecatedNotifyCallDirection direction, const SnaccInvokeContext* pContext = NULL);

private:
	/**
	 * Retrieves the current callstack
	 *
	 * @param remove - the amount of elements from the top to remove (to get rid of calls we do not want to see in the stack trace)
	 */
	static std::list<std::string> GetStackTrace(int remove = 1);

	// The current callback that is notified about deprecated method calls or object creations
	static SNACCDeprecatedNotify* m_pCallback;
	static bool m_bProvideStackTrace;
};

#endif // SNACC_DEPRECATED_H