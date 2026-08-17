#include "snacc-deprecated-successor.h"

#include "../../c-lib/include/asn-config.h"
#include <cctype>
#include <cstring>
#include <string>

namespace
{
	std::string trimLocal(const std::string& value)
	{
		size_t start = 0;
		while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
			++start;
		size_t end = value.size();
		while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
			--end;
		return value.substr(start, end - start);
	}

	bool startsWith(const std::string& value, const char* pszPrefix)
	{
		const char* pszSafe = pszPrefix ? pszPrefix : "";
		return value.rfind(pszSafe, 0) == 0;
	}

	bool parseArrowTarget(const std::string& arrowRemainder, DeprecatedSuccessorFields& fields, std::string& proseAfter)
	{
		std::string targetPart = trimLocal(arrowRemainder);
		if (startsWith(targetPart, "(none)"))
		{
			fields.scope = EDeprecatedSuccessorNone;
			fields.symbol.clear();
			fields.qualifier.clear();
			proseAfter = trimLocal(targetPart.substr(6));
			fields.proseAfter = proseAfter;
			fields.parsed = true;
			++fields.arrowCount;
			return true;
		}

		std::string targetToken;
		const size_t spacePos = targetPart.find(' ');
		if (spacePos != std::string::npos)
		{
			targetToken = targetPart.substr(0, spacePos);
			proseAfter = trimLocal(targetPart.substr(spacePos + 1));
		}
		else
			targetToken = targetPart;

		if (targetToken.empty())
			return false;

		const size_t qualifierPos = targetToken.find("::");
		if (qualifierPos == std::string::npos)
		{
			fields.scope = EDeprecatedSuccessorSameFile;
			fields.symbol = targetToken;
			fields.qualifier.clear();
		}
		else
		{
			fields.scope = EDeprecatedSuccessorQualified;
			fields.qualifier = targetToken.substr(0, qualifierPos);
			fields.symbol = targetToken.substr(qualifierPos + 2);
		}

		if (fields.symbol.empty())
			return false;

		fields.proseAfter = proseAfter;
		fields.parsed = true;
		++fields.arrowCount;
		return true;
	}
} // namespace

bool ParseDeprecatedSuccessorArrowLine(const std::string& trimmedLine, DeprecatedSuccessorFields& fields)
{
	std::string proseAfter;
	if (!startsWith(trimmedLine, "->"))
		return false;
	return parseArrowTarget(trimmedLine.substr(2), fields, proseAfter);
}

bool FindAndParseDeprecatedSuccessorInText(const std::string& text, DeprecatedSuccessorFields& fields, std::string& textWithoutSuccessor)
{
	const size_t arrowPos = text.find("->");
	if (arrowPos == std::string::npos)
	{
		textWithoutSuccessor = text;
		return false;
	}

	std::string proseAfter;
	if (!parseArrowTarget(text.substr(arrowPos + 2), fields, proseAfter))
	{
		textWithoutSuccessor = text;
		return false;
	}

	textWithoutSuccessor = trimLocal(text.substr(0, arrowPos));
	if (!proseAfter.empty())
	{
		if (!textWithoutSuccessor.empty())
			textWithoutSuccessor += " ";
		textWithoutSuccessor += proseAfter;
	}
	textWithoutSuccessor = trimLocal(textWithoutSuccessor);
	return true;
}

void EmitDeprecatedSuccessorMissingWarning(const char* pszFileName, int lineNo, const char* pszSymbolName)
{
	const char* pszFile = pszFileName ? pszFileName : "<unknown>";
	const char* pszSymbol = pszSymbolName ? pszSymbolName : "<unknown>";
	const int reportedLine = lineNo > 0 ? lineNo : 1;
	fprintf(
		stderr,
		"warning: %s:%d %s: @deprecated missing canonical successor (expected -> (none), -> Symbol, or -> Repo::Symbol)\n",
		pszFile,
		reportedLine,
		pszSymbol);
}

void EmitDeprecatedSuccessorNotInFileWarning(
	const char* pszFileName,
	int lineNo,
	const char* pszSymbolName,
	const char* pszSuccessorSymbol)
{
	const char* pszFile = pszFileName ? pszFileName : "<unknown>";
	const char* pszSymbol = pszSymbolName ? pszSymbolName : "<unknown>";
	const char* pszSuccessor = pszSuccessorSymbol ? pszSuccessorSymbol : "<unknown>";
	const int reportedLine = lineNo > 0 ? lineNo : 1;
	fprintf(
		stderr,
		"warning: %s:%d %s: @deprecated successor '%s' is not defined in this file\n",
		pszFile,
		reportedLine,
		pszSymbol,
		pszSuccessor);
}

void EmitDeprecatedSuccessorNotInModuleWarning(
	const char* pszFileName,
	int lineNo,
	const char* pszSymbolName,
	const char* pszSuccessorQualifier,
	const char* pszSuccessorSymbol,
	const char* pszResolvedModuleName)
{
	const char* pszFile = pszFileName ? pszFileName : "<unknown>";
	const char* pszSymbol = pszSymbolName ? pszSymbolName : "<unknown>";
	const char* pszQualifier = pszSuccessorQualifier ? pszSuccessorQualifier : "<unknown>";
	const char* pszSuccessor = pszSuccessorSymbol ? pszSuccessorSymbol : "<unknown>";
	const char* pszModule = pszResolvedModuleName ? pszResolvedModuleName : "<unknown>";
	const int reportedLine = lineNo > 0 ? lineNo : 1;
	fprintf(
		stderr,
		"warning: %s:%d %s: @deprecated successor '%s::%s' is not defined in loaded module '%s'\n",
		pszFile,
		reportedLine,
		pszSymbol,
		pszQualifier,
		pszSuccessor,
		pszModule);
}

extern "C" void PrintDeprecatedSuccessorHelp(FILE* fp)
{
	if (!fp)
		return;

	fprintf(fp, "   @deprecated canonical successor (warn-only; compilation continues):\n");
	fprintf(fp, "     -- @deprecated Day.Month.Year -> (none) [optional comment]\n");
	fprintf(fp, "     -- @deprecated Day.Month.Year -> SymbolInThisFile\n");
	fprintf(fp, "     -- @deprecated Day.Month.Year -> OtherModule::Symbol\n");
	fprintf(fp, "     -- @deprecated Day.Month.Year -> OtherRepo::Symbol\n");
	fprintf(fp, "     Arrow may be on the @deprecated line or the next -- comment line above the definition.\n");
	fprintf(fp, "     Unqualified successors must exist in the same file; qualified successors are checked when the\n");
	fprintf(fp, "     target module is part of the current esnacc invocation (UCWeb:: maps to loaded EUCWeb_* modules).\n");
	fprintf(fp, "     Validation applies to operations and SEQUENCE/CHOICE types only, not SEQUENCE or ENUMERATED members.\n");
	fprintf(fp, "     Legacy prose (see Symbol, moved to, file paths) without -> triggers a warning.\n");
}
