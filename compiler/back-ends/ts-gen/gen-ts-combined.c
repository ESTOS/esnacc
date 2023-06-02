#include "gen-ts-combined.h"
#include "../../core/snacc-util.h"
#include "../str-util.h"
#include "../structure-util.h"
#include <assert.h>
#include <string.h>

void PrintTSEscaped(FILE* src, const char* szData) {
	const char* szPos = szData;
	const char* szLast = NULL;
	int iStars = 0;
	// char szWhiteSpaceBuffer[512] = { 0 };
	while (*szPos != 0) {
		if (*szPos == '@')
			fprintf(src, "\\");
		if (szLast) {
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
		else {
			if (strlen(szWhiteSpaceBuffer)) {
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

void PrintTSRootTypes(FILE* src, Module* mod, const char* szSuffix) {
	fprintf(src, "// [%s]\n", __FUNCTION__);
	fprintf(src, "export const moduleName = \"%s%s\";\n", mod->moduleName, szSuffix ? szSuffix : "");
}

void PrintTSImports(FILE* src, ModuleList* mods, Module* mod, bool bIncludeConverters, bool bIncludeasn1ts, bool bIncludeTASN1Base) {
	fprintf(src, "// [%s]\n", __FUNCTION__);
	if (bIncludeTASN1Base)
		fprintf(src, "import { TSASN1Base } from \"./TSASN1Base\";\n");
	if (bIncludeasn1ts)
		fprintf(src, "import * as asn1ts from \"@estos/asn1ts\";\n");

	if (mod->imports) {
		char szAlreadyAdded[4096] = { 0 };
		AsnListNode* saveImport = mod->imports->curr;
		ImportModule* impMod;
		FOR_EACH_LIST_ELMT(impMod, mod->imports)
		{
			Module* referencedModule = GetModuleForImportModule(mods, impMod);
			if (referencedModule) {
				impMod->moduleRef = referencedModule;
				const char* szNameSpace = GetNameSpace(referencedModule);
				if (strstr(szAlreadyAdded, szNameSpace) == NULL) {
					strcat_s(szAlreadyAdded, 4096, szNameSpace);
					fprintf(src, "import * as %s from \"./%s\";\n", szNameSpace, referencedModule->moduleName);
				}
			}
		}

		if (bIncludeConverters) {
			szAlreadyAdded[0] = 0;
			FOR_EACH_LIST_ELMT(impMod, mod->imports)
			{
				Module* referencedModule = GetModuleForImportModule(mods, impMod);
				if (referencedModule) {
					const char* szNameSpace = GetNameSpace(referencedModule);
					if (strstr(szAlreadyAdded, szNameSpace) == NULL) {
						strcat_s(szAlreadyAdded, 4096, szNameSpace);
						impMod->moduleRef = referencedModule;
						fprintf(src, "import * as %s_Converter from \"./%s_Converter\";\n", szNameSpace, referencedModule->moduleName);
					}
				}
			}
		}

		mod->imports->curr = saveImport;
	}
}

const char* GetBERType(const enum BasicTypeChoiceId basicTypeChoiseId)
{
	switch (basicTypeChoiseId)
	{
	case BASICTYPE_BOOLEAN:
		return "Boolean";
	case BASICTYPE_INTEGER:
		return "Integer";
	case BASICTYPE_ENUMERATED:
		return "Enumerated";
	case BASICTYPE_UTF8_STR:
		return "Utf8String";
	case BASICTYPE_OCTETSTRING:
	case BASICTYPE_OCTETCONTAINING:
		return "OctetString";
	case BASICTYPE_REAL:
		return "Real";
	case BASICTYPE_ASNSYSTEMTIME:
		return "AsnSystemTime";
	case BASICTYPE_NULL:
		return "Null";
	case BASICTYPE_ANY:
		return "Any";
	case BASICTYPE_UTCTIME:
		return "UTCTime";
	case BASICTYPE_CHOICE:
		return "Choice";
	case BASICTYPE_SEQUENCE:
		return "Sequence";
	case BASICTYPE_SET:
		return "Set";
	case BASICTYPE_SETOF:
		return "SequenceOf";
	case BASICTYPE_SEQUENCEOF:
		return "SetOf";
	default:
		assert(FALSE);
	}

	exit(1);
}

int GetContextID(struct Type* type) {
	int iResult = -1;
	if (type->tags->count) {
		Tag* pTag = NULL;
		FOR_EACH_LIST_ELMT(pTag, type->tags) {
			if (pTag->tclass == CNTX) {
				iResult = pTag->code;
				break;
			}
		}
	}
	return iResult;
}
