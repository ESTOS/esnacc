#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../snacc_defines.h"

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

enum EDateFormat
{
	EDateFormat_EUROPEAN, // 31.1.2024
	EDateFormat_US_SLASH, // 1/31/2024
	EDateFormat_US_MINUS, // 1-31-2024
	EDateFormat_LONG	  // 20240131
};

/**
 * Converts a date in different notations into the seconds based on unix time 1.1.1970
 * DD.MM.YYYY
 * MM/DD/YYYY
 * MM-DD-YYYY
 * YYYYMMDD
 *
 * Returns -1 on error
 */
long long ConvertDateToUnixTime(const char* szDate)
{
	if (!szDate)
		return 0;
	if (!strlen(szDate))
		return 0;

	long long tmResult = -1;

	enum EDateFormat format = EDateFormat_EUROPEAN;
	if (strstr(szDate, "."))
		format = EDateFormat_EUROPEAN;
	else if (strstr(szDate, "-"))
		format = EDateFormat_US_MINUS;
	else if (strstr(szDate, "/"))
		format = EDateFormat_US_SLASH;
	else if (strlen(szDate) == 8)
		format = EDateFormat_LONG;
	else
	{
		assert(false);
		fprintf(stderr, "Unknown date format %s", szDate);
	}

#ifdef _WIN32
	SYSTEMTIME st;
	memset(&st, 0x00, sizeof(SYSTEMTIME));

	bool bSucceeded = false;
	switch (format)
	{
		case EDateFormat_EUROPEAN:
			bSucceeded = sscanf_s(szDate, "%hd.%hd.%hd", &st.wDay, &st.wMonth, &st.wYear) == 3;
			break;
		case EDateFormat_US_MINUS:
			bSucceeded = sscanf_s(szDate, "%hd-%hd-%hd", &st.wMonth, &st.wDay, &st.wYear) == 3;
			break;
		case EDateFormat_US_SLASH:
			bSucceeded = sscanf_s(szDate, "%hd/%hd/%hd", &st.wMonth, &st.wDay, &st.wYear) == 3;
			break;
		case EDateFormat_LONG:
			bSucceeded = sscanf_s(szDate, "%4hu%2hu%2hu", &st.wYear, &st.wMonth, &st.wDay) == 3;
			break;
	}

	if (bSucceeded)
	{
		if (st.wDay < 1 || st.wDay > 31)
			return tmResult;
		if (st.wMonth < 1 || st.wMonth > 12)
			return tmResult;
		if (st.wYear < 1970)
			return tmResult;
		FILETIME ft;
		SystemTimeToFileTime(&st, &ft);
		ULARGE_INTEGER uli;
		uli.LowPart = ft.dwLowDateTime;
		uli.HighPart = ft.dwHighDateTime;
		tmResult = (long long)((uli.QuadPart / 10000000ULL) - 11644473600ULL);
	}
#else
	struct tm tm;
	memset(&tm, 0x00, sizeof(tm));

	switch (format)
	{
		case EDateFormat_EUROPEAN:
			if (strptime(szDate, "%d.%m.%Y", &tm))
				tmResult = mktime(&tm);
			break;
		case EDateFormat_US_MINUS:
			if (strptime(szDate, "%m-%d-%Y", &tm))
				tmResult = mktime(&tm);
			break;
		case EDateFormat_US_SLASH:
			if (strptime(szDate, "%m/%d/%Y", &tm))
				tmResult = mktime(&tm);
			break;
		case EDateFormat_LONG:
			if (strptime(szDate, "%Y%m%d", &tm))
				tmResult = mktime(&tm);
			break;
	}

#endif
	return tmResult;
}

/**
 * Converts a unix time into something readable
 *
 * Returns a pointer to a buffer that needs to get released with Free
 */
char* ConvertUnixTimeToReadable(const long long tmUnixTime)
{
	if (tmUnixTime <= 0)
		return NULL;

	char* szBuffer = malloc(128);
	if (!szBuffer)
		return NULL;

#ifdef _WIN32
	struct tm timeinfo;
	localtime_s(&timeinfo, &tmUnixTime);
	strftime(szBuffer, 128, "%d.%m.%Y", &timeinfo);
#else
	struct tm* timeinfo;
	timeinfo = localtime((const time_t*)&tmUnixTime);
	strftime(szBuffer, 128, "%d.%m.%Y", timeinfo);
#endif

	return szBuffer;
}

/**
 * Converts a unix time into an ISO timestamp
 *
 * Returns a pointer to a buffer that needs to get released with Free
 */
char* ConvertUnixTimeToISO(const long long tmUnixTime)
{
	char* szBuffer = malloc(128);
	if (!szBuffer)
		return NULL;

#ifdef _WIN32
	struct tm timeinfo;
	localtime_s(&timeinfo, &tmUnixTime);
	strftime(szBuffer, 128, "%Y-%m-%dT00:00:00Z", &timeinfo);
#else
	struct tm* timeinfo;
	timeinfo = localtime((const time_t*)&tmUnixTime);
	strftime(szBuffer, 128, "%Y-%m-%dT00:00:00Z", timeinfo);
#endif

	return szBuffer;
}
