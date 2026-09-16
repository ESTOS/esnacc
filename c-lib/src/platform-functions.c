#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* mytolower(char* s)
{
	char* tmp = s;

	for (; *tmp; ++tmp)
		*tmp = (char)tolower((unsigned char)*tmp);

	return s;
}

int myfopen(FILE** pFile, const char* filename, const char* mode)
{
#ifdef _WIN32
	errno_t err = fopen_s(pFile, filename, mode);
	if (err != 0)
		*pFile = NULL;
#else
	*pFile = fopen(filename, mode);
#endif
	return *pFile == NULL;
}

// Portable getenv (CRT getenv_s on Windows, getenv shim on POSIX). See asn-config.h.
int mygetenv(size_t* buffer_used, char* buffer, size_t buffer_count, const char* varname)
{
#ifdef _WIN32
	return (int)getenv_s(buffer_used, buffer, buffer_count, varname);
#else
	const char* pszValue = getenv(varname);
	if (buffer_used != NULL)
		*buffer_used = 0;
	if (buffer != NULL && buffer_count > 0)
		buffer[0] = '\0';
	if (pszValue == NULL)
		return 0;
	if (buffer_used != NULL)
		*buffer_used = strlen(pszValue) + 1;
	if (buffer != NULL && buffer_count > 0)
	{
		strncpy(buffer, pszValue, buffer_count - 1);
		buffer[buffer_count - 1] = '\0';
	}
	return 0;
#endif
}
