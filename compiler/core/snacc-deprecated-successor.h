#ifndef SNACC_DEPRECATED_SUCCESSOR_H
#define SNACC_DEPRECATED_SUCCESSOR_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/** Prints canonical @deprecated successor syntax for esnacc7 -h. */
	void PrintDeprecatedSuccessorHelp(FILE* fp);

#ifdef __cplusplus
}

#include <string>

typedef enum EDeprecatedSuccessorScope
{
	EDeprecatedSuccessorUnset = 0,
	EDeprecatedSuccessorNone = 1,
	EDeprecatedSuccessorSameFile = 2,
	EDeprecatedSuccessorQualified = 3
} EDeprecatedSuccessorScope;

struct DeprecatedSuccessorFields
{
	int arrowCount = 0;
	bool parsed = false;
	EDeprecatedSuccessorScope scope = EDeprecatedSuccessorUnset;
	std::string symbol;
	std::string qualifier;
	std::string proseAfter;
};

bool ParseDeprecatedSuccessorArrowLine(const std::string& trimmedLine, DeprecatedSuccessorFields& fields);
bool FindAndParseDeprecatedSuccessorInText(const std::string& text, DeprecatedSuccessorFields& fields, std::string& textWithoutSuccessor);
void EmitDeprecatedSuccessorMissingWarning(const char* pszFileName, int lineNo, const char* pszSymbolName);
void EmitDeprecatedSuccessorNotInFileWarning(
	const char* pszFileName,
	int lineNo,
	const char* pszSymbolName,
	const char* pszSuccessorSymbol);
#endif

#endif /* SNACC_DEPRECATED_SUCCESSOR_H */
