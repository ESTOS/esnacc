#include "gen-swift-combined.h"
#include "../../core/snacc-util.h"
#include "../str-util.h"
#include "../structure-util.h"
#include <assert.h>

void printSwiftscaped(FILE* src, const char* szData)
{
	const char* szPos = szData;
	const char* szLast = NULL;
	int iStars = 0;
	// char szWhiteSpaceBuffer[512] = { 0 };
	while (*szPos != 0)
	{
		if (*szPos == '@')
			fprintf(src, "\\");
		if (szLast)
		{
			if (*szLast == '*' && *szPos != '*' && iStars == 1)
				fprintf(src, "*");
		}
#if FALSE
		// Remove trailing whitespaces (print whitespaces late in case we have no whitespaced following)
		if (*szPos == ' ')
			strcat_s(szWhiteSpaceBuffer, 512, " ");
		else if (*szPos == '\t')
			strcat_s(szWhiteSpaceBuffer, 512, "\t");
		else if (*szPos == '\n')
			szWhiteSpaceBuffer[0] = 0;
		else
		{
			if (strlen(szWhiteSpaceBuffer))
			{
				fprintf(src, szWhiteSpaceBuffer);
				szWhiteSpaceBuffer[0] = 0;
			}
			fprintf(src, "%c", *szPos);
		}
#else
		// Test code to find whitespaces in the output
		fprintf(src, "%c", *szPos);
#endif
		if (*szPos == '*')
			iStars++;
		else
			iStars = 0;
		szLast = szPos;
		szPos++;
	}
}

void PrintSwiftImports(FILE* src, ModuleList* mods, Module* mod, bool bIncludeConverters)
{
	PRINTCOMMENT(src, "// [%s]\n", __FUNCTION__);
	fprintf(src, "import Foundation\n");
}

const char* GetSwiftType(const enum BasicTypeChoiceId basicTypeChoiseId)
{
	switch (basicTypeChoiseId)
	{
		case BASICTYPE_BOOLEAN:
			return "Bool";
		case BASICTYPE_INTEGER:
			return "Int";
		case BASICTYPE_ENUMERATED:
			return "Int";
		case BASICTYPE_UTF8_STR:
			return "NSString";
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			return "Data";
		case BASICTYPE_REAL:
			return "Double";
		case BASICTYPE_ASNSYSTEMTIME:
			return "NSDate";
		case BASICTYPE_NULL:
		case BASICTYPE_ANY:
			return "AnyObject";
		case BASICTYPE_UTCTIME:
			return "NSDate";
		default:
			assert(FALSE);
	}

	exit(1);
}
