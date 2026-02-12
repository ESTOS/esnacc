#pragma once

#if defined(NDEBUG)
#define ASSERT(expr) ((void)0)
#else
#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#else
#define DEBUG_BREAK() ((void)0)
#endif

#define ASSERT(expr)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
	do                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
	{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
		if (!(expr))                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
		{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
			fprintf(stderr, "ASSERT failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__);                                                                                                                                                                                                                                                                                                                                                                                                                             \
			DEBUG_BREAK();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
		}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
	} while (0)
#endif
