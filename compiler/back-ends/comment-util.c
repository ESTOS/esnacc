#include <string.h>
#include "../../c-lib/include/asn-incl.h"
#include "../core/asn1module.h"
#include "snacc.h"
#include "../core/asn_comments.h"
#include "str-util.h"
#include "comment-util.h"

const char* getDeprecated(const char* szDeprecated, enum COMMENTSTYLE style)
{
	if (szDeprecated && strlen(szDeprecated))
		return szDeprecated;
	else if (style == COMMENTSTYLE_TYPESCRIPT)
		return "*";
	else
		return (const char*)NULL;
}

void printEscaped(FILE* src, const char* szData)
{
	const char* szPos = szData;
	const char* szLast = NULL;
	int iStars = 0;
	char szWhiteSpaceBuffer[512] = {0};
	while (*szPos != 0)
	{
		if (*szPos == '@')
			fprintf(src, "\\");
		if (szLast)
		{
			if (*szLast == '*' && *szPos != '*' && iStars == 1)
				fprintf(src, "*");
		}
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
				fprintf(src, "%s", szWhiteSpaceBuffer);
				szWhiteSpaceBuffer[0] = 0;
			}
			fprintf(src, "%c", *szPos);
		}
		if (*szPos == '*')
			iStars++;
		else
			iStars = 0;
		szLast = szPos;
		szPos++;
	}
}

bool printComment(FILE* src, const char* szPrefix, const char* szString, const char* szSuffix)
{
	if (!strlen(szString))
		return false;

	fprintf(src, "%s", szPrefix);

	const char* posBegin = szString;
	const char* posEnd = strstr(szString, "\\n");
	if (posEnd)
	{
		size_t stSize = strlen(szString) + 10;
		char* szBuffer = malloc(stSize);
		if (szBuffer)
		{
			int iFirst = 1;
			bool nInsideUMLBlock = false;
			while (posEnd != NULL)
			{
				memset(szBuffer, 0x00, stSize);
				memcpy(szBuffer, posBegin, posEnd - posBegin);
				char* szBuffer2 = str_replace(szBuffer, "\\t", "\t");

				if (strstr(szBuffer, "@startuml") > 0)
				{
					nInsideUMLBlock = true;
					if (!iFirst)
					{
						fprintf(src, "\n");
						fprintf(src, "%s", szPrefix);
					}
					fprintf(src, " UML section is not exported to this file!");
				}

				if (!nInsideUMLBlock)
				{
					if (!iFirst)
					{
						fprintf(src, "\n");
						fprintf(src, "%s", szPrefix);
					}
					if (strlen(szBuffer2))
						fprintf(src, " ");
					printEscaped(src, szBuffer2);
				}

				if (strstr(szBuffer, "@enduml") > 0)
					nInsideUMLBlock = false;

				free(szBuffer2);

				iFirst = 0;
				// Is there any text left?
				if (strlen(posEnd))
				{
					// Skip the \n
					posBegin = posEnd + 2;
					// Is there another \n ?
					posEnd = strstr(posBegin, "\\n");
					// If not, there's still text, then we'll take that
					if (posEnd == NULL && strlen(posBegin))
						posEnd = posBegin + strlen(posBegin);
				}
				else
				{
					posEnd = NULL;
				}
			}
			free(szBuffer);
		}
	}
	else
	{
		fprintf(src, " ");
		char* szBuffer = str_replace(szString, "\\t", "\t");
		printEscaped(src, szBuffer);
		free(szBuffer);
	}

	fprintf(src, "%s", szSuffix);

	return true;
}

void printMemberComment(FILE* src, const Module* m, const TypeDef* td, const char* szElement, const char* szIndent, enum COMMENTSTYLE style)
{
	if (!giWriteComments)
		return;

	asnmembercomment comment;
	bool bSucceeded = false;
	if (style == COMMENTSTYLE_CPP)
		bSucceeded = GetMemberComment_ASCII(m->moduleName, td->definedName, szElement, &comment) ? true : false;
	else
		bSucceeded = GetMemberComment_UTF8(m->moduleName, td->definedName, szElement, &comment) ? true : false;
	if (bSucceeded)
	{
		if (strlen(comment.szShort) || comment.i64Deprecated || comment.iPrivate)
		{
			int iMultiline = 0;
			if (comment.i64Deprecated)
				iMultiline++;
			if (strstr(comment.szDeprecated, "\\n"))
				iMultiline += 1;
			if (comment.iPrivate)
				iMultiline++;
			if (strstr(comment.szShort, "\\n"))
				iMultiline += 2;
			else if (strlen(comment.szShort))
				iMultiline += 1;
			char szPrefix[128] = {0};
			char szSuffix[128] = {0};
			strcat_s(szPrefix, 128, szIndent);
			if (style == COMMENTSTYLE_CPP || style == COMMENTSTYLE_JAVA || style == COMMENTSTYLE_SWIFT)
			{
				strcat_s(szPrefix, 128, "//");
			}
			else
			{
				strcat_s(szPrefix, 128, iMultiline > 1 ? " *" : "/**");
				strcat_s(szSuffix, 128, iMultiline > 1 ? "" : " */");
				if (iMultiline > 1)
					fprintf(src, "%s/**\n", szIndent);
			}

			bool bAdded = printComment(src, szPrefix, comment.szShort, szSuffix);

			if (comment.i64Deprecated || comment.iPrivate)
			{
				if (bAdded)
				{
					bAdded = false;
					fprintf(src, "\n");
				}
				if (comment.i64Deprecated)
				{
					const char* szComment = getDeprecated(comment.szDeprecated, style);
					char* szTime = ConvertUnixTimeToReadable(comment.i64Deprecated);
					fprintf(src, "%s @deprecated", szPrefix);
					if (szTime)
						fprintf(src, " %s", szTime);
					if (szComment)
						fprintf(src, " %s", szComment);
					fprintf(src, "%s\n", szSuffix);
					free(szTime);
				}
				if (comment.iPrivate)
					fprintf(src, "%s @private%s\n", szPrefix, szSuffix);
			}
			else if (bAdded)
				fprintf(src, "\n");

			if (iMultiline > 1)
			{
				if (style == COMMENTSTYLE_TYPESCRIPT)
					fprintf(src, "%s */\n", szIndent);
			}
		}
	}
}

void printModuleComment(FILE* src, const char* szModuleName, enum COMMENTSTYLE style)
{
	if (!giWriteComments)
		return;

	asnmodulecomment comment;
	bool bSucceeded = false;
	if (style == COMMENTSTYLE_CPP)
		bSucceeded = GetModuleComment_ASCII(szModuleName, &comment) ? true : false;
	else
		bSucceeded = GetModuleComment_UTF8(szModuleName, &comment) ? true : false;
	if (bSucceeded)
	{
		bool bHasShort = strlen(comment.szShort) ? true : false;
		bool bHasLong = strlen(comment.szLong) ? true : false;
		if (bHasShort || bHasLong || comment.i64Deprecated || comment.iPrivate)
		{
			fprintf(src, "/**\n");
			if (bHasShort)
				printComment(src, " *", comment.szShort, "\n");
			if (bHasLong)
				printComment(src, " *", comment.szLong, "\n");
			if (comment.i64Deprecated || comment.iPrivate)
			{
				if (bHasShort || bHasLong)
					fprintf(src, " *\n");
				if (comment.i64Deprecated)
				{
					const char* szComment = getDeprecated(comment.szDeprecated, style);
					char* szTime = ConvertUnixTimeToReadable(comment.i64Deprecated);
					fprintf(src, " * @deprecated");
					if (szTime)
						fprintf(src, " %s", szTime);
					if (szComment)
						fprintf(src, " %s", szComment);
					fprintf(src, "\n");
					free(szTime);
				}
				if (comment.iPrivate)
					fprintf(src, " * @private\n");
			}
			fprintf(src, " */\n");
		}
	}
}

bool printOperationComment(FILE* src, const Module* m, const char* szOperationName, enum COMMENTSTYLE style)
{
	if (!giWriteComments)
		return false;

	asnoperationcomment comment;
	bool bSucceeded = false;
	if (style == COMMENTSTYLE_CPP)
		bSucceeded = GetOperationComment_ASCII(m->moduleName, szOperationName, &comment) ? true : false;
	else
		bSucceeded = GetOperationComment_UTF8(m->moduleName, szOperationName, &comment) ? true : false;
	if (bSucceeded)
	{
		bool bHasShort = strlen(comment.szShort) ? true : false;
		bool bHasLong = strlen(comment.szLong) ? true : false;
		if (bHasShort || bHasLong || comment.i64Deprecated || comment.iPrivate)
		{
			const char* szPrefix = "";
			if (style == COMMENTSTYLE_CPP)
				szPrefix = "\t//";
			else if (style == COMMENTSTYLE_SWIFT || style == COMMENTSTYLE_JAVA)
			{
				fprintf(src, "/**\n");
				szPrefix = " *";
			}
			else
			{
				fprintf(src, "\t/**\n");
				szPrefix = "\t *";
			}
			if (bHasShort)
				printComment(src, szPrefix, comment.szShort, "\n");
			if (bHasLong)
				printComment(src, szPrefix, comment.szLong, "\n");
			if (comment.i64Deprecated || comment.iPrivate)
			{
				if ((bHasShort || bHasLong) && style != COMMENTSTYLE_CPP)
					fprintf(src, "%s\n", szPrefix);
				if (comment.i64Deprecated)
				{
					const char* szComment = getDeprecated(comment.szDeprecated, style);
					char* szTime = ConvertUnixTimeToReadable(comment.i64Deprecated);
					fprintf(src, "%s @deprecated", szPrefix);
					if (szTime)
						fprintf(src, " %s", szTime);
					if (szComment)
						fprintf(src, " %s", szComment);
					fprintf(src, "\n");
					free(szTime);
				}
				if (comment.iPrivate)
					fprintf(src, "%s @private\n", szPrefix);
			}
			if (style == COMMENTSTYLE_TYPESCRIPT)
				fprintf(src, "\t */\n");
			else if (style == COMMENTSTYLE_SWIFT || style == COMMENTSTYLE_JAVA)
				fprintf(src, " */\n");

			return true;
		}
	}

	return false;
}

void printSequenceComment(FILE* src, const Module* m, const TypeDef* td, enum COMMENTSTYLE style)
{
	if (style == COMMENTSTYLE_JAVA)
		fprintf(src, "@Generated(\"estos esnacc\")\n");

	if (!giWriteComments)
		return;

	asnsequencecomment comment;
	bool bSucceeded = false;
	if (style == COMMENTSTYLE_CPP)
		bSucceeded = GetSequenceComment_ASCII(m->moduleName, td->definedName, &comment) ? true : false;
	else
		bSucceeded = GetSequenceComment_UTF8(m->moduleName, td->definedName, &comment) ? true : false;
	if (bSucceeded)
	{
		bool bHasShort = strlen(comment.szShort) ? true : false;
		bool bHasLong = strlen(comment.szLong) ? true : false;
		if (bHasShort || bHasLong || comment.i64Deprecated || comment.iPrivate)
		{
			const char* szPrefix = "";
			if (style == COMMENTSTYLE_CPP)
			{
				szPrefix = "//";
			}
			else
			{
				fprintf(src, "/**\n");
				szPrefix = " *";
			}
			if (bHasShort)
				printComment(src, szPrefix, comment.szShort, "\n");
			if (bHasLong)
				printComment(src, szPrefix, comment.szLong, "\n");
			if (comment.i64Deprecated || comment.iPrivate)
			{
				if (bHasShort || bHasLong)
					fprintf(src, "%s\n", szPrefix);
				if (comment.i64Deprecated)
				{
					const char* szComment = getDeprecated(comment.szDeprecated, style);
					char* szTime = ConvertUnixTimeToReadable(comment.i64Deprecated);
					fprintf(src, "%s @deprecated", szPrefix);
					if (szTime)
						fprintf(src, " %s", szTime);
					if (szComment)
						fprintf(src, " %s", szComment);
					fprintf(src, "\n");
					free(szTime);
				}
				if (comment.iPrivate)
					fprintf(src, "%s @private\n", szPrefix);
			}
			if (style != COMMENTSTYLE_CPP)
				fprintf(src, " */\n");
		}
	}
}
