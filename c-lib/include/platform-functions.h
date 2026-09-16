#pragma once

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

	char* mytolower(char* s);
	int myfopen(FILE** pFile, const char* filename, const char* mode);
	// Portable getenv: CRT getenv_s on Windows, getenv shim on POSIX (asn-config.h macro).
	int mygetenv(size_t* buffer_used, char* buffer, size_t buffer_count, const char* varname);

#ifdef __cplusplus
}
#endif
