#include "interface_baseline.h"
#include "time_helpers.h"
#include "../../snacc.h"
#include "asn_comments.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

extern int copy_file(const char* source, const char* destination);

int gNodeprecatedAutoResolve = 0;
int gCliNodeprecatedExplicit = 0;
int gInterfaceBaselineAutoResolved = 0;
long long gi64FileBaselineUnix = 0;

/**
 * Returns whether a version string uses the same date notations as @deprecated/@added.
 */
static int LineLooksLikeAsn1Date(const char* szValue)
{
	if (!szValue || !szValue[0])
		return 0;
	if (strchr(szValue, '.') || strchr(szValue, '-') || strchr(szValue, '/'))
		return 1;
	return strlen(szValue) == 8;
}

void SetMajorVersionFromUnixBaseline(long long i64UnixTime)
{
	if (i64UnixTime <= 0)
		return;

	char* pszNumeric = ConvertUnixTimeToNumericDate(i64UnixTime);
	if (!pszNumeric)
		return;

	gMajorInterfaceVersion = atoi(pszNumeric);
	free(pszNumeric);
}

void ApplyDeprecatedCutoffUnix(long long i64UnixTime)
{
	if (i64UnixTime <= 0)
		return;

	gi64NoDeprecatedSymbols = i64UnixTime;
	SetMajorVersionFromUnixBaseline(i64UnixTime);
}

static int ParseDeprecatedBaselineLine(const char* szLine)
{
	if (!szLine)
		return 0;

	int start = 0;
	while (szLine[start] && isspace((unsigned char)szLine[start]))
		start++;

	if (!szLine[start] || szLine[start] == '#' || (szLine[start] == '/' && szLine[start + 1] == '/'))
		return 0;

	char szTrimmed[128] = {0};
	strcpy_s(szTrimmed, sizeof(szTrimmed), &szLine[start]);
	size_t len = strlen(szTrimmed);
	while (len > 0 && (szTrimmed[len - 1] == '\r' || szTrimmed[len - 1] == '\n' || isspace((unsigned char)szTrimmed[len - 1])))
		szTrimmed[--len] = '\0';

	return ParseInterfaceVersionValue(szTrimmed);
}

int LoadDeprecatedBaselineFile(const char* szDirectory)
{
	if (!szDirectory)
		return 0;

	char szPath[_MAX_PATH] = {0};
	strcpy_s(szPath, _MAX_PATH - 1, szDirectory);
	strcat_s(szPath, _MAX_PATH - 1, DEPRECATED_BASELINE_FILENAME);

	FILE* pFile = NULL;
	if (fopen_s(&pFile, szPath, "r") != 0 || !pFile)
		return 0;

	int iLoaded = 0;
	while (true)
	{
		char szLine[128] = {0};
		if (!fgets(szLine, sizeof(szLine), pFile))
			break;

		if (ParseDeprecatedBaselineLine(szLine))
		{
			iLoaded = 1;
			break;
		}
	}

	fclose(pFile);
	return iLoaded;
}

int CopyDeprecatedBaselineFile(const char* szSourceDirectory, const char* szTargetDirectory)
{
	if (!szSourceDirectory || !szTargetDirectory)
		return 0;

	char szSource[_MAX_PATH] = {0};
	char szTarget[_MAX_PATH] = {0};
	strcpy_s(szSource, _MAX_PATH - 1, szSourceDirectory);
	strcat_s(szSource, _MAX_PATH - 1, DEPRECATED_BASELINE_FILENAME);
	strcpy_s(szTarget, _MAX_PATH - 1, szTargetDirectory);
	strcat_s(szTarget, _MAX_PATH - 1, DEPRECATED_BASELINE_FILENAME);

	FILE* pFile = NULL;
	if (fopen_s(&pFile, szSource, "r") != 0 || !pFile)
		return 0;
	fclose(pFile);

	extern int copy_file(const char* source, const char* destination);
	return copy_file(szSource, szTarget) == 0;
}

long long GetInterfaceBaselineUnixTimestamp(void)
{
	if (gi64NoDeprecatedSymbols > 1)
		return gi64NoDeprecatedSymbols;
	if (gi64FileBaselineUnix > 0)
		return gi64FileBaselineUnix;
	return 0;
}

int ParseInterfaceVersionValue(const char* szTrimmedLine)
{
	if (!szTrimmedLine || !szTrimmedLine[0])
		return 0;

	if (LineLooksLikeAsn1Date(szTrimmedLine))
	{
		const long long i64Unix = ConvertDateToUnixTime(szTrimmedLine);
		if (i64Unix > 0)
		{
			SetMajorVersionFromUnixBaseline(i64Unix);
			if (gCliNodeprecatedExplicit)
				return 1;

			if (gNodeprecatedAutoResolve)
			{
				gi64FileBaselineUnix = i64Unix;
				return 1;
			}

			gi64NoDeprecatedSymbols = i64Unix;
			return 1;
		}
	}

	const int iLegacyMajor = atoi(szTrimmedLine);
	if (iLegacyMajor > 0)
	{
		gMajorInterfaceVersion = iLegacyMajor;
		return 1;
	}

	return 0;
}

void ResolveInterfaceBaselineAfterParse(void)
{
	if (!gNodeprecatedAutoResolve)
		return;

	gNodeprecatedAutoResolve = 0;
	gInterfaceBaselineAutoResolved = 1;

	const long long i64MaxDeprecated = GetMaxDeprecatedTimestamp();
	if (i64MaxDeprecated > 0)
	{
		ApplyDeprecatedCutoffUnix(i64MaxDeprecated);
		return;
	}

	if (gi64FileBaselineUnix > 0)
		gi64NoDeprecatedSymbols = gi64FileBaselineUnix;
}

void ResetInterfaceBaselineStateForTests(void)
{
	gNodeprecatedAutoResolve = 0;
	gCliNodeprecatedExplicit = 0;
	gInterfaceBaselineAutoResolved = 0;
	gi64FileBaselineUnix = 0;
	gi64NoDeprecatedSymbols = 0;
	gMajorInterfaceVersion = 0;
}
