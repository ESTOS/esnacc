#pragma once

#include <stdio.h>

#if defined(NDEBUG)

#define ASSERT(...) ((void)0)
#define ASSERT_FAILED(...) ((void)0)

#else

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "platform-functions.h"

#if defined(_WIN32)
#ifdef __cplusplus
extern "C"
{
#endif
	__declspec(dllimport) int __stdcall IsDebuggerPresent(void);
#ifdef __cplusplus
}
#endif
#endif

#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#else
#define DEBUG_BREAK() ((void)0)
#endif

#if defined(__cplusplus)
#define SNACC_ASSERT_INLINE inline
#else
#if defined(_MSC_VER)
#define SNACC_ASSERT_INLINE static __inline
#else
#define SNACC_ASSERT_INLINE static inline
#endif
#endif

// True when a debugger is attached to this process (debug run vs loose run; independent of Debug/Release build).
SNACC_ASSERT_INLINE int SnaccDebuggerIsAttached(void)
{
#if defined(_WIN32)
	return IsDebuggerPresent() != 0;
#elif defined(__linux__)
	FILE* pStatus = fopen("/proc/self/status", "r");
	if (pStatus == NULL)
		return 0;
	char szLine[256];
	int iTracerPid = 0;
	while (fgets(szLine, sizeof(szLine), pStatus) != NULL)
	{
		if (strncmp(szLine, "TracerPid:", 10) == 0)
		{
			iTracerPid = atoi(szLine + 10);
			break;
		}
	}
	fclose(pStatus);
	return iTracerPid != 0;
#else
	return 0;
#endif
}

// Set on snacc-cpp-tests only (see cpp-lib/tests/CMakeLists.txt) so loose GTest runs log to stdout.
SNACC_ASSERT_INLINE int SnaccIsGTestBinary(void)
{
#if defined(SNACC_GTEST_BINARY)
	return 1;
#else
	return 0;
#endif
}

// When set, force breakpoints during tests even without a debugger (parity with TS SNACC_ROSE_DEBUG_IN_TESTS).
SNACC_ASSERT_INLINE int SnaccRoseDebugInTestsEnabled(void)
{
	char szValue[64];
	size_t cchValue = 0;
	if (mygetenv(&cchValue, szValue, sizeof(szValue), "SNACC_ROSE_DEBUG_IN_TESTS") != 0 || cchValue == 0)
		return 0;
	return szValue[0] != '\0' && strcmp(szValue, "0") != 0;
}

// Break only on a debug run (debugger attached) or when SNACC_ROSE_DEBUG_IN_TESTS overrides loose GTest runs.
SNACC_ASSERT_INLINE int SnaccAssertShouldBreak(void)
{
	if (SnaccDebuggerIsAttached())
		return 1;
	if (SnaccRoseDebugInTestsEnabled())
		return 1;
	return 0;
}

// Loose GTest runs (no debugger) use stdout so Test Explorer / ctest capture assert diagnostics.
SNACC_ASSERT_INLINE FILE* SnaccAssertOutputStream(void)
{
	if (SnaccIsGTestBinary() && !SnaccDebuggerIsAttached() && !SnaccRoseDebugInTestsEnabled())
		return stdout;
	return stderr;
}

SNACC_ASSERT_INLINE void SnaccAssertImpl(int bCondition, const char* szMessage, const char* szExpr, const char* szFile, int iLine)
{
	if (bCondition)
		return;
	FILE* pStream = SnaccAssertOutputStream();
	if (szExpr && szExpr[0] != '\0')
		fprintf(pStream, "ASSERT failed: %s\n  condition: %s\n  at %s:%d\n", szMessage, szExpr, szFile, iLine);
	else
		fprintf(pStream, "ASSERT failed: %s\n  at %s:%d\n", szMessage, szFile, iLine);
	if (SnaccAssertShouldBreak())
		DEBUG_BREAK();
}

SNACC_ASSERT_INLINE void SnaccAssertFailV(const char* szFormat, ...)
{
	char szMessage[512];
	va_list args;
	va_start(args, szFormat);
	(void)vsnprintf(szMessage, sizeof(szMessage), szFormat, args);
	va_end(args);
	szMessage[sizeof(szMessage) - 1] = '\0';
	SnaccAssertImpl(0, szMessage, "", __FILE__, __LINE__);
}

#define SNACC_ASSERT_GET_MACRO(_1, _2, NAME, ...) NAME
#define SNACC_ASSERT_1(condition) SnaccAssertImpl(!!(condition), #condition, #condition, __FILE__, __LINE__)
#define SNACC_ASSERT_2(condition, message) SnaccAssertImpl(!!(condition), (message), #condition, __FILE__, __LINE__)

/* Debug assert: ASSERT(condition) uses #condition as message; ASSERT(condition, message) prints message. */
#define ASSERT(...) SNACC_ASSERT_GET_MACRO(__VA_ARGS__, SNACC_ASSERT_2, SNACC_ASSERT_1)(__VA_ARGS__)

/* Debug assert for a known error path; supports printf-style formatting. */
#define ASSERT_FAILED(...) SnaccAssertFailV(__VA_ARGS__)

#endif
