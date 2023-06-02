#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

	char* mytolower(char* s);
	int myfopen(FILE** pFile, const char* filename, const char* mode);

#ifdef __cplusplus
}
#endif

