#include "gen-ts-combined.h"
#include "../../core/snacc-util.h"
#include "../str-util.h"
#include "../structure-util.h"
#include "../../../snacc.h"
#include "../../core/asn_comments.h"
#include "../../core/time_helpers.h"
#include <assert.h>
#include <string.h>

void PrintTSEscaped(FILE* src, const char* szData)
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

void PrintTSRootTypes(FILE* src, Module* mod, const char* szSuffix)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);
	fprintf(src, "export const MODULE_NAME = \"%s%s\";\n", mod->moduleName, szSuffix ? szSuffix : "");

	if (gMajorInterfaceVersion >= 0)
	{
		long long lMinorModuleVersion = GetModuleMinorVersion(mod->moduleName);
		fprintf(src, "export const MODULE_LASTCHANGE = \"%s\";\n", ConvertUnixTimeToISO(lMinorModuleVersion));
		fprintf(src, "export const MODULE_MAJOR_VERSION = %i;\n", gMajorInterfaceVersion);
		fprintf(src, "export const MODULE_MINOR_VERSION = %lld;\n", lMinorModuleVersion);
		fprintf(src, "export const MODULE_VERSION = \"%i.%lld.0\";\n", gMajorInterfaceVersion, lMinorModuleVersion);
	}
}

void PrintTSImports(FILE* src, ModuleList* mods, Module* mod, bool bIncludeConverters, bool bIncludeasn1ts, bool bIncludeDeprecatedCallback)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);
	if (bIncludeDeprecatedCallback)
	{
		bool bContainsDeprecated = false;
		TypeDef* td;
		FOR_EACH_LIST_ELMT(td, mod->typeDefs)
		{
			if (IsDeprecatedFlaggedSequence(mod, td->definedName))
			{
				bContainsDeprecated = true;
				break;
			}
		}
		if (!bContainsDeprecated)
		{
			ValueDef* vd;
			FOR_EACH_LIST_ELMT(vd, mod->valueDefs)
			{
				if (vd->value->basicValue->choiceId != BASICVALUE_INTEGER)
					continue;
				if (vd->value->type->basicType->choiceId != BASICTYPE_MACROTYPE)
					continue;
				if (vd->value->type->basicType->a.macroType->choiceId != MACROTYPE_ROSOPERATION)
					continue;
				if (IsDeprecatedFlaggedOperation(mod, vd->definedName))
					bContainsDeprecated = true;
			}
		}
		if (bContainsDeprecated)
			fprintf(src, "import { TSDeprecatedCallback } from \"./TSDeprecatedCallback%s\";\n", getCommonJSFileExtension());
	}
	if (bIncludeasn1ts)
		fprintf(src, "import * as asn1ts from \"@estos/asn1ts\";\n");

	if (mod->imports)
	{
		char szAlreadyAdded[4096] = {0};
		AsnListNode* saveImport = mod->imports->curr;
		ImportModule* impMod;
		FOR_EACH_LIST_ELMT(impMod, mod->imports)
		{
			Module* referencedModule = GetModuleForImportModule(mods, impMod);
			if (referencedModule)
			{
				impMod->moduleRef = referencedModule;
				const char* szNameSpace = GetNameSpace(referencedModule);
				if (strstr(szAlreadyAdded, szNameSpace) == NULL)
				{
					strcat_s(szAlreadyAdded, 4096, szNameSpace);
					fprintf(src, "import * as %s from \"./%s%s\";\n", szNameSpace, referencedModule->moduleName, getCommonJSFileExtension());
				}
			}
		}

		if (bIncludeConverters)
		{
			szAlreadyAdded[0] = 0;
			FOR_EACH_LIST_ELMT(impMod, mod->imports)
			{
				Module* referencedModule = GetModuleForImportModule(mods, impMod);
				if (referencedModule)
				{
					const char* szNameSpace = GetNameSpace(referencedModule);
					if (strstr(szAlreadyAdded, szNameSpace) == NULL)
					{
						strcat_s(szAlreadyAdded, 4096, szNameSpace);
						impMod->moduleRef = referencedModule;
						fprintf(src, "import * as %s_Converter from \"./%s_Converter%s\";\n", szNameSpace, referencedModule->moduleName, getCommonJSFileExtension());
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
		case BASICTYPE_EXTENSION:
			return "Extension";
		default:
			assert(FALSE);
	}

	exit(1);
}

const char* getCommonJSFileExtension()
{
	if (genTSESMCode)
		return ".js";
	else
		return "";
}
