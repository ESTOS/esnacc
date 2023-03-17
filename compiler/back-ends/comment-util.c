#include <string.h>
#include "../../c-lib/include/asn-incl.h"
#include "../core/asn1module.h"
#include "snacc.h"
#include "../core/asn_comments.h"
#include "str-util.h"
#include "comment-util.h"

void printEscaped(FILE* src, const char* szData) {
	const char* szPos = szData;
	const char* szLast = NULL;
	int iStars = 0;
	char szWhiteSpaceBuffer[512] = { 0 };
	while (*szPos != 0) {
		if (*szPos == '@')
			fprintf(src, "\\");
		if (szLast) {
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
		else {
			if (strlen(szWhiteSpaceBuffer)) {
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

void printComment(FILE* src, const char* szPrefix, const char* szString, const char* szSuffix)
{
	if (!strlen(szString))
		return;

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

				if (strstr(szBuffer, "@startuml") > 0) {
					nInsideUMLBlock = true;
					if (!iFirst) {
						fprintf(src, "\n");
						fprintf(src, "%s", szPrefix);
					}
					fprintf(src, " UML section is not exported to this file!");
				}

				if (!nInsideUMLBlock) {
					if (!iFirst) {
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
				// Ist noch Text vorhanden?
				if (strlen(posEnd)) {
					// Das \n überspringen
					posBegin = posEnd + 2;
					// Gibt es noch ein \n ?
					posEnd = strstr(posBegin, "\\n");
					// Wenn nicht, gibt es noch Text, dann nehmen wir den
					if (posEnd == NULL && strlen(posBegin))
						posEnd = posBegin + strlen(posBegin);
				} else {
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
}

void printMemberComment(FILE* src, const Module* m, const TypeDef* td, const char* szElement) {
	if (!gWriteComments)
		return;

	asnmembercomment comment;
	if (GetMemberComment_UTF8(m->moduleName, td->definedName, szElement, &comment)) {
		if (strlen(comment.szShort) || comment.iDeprecated || comment.iPrivate) {
			int iMultiline = 0;
			if (comment.iDeprecated)
				iMultiline++;
			if (comment.iPrivate)
				iMultiline++;
			if (strstr(comment.szShort, "\\n"))
				iMultiline += 2;
			else if(strlen(comment.szShort))
				iMultiline += 1;
			const char* szRemarksPrefix = iMultiline > 1 ? "\t *" : "\t/**";
			const char* prefix = iMultiline > 1 ? "\t *" : "\t/**";
			const char* suffix = iMultiline > 1 ? "" : " */";
			if (iMultiline > 1)
				fprintf(src, "\t/**\n");

			printComment(src, szRemarksPrefix, comment.szShort, suffix);

			if (comment.iDeprecated || comment.iPrivate)
			{
				if (comment.iDeprecated)
					fprintf(src, "\n%s @deprecated - this property is deprecated%s", prefix, suffix);
				if (comment.iPrivate)
					fprintf(src, "\n%s @private%s", prefix, suffix);
			}

			if (iMultiline > 1)
				fprintf(src, "\n\t */");
			fprintf(src, "\n");
		}
	}
}

void printModuleComment(FILE* src, const char* szModuleName) {
	if (!gWriteComments)
		return;

	asnmodulecomment moduleComment;
	if (GetModuleComment_UTF8(szModuleName, &moduleComment))
	{
		bool bHasShort = strlen(moduleComment.szShort) ? true : false;
		bool bHasLong = strlen(moduleComment.szLong) ? true : false;
		if (bHasShort || bHasLong || moduleComment.iDeprecated || moduleComment.iPrivate) {
			fprintf(src, "/**\n");
			if (bHasShort)
				printComment(src, " *", moduleComment.szShort, "\n");
			if (bHasLong)
				printComment(src, " *", moduleComment.szLong, "\n");
			if (moduleComment.iDeprecated || moduleComment.iPrivate) {
				if (bHasShort || bHasLong)
					fprintf(src, " *\n");
				if (moduleComment.iDeprecated) {
					if (strlen(moduleComment.szDeprecated))
						fprintf(src, " * @deprecated - %s\n", moduleComment.szDeprecated);
					else
						fprintf(src, " * @deprecated\n");
				}
				if (moduleComment.iPrivate)
					fprintf(src, "* @private\n");
			}
			fprintf(src, " */\n");
		}
	}
}

void printSequenceComment(FILE* src, const Module* m, const TypeDef* td) {
	if (!gWriteComments)
		return;

	asnsequencecomment sequenceComment;
	if (GetSequenceComment_UTF8(m->moduleName, td->definedName, &sequenceComment))
	{
		bool bHasShort = strlen(sequenceComment.szShort) ? true : false;
		bool bHasLong = strlen(sequenceComment.szLong) ? true : false;
		if (bHasShort || bHasLong || sequenceComment.iDeprecated || sequenceComment.iPrivate) {
			fprintf(src, "/**\n");
			if (bHasShort)
				printComment(src, " *", sequenceComment.szShort, "\n");
			if (bHasLong)
				printComment(src, " *", sequenceComment.szLong, "\n");
			if (sequenceComment.iDeprecated || sequenceComment.iPrivate) {
				if (bHasShort || bHasLong)
					fprintf(src, " *\n");
				if (sequenceComment.iDeprecated) {
					if (strlen(sequenceComment.szDeprecated))
						fprintf(src, " * @deprecated - %s\n", sequenceComment.szDeprecated);
					else
						fprintf(src, " * @deprecated\n");
				}
				if (sequenceComment.iPrivate)
					fprintf(src, "* @private\n");
			}
			fprintf(src, " */\n");
		}
	}
}
