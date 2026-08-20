#pragma once

#include <stdio.h>

#if defined(NDEBUG)

#define ASSERT(...) ((void)0)
#define ASSERT_FAILED(...) ((void)0)

#else

#include <stdarg.h>

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

SNACC_ASSERT_INLINE void SnaccAssertImpl(int bCondition, const char* szMessage, const char* szExpr, const char* szFile, int iLine)
{
	if (bCondition)
		return;
	if (szExpr && szExpr[0] != '\0')
		fprintf(stderr, "ASSERT failed: %s\n  condition: %s\n  at %s:%d\n", szMessage, szExpr, szFile, iLine);
	else
		fprintf(stderr, "ASSERT failed: %s\n  at %s:%d\n", szMessage, szFile, iLine);
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
