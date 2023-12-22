#include "gen-ts-converter.h"
#include "gen-ts-combined.h"
#include "../str-util.h"
#include "../structure-util.h"
#include "../tag-util.h"
#include "../../core/efileressources.h"
#include <assert.h>
#include <string.h>

void Print_JSON_EncoderCodeStructuredType(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs);
void Print_JSON_DecoderCodeStructuredType(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs, const char* szIndent);
void Print_JSON_EncoderNamedType(FILE* src, ModuleList* mods, Module* m, enum BasicTypeChoiceId type, NamedType* e, const char* szIndent);
void Print_BER_EncoderCodeStructuredType(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs);
void Print_BER_DecoderCodeStructuredType(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs, const char* szIndent);
void Print_BER_EncoderChoiceNamedType(FILE* src, ModuleList* mods, Module* m, enum BasicTypeChoiceId choiceId, Type* type, const char* szIndent, const char* szSourceFieldName, int iOptionalID);

void SaveTSConverterFilesToOutputDirectory(const char* szPath)
{
	{
		char szFileName[_MAX_PATH] = {0};
		strcpy_s(szFileName, _MAX_PATH, szPath);
		strcat_s(szFileName, _MAX_PATH, "TSConverterBase.ts");
		SaveResourceToFile(ETS_CONVERTER_BASE, szFileName);
	}
}

void PrintTSConverterComments(FILE* src, Module* m)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);
	fprintf(src, "/*\n");
	fprintf(src, " * %s\n", RemovePath(m->tsConverterFileName));
	fprintf(src, " * \"%s\" ASN.1 stubs.\n", m->modId->name);
	write_snacc_header(src, " * ");
	fprintf(src, " */\n\n");
	fprintf(src, "// prettier-ignore\n");
	fprintf(src, ESLINT_DISABLE);
}

void PrintTSConverterImports(FILE* src, ModuleList* mods, Module* mod)
{
	fprintf(src, "\n// [%s]\n", __FUNCTION__);

	fprintf(src, "import { ConverterError, ConverterErrorType, ConverterErrors, TSConverter, IDecodeContext, IEncodeContext, INamedType } from \"./%s%s\";\n", "TSConverterBase", getCommonJSFileExtension());

	// Our own data structure file is not in the imports
	fprintf(src, "import * as %s from \"./%s%s\";\n", GetNameSpace(mod), mod->moduleName, getCommonJSFileExtension());
	if (strcmp(mod->modId->name, "UC-Server-Access-Protocol-Common") == 0)
		fprintf(src, "import { EAsnOptionalParametersConverter } from \"./TSOptionalParamConverter%s\";\n", getCommonJSFileExtension());

	PrintTSImports(src, mods, mod, true, true, false);
}

const char* GetJSONType(enum BasicTypeChoiceId choiceId)
{
	switch (choiceId)
	{
		case BASICTYPE_BOOLEAN:
			return "boolean";
		case BASICTYPE_INTEGER:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_REAL:
			return "number";
		case BASICTYPE_UTF8_STR:
			return "string";
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			return "Uint8Array";
		case BASICTYPE_NULL:
		case BASICTYPE_ANY:
			return "object";
		case BASICTYPE_ASNSYSTEMTIME:
			return "Date";
		default:
			snacc_exit("unknown choiceId %d", choiceId);
	}

	return "";
}

void Print_JSON_EncoderSetOfDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seqOf, int novolatilefuncs)
{
	fprintf(src, "\t\t// [%s]\n", __FUNCTION__);

	char* szConverted = FixName(td->definedName);

	// Unser Sequence Typ
	enum BasicTypeChoiceId choice = seqOf->basicType->a.sequenceOf->basicType->choiceId;

	const char* szTypeName = NULL;
	BasicType* pBase = GetBaseBasicType(seqOf->basicType->a.sequenceOf->basicType, &szTypeName);
	enum BasicTypeChoiceId baseChoice = pBase->choiceId;
	if (baseChoice == BASICTYPE_OCTETCONTAINING && pBase->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
		baseChoice = BASICTYPE_UTF8_STR;

	// Ist der Typ den wir iterieren eine basis Type?
	if (IsSimpleType(baseChoice))
	{
		fprintf(src, "\t\tfor (const se of s) {\n");
		fprintf(src, "\t\t\tif (TSConverter.validateParamType(se, \"%s\", \"%s\", errors, newContext, %s))\n", szConverted, GetJSONType(baseChoice), td->type->optional ? "true" : "false");
		fprintf(src, "\t\t\t\tt.push(se);\n");
		fprintf(src, "\t\t}\n");
	}
	else
	{
		fprintf(src, "\t\tfor (const id in s) {\n");
		fprintf(src, "\t\t\tconst se = s[id];\n");
		fprintf(src, "\t\t\tif (se === undefined)\n");
		fprintf(src, "\t\t\t\tcontinue;\n");
		if (choice == BASICTYPE_IMPORTTYPEREF)
		{
			Module* mod = GetImportModuleRefByClassName(szTypeName, mods, m);
			const char* szNameSpace = GetNameSpace(mod);
			fprintf(src, "\t\t\tconst val = %s_Converter.%s_Converter.toJSON(se, errors, newContext, \"%s\")\n", szNameSpace, szTypeName, szTypeName);
			fprintf(src, "\t\t\tif (val)\n");
			fprintf(src, "\t\t\t\tt.push(val);\n");
		}
		else if (choice == BASICTYPE_LOCALTYPEREF)
		{
			fprintf(src, "\t\t\tconst val = %s_Converter.toJSON(se, errors, newContext, \"%s\");\n", szTypeName, szTypeName);
			fprintf(src, "\t\t\tif (val)\n");
			fprintf(src, "\t\t\t\tt.push(val);\n");
		}
		else
		{
			assert(FALSE);
		}
		fprintf(src, "\t\t}\n");
	}

	free(szConverted);
}

void Print_BER_EncoderSetOfDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seqOf, int novolatilefuncs)
{
	fprintf(src, "\n\t\t// [%s]\n", __FUNCTION__);

	char* szConverted = FixName(td->definedName);

	// Unser Sequence Typ
	enum BasicTypeChoiceId choice = seqOf->basicType->a.sequenceOf->basicType->choiceId;

	// Basis Typ der Sequence
	BasicType* pRootBasicType = GetRootType(seqOf, NULL)->basicType;
	enum BasicTypeChoiceId rootChoiceId = pRootBasicType->choiceId;
	if (rootChoiceId == BASICTYPE_OCTETCONTAINING && pRootBasicType->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
		rootChoiceId = BASICTYPE_UTF8_STR;

	const char* szTypeName = NULL;
	BasicType* pBase = GetBaseBasicType(seqOf->basicType->a.sequenceOf->basicType, &szTypeName);
	enum BasicTypeChoiceId baseChoice = pBase->choiceId;
	if (baseChoice == BASICTYPE_OCTETCONTAINING && pBase->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
		baseChoice = BASICTYPE_UTF8_STR;

	// Ist der Typ den wir iterieren eine basis Type?
	if (IsSimpleType(baseChoice))
	{
		fprintf(src, "\t\tfor (const se of s) {\n");
		fprintf(src, "\t\t\tif (TSConverter.validateParamType(se, \"%s\", \"%s\", errors, newContext, %s))\n", szConverted, GetJSONType(rootChoiceId), td->type->optional ? "true" : "false");
		fprintf(src, "\t\t\t\tt.push(new asn1ts.%s({ value: se }));\n", GetBERType(baseChoice));
		fprintf(src, "\t\t}\n");
	}
	else
	{
		fprintf(src, "\t\tfor (const id in s) {\n");
		if (choice == BASICTYPE_IMPORTTYPEREF)
		{
			Module* mod = GetImportModuleRefByClassName(szTypeName, mods, m);
			const char* szNameSpace = GetNameSpace(mod);
			fprintf(src, "\t\t\tconst val = %s_Converter.%s_Converter.toBER(s[id], errors, newContext, \"%s\");\n", szNameSpace, szTypeName, szTypeName);
		}
		else if (choice == BASICTYPE_LOCALTYPEREF)
			fprintf(src, "\t\t\tconst val = %s_Converter.toBER(s[id], errors, newContext, \"%s\");\n", szTypeName, szTypeName);
		else
			snacc_exit("invalid choice %d", choice);
		fprintf(src, "\t\t\tif (val)\n");
		fprintf(src, "\t\t\t\tt.push(val);\n");
		fprintf(src, "\t\t}\n\n");
	}

	fprintf(src, "\t\tif (errors.validateResult(newContext, \"%s\"))\n", szTypeName);
	fprintf(src, "\t\t\treturn result;\n\n");

	free(szConverted);
}

void Print_JSON_DecoderSetOfDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seqOf, int novolatilefuncs, const char* szIdendt)
{
	fprintf(src, "\n%s// [%s]\n", szIdendt, __FUNCTION__);

	char* szConverted = FixName(td->definedName);
	const char* szClassName = seqOf->basicType->a.sequenceOf->cxxTypeRefInfo->className;
	const char* szTypeName = NULL;
	BasicType* type = GetRootType(seqOf->basicType->a.sequenceOf, &szTypeName)->basicType;

	if (IsSimpleType(type->choiceId))
	{
		const char* szType = "";
		const char* szElement = "";
		bool bInstanceOfCompare = false;

		enum BasicTypeChoiceId choiceId = type->choiceId;
		if (choiceId == BASICTYPE_OCTETCONTAINING && type->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
			choiceId = BASICTYPE_UTF8_STR;

		switch (type->choiceId)
		{
			case BASICTYPE_BOOLEAN:
				szElement = "se";
				szType = "boolean";
				break;
			case BASICTYPE_INTEGER:
			case BASICTYPE_ENUMERATED:
			case BASICTYPE_REAL:
				szElement = "se";
				szType = "number";
				break;
			case BASICTYPE_UTF8_STR:
				szElement = "se";
				szType = "string";
				break;
			case BASICTYPE_ASNSYSTEMTIME:
				// Used internally for AsnSystemTime
				szElement = "addDateParam(elem)";
				szType = "Date";
				bInstanceOfCompare = true;
				break;
			case BASICTYPE_OCTETSTRING:
			case BASICTYPE_OCTETCONTAINING:
				szElement = "TSConverter.decode64(elem)";
				szType = "Uint8Array";
				bInstanceOfCompare = true;
				break;
			default:
				snacc_exit("unknown type->choiceId %d", type->choiceId);
		}

		fprintf(src, "%sfor (const se of s) {\n", szIdendt);
		if (bInstanceOfCompare)
			fprintf(src, "%s\tif (TSConverter.validateParamType(se, \"%s\", se instanceof %s, errors, newContext, false))\n", szIdendt, szConverted, szType);
		else
			fprintf(src, "%s\tif (TSConverter.validateParamType(se, \"%s\", \"%s\", errors, newContext, false))\n", szIdendt, szConverted, szType);
		fprintf(src, "%s\t\tt.push(%s);\n", szIdendt, szElement);
		fprintf(src, "%s}\n", szIdendt);
	}
	else
	{
		fprintf(src, "%sfor (const id in s) {\n", szIdendt);
		fprintf(src, "%s\tconst se = s[id];\n", szIdendt);
		fprintf(src, "%s\tif (se === undefined)\n", szIdendt);
		fprintf(src, "%s\t\tcontinue;\n", szIdendt);
		if (seqOf->basicType->a.sequenceOf->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			Module* mod = GetImportModuleRefByClassName(szTypeName, mods, m);
			fprintf(src, "%s\tconst val = %s_Converter.%s_Converter.fromJSON(se, errors, newContext, \"%s\", false);\n", szIdendt, GetNameSpace(mod), szClassName, szClassName);
		}
		else
			fprintf(src, "%s\tconst val = %s_Converter.fromJSON(se, errors, newContext, \"%s\", false);\n", szIdendt, szClassName, szClassName);
		fprintf(src, "%s\tif (val)\n", szIdendt);
		fprintf(src, "%s\t\tt.push(val);\n", szIdendt);
		fprintf(src, "%s}\n", szIdendt);
	}

	free(szConverted);
}

void Print_BER_DecoderSetOfDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seqOf, int novolatilefuncs, const char* szIdendt)
{
	fprintf(src, "\n%s// [%s]\n", szIdendt, __FUNCTION__);

	char* szConverted = FixName(td->definedName);
	const char* szClassName = seqOf->basicType->a.sequenceOf->cxxTypeRefInfo->className;
	const char* szTypeName = NULL;
	BasicType* type = GetRootType(seqOf->basicType->a.sequenceOf, &szTypeName)->basicType;

	enum BasicTypeChoiceId choiceId = type->choiceId;
	if (choiceId == BASICTYPE_OCTETCONTAINING && type->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
		choiceId = BASICTYPE_UTF8_STR;

	fprintf(src, "%sfor (const se of s.valueBlock.value) {\n", szIdendt);
	fprintf(src, "%s\tif (asn1ts.%s.typeGuard(se)) {\n", szIdendt, GetBERType(choiceId));

	if (IsSimpleType(choiceId))
	{
		bool bInstanceOfCompare = choiceId == BASICTYPE_OCTETCONTAINING || choiceId == BASICTYPE_OCTETSTRING;

		fprintf(src, "%s\t\tconst value = se.getValue();\n", szIdendt);
		fprintf(src, "%s\t\tif (TSConverter.validateParamType(value, \"%s\", ", szIdendt, szConverted);
		if (bInstanceOfCompare)
			fprintf(src, "data instanceof \"%s\"", GetJSONType(choiceId));
		else
			fprintf(src, "\"%s\"", GetJSONType(choiceId));
		fprintf(src, ", errors, newContext, false))");

		if (type->choiceId == BASICTYPE_ASNSYSTEMTIME)
		{
			fprintf(src, " {\n");
			fprintf(src, "%s\t\t\tconst date = TSConverter.getDateTimeFromVariantTime(value);\n", szIdendt);
			fprintf(src, "%s\t\t\tt.push(date);\n", szIdendt);
			fprintf(src, "%s\t\t}\n", szIdendt);
		}
		else
		{
			fprintf(src, "\n");
			fprintf(src, "%s\t\t\tt.push(value);\n", szIdendt);
		}
	}
	else
	{
		if (seqOf->basicType->a.sequenceOf->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			Module* mod = GetImportModuleRefByClassName(szTypeName, mods, m);
			fprintf(src, "%s\t\tconst val = %s_Converter.%s_Converter.fromBER(se, errors, newContext, \"%s\", optional);\n", szIdendt, GetNameSpace(mod), szClassName, szClassName);
		}
		else
			fprintf(src, "%s\t\tconst val = %s_Converter.fromBER(se, errors, newContext, \"%s\", optional);\n", szIdendt, szClassName, szClassName);
		fprintf(src, "%s\t\tif (val)\n", szIdendt);
		fprintf(src, "%s\t\t\tt.push(val);\n", szIdendt);
	}
	fprintf(src, "%s\t} else\n", szIdendt);
	fprintf(src, "%s\t\terrors.push(new ConverterError(ConverterErrorType.PROPERTY_TYPEMISMATCH, newContext.context, \"wrong type\"));\n", szIdendt);
	fprintf(src, "%s}\n", szIdendt);

	free(szConverted);
}

void Print_JSON_EncoderChoiceDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* choice, int novolatilefuncs)
{
	fprintf(src, "\t\t// [%s]\n", __FUNCTION__);

	char* szConverted = FixName(td->definedName);
	fprintf(src, "\t\tif (newContext.bAddTypes)\n");
	fprintf(src, "\t\t\tt._type = \"%s\";\n", szConverted);
	int bFirst = TRUE;
	bool bCurly = false;
	NamedType* e;
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.sequence)
	{
		fprintf(src, "\t\t");
		if (bCurly)
		{
			fprintf(src, "} ");
			bCurly = false;
		}

		fprintf(src, "%s (s.%s != null)", bFirst ? "if" : "else if", e->fieldName);

		enum BasicTypeChoiceId choiceId = e->type->basicType->choiceId;
		if (choiceId == BASICTYPE_LOCALTYPEREF || choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			if (strcmp(e->type->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
				choiceId = BASICTYPE_ASNSYSTEMTIME;
			else
			{
				enum BasicTypeChoiceId baseType = GetBaseBasicTypeChoiceId(e->type->basicType);
				if (IsSimpleType(baseType))
					choiceId = baseType;
			}
		}

		if (choiceId == BASICTYPE_LOCALTYPEREF || choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			bCurly = true;
			fprintf(src, " {\n");
		}
		else
			fprintf(src, "\n");

		Print_JSON_EncoderNamedType(src, mods, m, choiceId, e, "\t\t\t");

		bFirst = FALSE;
	}

	fprintf(src, "\t\t");
	if (bCurly)
		fprintf(src, "} ");
	fprintf(src, "else\n");
	fprintf(src, "\t\t\terrors.push(new ConverterError(ConverterErrorType.PROPERTY_MISSING, newContext.context, \"property missing\"));\n");

	free(szConverted);
}

void Print_BER_EncoderChoiceDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* choice, int novolatilefuncs)
{
	fprintf(src, "\n\t\t// [%s]\n", __FUNCTION__);

	char* szConverted = FixName(td->definedName);
	int iPropertyCounter = 1;
	NamedType* e;
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.sequence)
	{
		Type* type = e->type;
		enum BasicTypeChoiceId choiceId = type->basicType->choiceId;
		if (choiceId == BASICTYPE_LOCALTYPEREF || choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			if (strcmp(type->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
				choiceId = BASICTYPE_ASNSYSTEMTIME;
			else
			{
				enum BasicTypeChoiceId baseType = GetBaseBasicTypeChoiceId(e->type->basicType);
				if (IsSimpleType(baseType))
				{
					choiceId = baseType;
					type = type->basicType->a.importTypeRef->link->type;
				}
			}
		}
		char* szSourceFieldName = FixName(e->fieldName);

		const char* szIndent = "\t\t\t";
		if (iPropertyCounter == 1)
			fprintf(src, "\t\tif ");
		else
			fprintf(src, "\t\telse if ");

		if (choiceId == BASICTYPE_LOCALTYPEREF || choiceId == BASICTYPE_IMPORTTYPEREF)
			fprintf(src, "(s.%s)\n", szSourceFieldName);

		Print_BER_EncoderChoiceNamedType(src, mods, m, choiceId, type, szIndent, szSourceFieldName, GetContextID(e->type));
		free(szSourceFieldName);
		iPropertyCounter++;
	}
	fprintf(src, "\t\telse\n");
	fprintf(src, "\t\t\terrors.push(new ConverterError(ConverterErrorType.PROPERTY_MISSING, newContext.context, \"property missing\"));\n");

	fprintf(src, "\t\tif (errors.validateResult(newContext, \"%s\"))\n", szConverted);
	fprintf(src, "\t\t\treturn t;\n");

	free(szConverted);
}

void Print_JSON_DecoderChoiceDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* choice, int novolatilefuncs, const char* szIdendt)
{
	fprintf(src, "\n%s// [%s]\n", szIdendt, __FUNCTION__);

	char* szConverted = FixName(td->definedName);

	bool bFirst = true;
	bool bMultiLine = false;
	NamedType* e;
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.sequence)
	{
		BasicType* type = e->type->basicType;
		const char* szOptional = e->type->optional ? "true" : "false";
		const char* szTypeName = NULL;
		BasicType* newtype = GetRootType(e->type, &szTypeName)->basicType;
		if (IsSimpleType(newtype->choiceId))
			type = newtype;
		enum BasicTypeChoiceId choiceId = type->choiceId;

		fprintf(src, "%s", szIdendt);
		if (bMultiLine)
			fprintf(src, "} ");
		if (bFirst)
			bFirst = false;
		else
			fprintf(src, "else ");
		fprintf(src, "if (s.%s !== undefined)", e->fieldName);

		bMultiLine = choiceId != BASICTYPE_LOCALTYPEREF && choiceId != BASICTYPE_IMPORTTYPEREF;
		if (bMultiLine)
			fprintf(src, " {");
		fprintf(src, "\n");

		if (choiceId == BASICTYPE_OCTETCONTAINING && type->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
			choiceId = BASICTYPE_UTF8_STR;

		switch (choiceId)
		{
			case BASICTYPE_BOOLEAN:
			case BASICTYPE_INTEGER:
			case BASICTYPE_ENUMERATED:
			case BASICTYPE_REAL:
			case BASICTYPE_UTF8_STR:
			case BASICTYPE_ASNSYSTEMTIME:
			case BASICTYPE_NULL:
				fprintf(src, "%s\tif (TSConverter.validateParam(s, \"%s\", \"%s\", errors, newContext))\n", szIdendt, e->fieldName, GetJSONType(choiceId));
				fprintf(src, "%s\t\tt.%s = s.%s;\n", szIdendt, e->fieldName, e->fieldName);
				break;
			case BASICTYPE_OCTETCONTAINING:
			case BASICTYPE_OCTETSTRING:
				fprintf(src, "%s\tif (TSConverter.validateParam(s, \"%s\", \"string\", errors, newContext))\n", szIdendt, e->fieldName);
				fprintf(src, "%s\t\tt.%s = TSConverter.decode64(s.%s as unknown as string);\n", szIdendt, e->fieldName, e->fieldName);
				break;
			case BASICTYPE_LOCALTYPEREF:
			case BASICTYPE_IMPORTTYPEREF:
				{
					Module* mod = m;
					if (choiceId == BASICTYPE_IMPORTTYPEREF)
						mod = GetImportModuleRefByClassName(e->type->cxxTypeRefInfo->className, mods, m);
					const char* ns = GetNameSpace(mod);
					if (mod == m)
						fprintf(src, "%s\tt.%s = %s_Converter.fromJSON(s.%s, errors, newContext, \"%s\", %s);\n", szIdendt, e->fieldName, e->type->cxxTypeRefInfo->className, e->fieldName, e->fieldName, szOptional);
					else
						fprintf(src, "%s\tt.%s = %s_Converter.%s_Converter.fromJSON(s.%s, errors, newContext, \"%s\", %s);\n", szIdendt, e->fieldName, ns, e->type->cxxTypeRefInfo->className, e->fieldName, e->fieldName, szOptional);
				}
				break;
			case BASICTYPE_EXTENSION:
				// Do Nothing here
				break;
			default:
				snacc_exit("unknown choiceId %d", choiceId);
		}
	}
	fprintf(src, "%s", szIdendt);
	if (bMultiLine)
		fprintf(src, "} ");
	fprintf(src, "else if (!(optional === true))\n");
	fprintf(src, "%s\terrors.push(new ConverterError(ConverterErrorType.PROPERTY_MISSING, newContext.context, \"Property has not been filled\"));\n", szIdendt);

	free(szConverted);
}

void Print_BER_DecoderChoiceDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* choice, int novolatilefuncs, const char* szIdendt)
{
	fprintf(src, "\n%s// [%s]\n", szIdendt, __FUNCTION__);

	char* szConverted = FixName(td->definedName);

	bool bFirst = true;
	bool bMultiLine = false;
	NamedType* e;
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.sequence)
	{
		BasicType* pBasicType = ResolveBasicTypeReferences(e->type->basicType, NULL);
		enum BasicTypeChoiceId choiceId = pBasicType->choiceId;
		const char* szOptional = e->type->optional ? "true" : "false";

		fprintf(src, "%s", szIdendt);
		if (bMultiLine)
			fprintf(src, "} ");
		if (bFirst)
			bFirst = false;
		else
			fprintf(src, "else ");

		enum BasicTypeChoiceId compareChoiceID = choiceId;
		if (compareChoiceID == BASICTYPE_SEQUENCEOF)
			compareChoiceID = BASICTYPE_SEQUENCE;
		else if (compareChoiceID == BASICTYPE_SETOF)
			compareChoiceID = BASICTYPE_SET;
		fprintf(src, "if (s.choiceName === \"%s\" && asn1ts.%s.typeGuard(s))", e->fieldName, GetBERType(compareChoiceID));

		bMultiLine = compareChoiceID != BASICTYPE_SEQUENCE && compareChoiceID != BASICTYPE_SET;
		if (bMultiLine)
			fprintf(src, " {");
		fprintf(src, "\n");

		if (choiceId == BASICTYPE_OCTETCONTAINING && pBasicType->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
			choiceId = BASICTYPE_UTF8_STR;

		switch (choiceId)
		{
			case BASICTYPE_BOOLEAN:
			case BASICTYPE_INTEGER:
			case BASICTYPE_ENUMERATED:
			case BASICTYPE_REAL:
			case BASICTYPE_UTF8_STR:
			case BASICTYPE_NULL:
				fprintf(src, "%s\tconst _%s = s.getValue();\n", szIdendt, e->fieldName);
				fprintf(src, "%s\tif (TSConverter.validateParamType(_%s, \"%s\", \"%s\", errors, newContext, %s))\n", szIdendt, e->fieldName, e->fieldName, GetJSONType(choiceId), szOptional);
				fprintf(src, "%s\t\tt.%s = _%s;\n", szIdendt, e->fieldName, e->fieldName);
				break;
			case BASICTYPE_OCTETCONTAINING:
			case BASICTYPE_OCTETSTRING:
				fprintf(src, "%s\tconst _%s = s.getValue();\n", szIdendt, e->fieldName);
				fprintf(src, "%s\tif (TSConverter.validateParamType(_%s, \"%s\", \"%s\", errors, newContext, %s))\n", szIdendt, e->fieldName, e->fieldName, GetJSONType(choiceId), szOptional);
				fprintf(src, "%s\t\tt.%s = new Uint8Array(_%s);\n", szIdendt, e->fieldName, e->fieldName);
				break;
			case BASICTYPE_ASNSYSTEMTIME:
				fprintf(src, "%s\tconst _%s = s.getValue();\n", szIdendt, e->fieldName);
				fprintf(src, "%s\tif (TSConverter.validateParamType(_%s, \"%s\", \"%s\", errors, newContext, %s))\n", szIdendt, e->fieldName, e->fieldName, GetJSONType(choiceId), szOptional);
				fprintf(src, "%s\t\tt.%s = TSConverter.getDateTimeFromVariantTime(_%s);\n", szIdendt, e->fieldName, e->fieldName);
				break;
			case BASICTYPE_SEQUENCEOF:
			case BASICTYPE_SETOF:
			case BASICTYPE_SEQUENCE:
			case BASICTYPE_SET:
				{
					size_t size = strlen(e->fieldName) + 1;
					char* pBuffer = malloc(size);
					if (!pBuffer)
					{
						snacc_exit("Out of memory");
						return;
					}
					pBuffer[0] = 0;
					strcpy_s(pBuffer, size, e->fieldName);
					_strlwr_s(pBuffer, size);

					Module* mod = m;
					if (e->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
						mod = GetImportModuleRefByClassName(e->type->cxxTypeRefInfo->className, mods, m);
					const char* ns = GetNameSpace(mod);
					if (mod == m)
						fprintf(src, "%s\tt.%s = %s_Converter.fromBER(s, undefined, newContext, \"%s\", %s);\n", szIdendt, e->fieldName, e->type->cxxTypeRefInfo->className, pBuffer, szOptional);
					else
						fprintf(src, "%s\tt.%s = %s_Converter.%s_Converter.fromBER(s, undefined, newContext, \"%s\", %s);\n", szIdendt, e->fieldName, ns, e->type->cxxTypeRefInfo->className, pBuffer, szOptional);
					free(pBuffer);
				}
				break;
			case BASICTYPE_EXTENSION:
				// Do Nothing here
				break;
			default:
				snacc_exit("unknown choiceId %d", choiceId);
		}
	}
	fprintf(src, "%s", szIdendt);
	if (bMultiLine)
		fprintf(src, "} ");
	fprintf(src, "else if (!(optional === true))\n");
	fprintf(src, "%s\terrors.push(new ConverterError(ConverterErrorType.PROPERTY_MISSING, newContext.context, \"Property has not been filled\"));\n", szIdendt);

	free(szConverted);
}

void Print_JSON_EncoderNamedType(FILE* src, ModuleList* mods, Module* m, enum BasicTypeChoiceId type, NamedType* e, const char* szIndent)
{
	char* szFieldName = FixName(e->fieldName);
	const char* szObjectName = "t";
	const bool bOptional = e->type->optional;
	const char* szClassName = e->type->cxxTypeRefInfo->className;
	const char* szOptional = bOptional ? ", true" : "";

	switch (type)
	{
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_INTEGER:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_REAL:
		case BASICTYPE_UTF8_STR:
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_NULL:
		case BASICTYPE_ANY:
			fprintf(src, "%sTSConverter.fillJSONParam(s, %s, \"%s\", \"%s\", errors, newContext%s);\n", szIndent, szObjectName, szFieldName, GetJSONType(type), szOptional);
			break;
		case BASICTYPE_ASNSYSTEMTIME:
			fprintf(src, "%sTSConverter.fillJSONParam(s, %s, \"%s\", \"Date\", errors, newContext%s);\n", szIndent, szObjectName, szFieldName, szOptional);
			break;
		case BASICTYPE_LOCALTYPEREF:
		case BASICTYPE_IMPORTTYPEREF:
			{
				const char* szIndent2 = "";
				if (bOptional)
				{
					fprintf(src, "%sif (s.%s) {\n", szIndent, szFieldName);
					szIndent2 = "\t";
				}

				const char* szNameSpace = "";
				if (type == BASICTYPE_LOCALTYPEREF)
					szNameSpace = GetNameSpace(m);
				else
				{
					Module* ref = GetImportModuleRefByClassName(szClassName, mods, m);
					szNameSpace = GetNameSpace(ref);
				}
				if (type == BASICTYPE_LOCALTYPEREF)
					fprintf(src, "%s%sconst _%s = %s_Converter.toJSON(s.%s, errors, newContext, \"%s\");\n", szIndent, szIndent2, szFieldName, szClassName, szFieldName, szFieldName);
				else
					fprintf(src, "%s%sconst _%s = %s_Converter.%s_Converter.toJSON(s.%s, errors, newContext, \"%s\");\n", szIndent, szIndent2, szFieldName, szNameSpace, szClassName, szFieldName, szFieldName);
				fprintf(src, "%s%sif (_%s)\n", szIndent, szIndent2, szFieldName);
				fprintf(src, "%s%s\t%s%s%s = _%s;\n", szIndent, szIndent2, szObjectName, szObjectName ? "." : "", szFieldName, szFieldName);
				if (bOptional)
					fprintf(src, "%s}\n", szIndent);
				break;
			}
		case BASICTYPE_EXTENSION:
			break;
		default:
			snacc_exit("unknown type %d", type);
	}
	free(szFieldName);
}

void Print_BER_EncoderChoiceNamedType(FILE* src, ModuleList* mods, Module* m, enum BasicTypeChoiceId choiceId, Type* type, const char* szIndent, const char* szSourceFieldName, int iOptionalID)
{
	char szOptionalParam[128] = {0};
	char szOptionalID[128] = {0};
	if (iOptionalID >= 0)
	{
		sprintf_s(szOptionalParam, sizeof(szOptionalParam), ", idBlock: { optionalID: %i }", iOptionalID);
		sprintf_s(szOptionalID, sizeof(szOptionalID), ", %i", iOptionalID);
	}

	switch (choiceId)
	{
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_INTEGER:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_REAL:
		case BASICTYPE_UTF8_STR:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_NULL:
		case BASICTYPE_ANY:
		case BASICTYPE_ASNSYSTEMTIME:
			fprintf(src, "(TSConverter.validateParam(s, \"%s\", \"%s\", errors, newContext, true))\n", szSourceFieldName, GetJSONType(choiceId));
			if (choiceId == BASICTYPE_OCTETSTRING)
				fprintf(src, "%st = new asn1ts.%s({ valueHex: s.%s, name: \"%s\"%s });\n", szIndent, GetBERType(choiceId), szSourceFieldName, szSourceFieldName, szOptionalParam);
			else if (choiceId == BASICTYPE_NULL)
				fprintf(src, "%st = new asn1ts.%s({ name: \"%s\"%s });\n", szIndent, GetBERType(choiceId), szSourceFieldName, szOptionalParam);
			else if (choiceId == BASICTYPE_ANY)
			{
				fprintf(src, "%s// eslint-disable-next-line @typescript-eslint/no-unsafe-assignment\n", szIndent);
				fprintf(src, "%st = new asn1ts.%s({ value: s.%s, name: \"%s\"%s });\n", szIndent, GetBERType(choiceId), szSourceFieldName, szSourceFieldName, szOptionalParam);
			}
			else
				fprintf(src, "%st = new asn1ts.%s({ value: s.%s, name: \"%s\"%s });\n", szIndent, GetBERType(choiceId), szSourceFieldName, szSourceFieldName, szOptionalParam);
			break;
		case BASICTYPE_LOCALTYPEREF:
		case BASICTYPE_IMPORTTYPEREF:
			{
				const char* szNameSpace = "";
				const char* szClassName = type->cxxTypeRefInfo->className;
				if (choiceId == BASICTYPE_LOCALTYPEREF)
					szNameSpace = GetNameSpace(m);
				else
				{
					Module* ref = GetImportModuleRefByClassName(szClassName, mods, m);
					szNameSpace = GetNameSpace(ref);
				}
				if (choiceId == BASICTYPE_LOCALTYPEREF)
					fprintf(src, "%st = %s_Converter.toBER(s.%s, errors, newContext, \"%s\"%s);\n", szIndent, szClassName, szSourceFieldName, szSourceFieldName, szOptionalID);
				else
					fprintf(src, "%st = %s_Converter.%s_Converter.toBER(s.%s, errors, newContext, \"%s\"%s);\n", szIndent, szNameSpace, szClassName, szSourceFieldName, szSourceFieldName, szOptionalID);
				break;
			}
		case BASICTYPE_EXTENSION:
			break;
		default:
			snacc_exit("unknown choiceId %d", choiceId);
	}
	// free(szSourceFieldName);
}

void Print_BER_EncoderValidateProperty(FILE* src, ModuleList* mods, Module* m, enum BasicTypeChoiceId type, NamedType* e, const char* szIndent)
{
	if (type != BASICTYPE_EXTENSION)
	{
		const bool bOptional = e->type->optional;
		const char* szOptional = bOptional ? ", true" : "";
		char szOptionalParam[128] = {0};
		if (bOptional)
		{
			int iOptionalID = GetContextID(e->type);
			if (iOptionalID >= 0)
				sprintf_s(szOptionalParam, sizeof(szOptionalParam), ", %i", iOptionalID);
			else
				sprintf_s(szOptionalParam, sizeof(szOptionalParam), "%s", ", true");
		}

		char* szFieldName = FixName(e->fieldName);

		switch (type)
		{
			case BASICTYPE_UTF8_STR:
			case BASICTYPE_BOOLEAN:
			case BASICTYPE_INTEGER:
			case BASICTYPE_REAL:
			case BASICTYPE_ENUMERATED:
			case BASICTYPE_ANY:
				fprintf(src, "%sTSConverter.validateParam(s, \"%s\", \"%s\", errors, newContext%s);\n", szIndent, szFieldName, GetJSONType(type), szOptional);
				break;
			case BASICTYPE_OCTETSTRING:
			case BASICTYPE_OCTETCONTAINING:
				fprintf(src, "%sTSConverter.validateParam(s, \"%s\", \"Uint8Array\", errors, newContext%s);\n", szIndent, szFieldName, szOptional);
				break;
			case BASICTYPE_ASNSYSTEMTIME:
				fprintf(src, "%sTSConverter.validateParam(s, \"%s\", \"Date\", errors, newContext%s);\n", szIndent, szFieldName, szOptional);
				break;
			case BASICTYPE_LOCALTYPEREF:
			case BASICTYPE_IMPORTTYPEREF:
				{
					const char* szIndent2 = "";
					const char* szClassName = e->type->cxxTypeRefInfo->className;
					if (type == BASICTYPE_IMPORTTYPEREF)
						m = GetImportModuleRefByClassName(szClassName, mods, m);
					const char* szNameSpace = GetNameSpace(m);

					fprintf(src, "%s%sconst _%s = ", szIndent, szIndent2, szFieldName);
					if (type == BASICTYPE_IMPORTTYPEREF)
						fprintf(src, "%s_Converter.", szNameSpace);
					fprintf(src, "%s_Converter.toBER(s.%s, errors, newContext, \"%s\"%s);\n", szClassName, szFieldName, szFieldName, szOptionalParam);

					break;
				}
			case BASICTYPE_NULL:
				break;
			default:
				snacc_exit("unknown type %d", type);
		}

		free(szFieldName);
	}
}

void Print_BER_EncoderAssignProperty(FILE* src, ModuleList* mods, Module* m, enum BasicTypeChoiceId type, NamedType* e, const char* szIndent)
{
	const char* szFieldName = e->fieldName;
	const bool bOptional = e->type->optional ? true : false;
	// const bool bImplicit = e->type->implicit ? true : false;
	char szOptional[128] = {0};
	int iOptionalID = -1;
	if (bOptional)
	{
		iOptionalID = GetContextID(e->type);
		if (iOptionalID >= 0)
			sprintf_s(szOptional, sizeof(szOptional), ", idBlock: { optionalID: %i }", iOptionalID);
	}

	if (type != BASICTYPE_EXTENSION)
	{
		char* szAccessName = FixName(szFieldName);

		if (type == BASICTYPE_IMPORTTYPEREF || type == BASICTYPE_LOCALTYPEREF)
			fprintf(src, "%sif (_%s)\n\t", szIndent, szAccessName);
		else if (bOptional)
			fprintf(src, "%sif (s.%s !== undefined)\n\t", szIndent, szAccessName);
		fprintf(src, "%st.push(", szIndent);

		switch (type)
		{
			case BASICTYPE_UTF8_STR:
			case BASICTYPE_BOOLEAN:
			case BASICTYPE_INTEGER:
			case BASICTYPE_REAL:
			case BASICTYPE_ENUMERATED:
				fprintf(src, "new asn1ts.%s({ value: s.%s, name: \"%s\"%s })", GetBERType(type), szAccessName, szAccessName, szOptional);
				break;
			case BASICTYPE_NULL:
				fprintf(src, "new asn1ts.%s({ name: \"%s\"%s })", GetBERType(type), szAccessName, szOptional);
				break;
			case BASICTYPE_ANY:
				fprintf(src, "s.%s as asn1ts.Sequence", szAccessName);
				break;
			case BASICTYPE_OCTETCONTAINING:
				if (e->type->basicType->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
					fprintf(src, "new asn1ts.%s({ valueHex: new TextEncoder().encode(s.%s), name: \"%s\"%s })", GetBERType(type), szAccessName, szAccessName, szOptional);
				else
					fprintf(src, "new asn1ts.%s({ valueHex: s.%s, name: \"%s\"%s })", GetBERType(type), szAccessName, szAccessName, szOptional);
				break;
			case BASICTYPE_OCTETSTRING:
				fprintf(src, "new asn1ts.%s({ valueHex: s.%s, name: \"%s\"%s })", GetBERType(type), szAccessName, szAccessName, szOptional);
				break;
			case BASICTYPE_ASNSYSTEMTIME:
				fprintf(src, "new asn1ts.Real({ value: TSConverter.getVariantTimeFromDateTime(s.%s), name: \"%s\"%s })", szAccessName, szAccessName, szOptional);
				break;
			case BASICTYPE_LOCALTYPEREF:
			case BASICTYPE_IMPORTTYPEREF:
				{
					fprintf(src, "_%s", szFieldName);
					break;
				}
			default:
				snacc_exit("unknown type %d", type);
		}

		fprintf(src, ");\n");

		free(szAccessName);
	}
}

void Print_JSON_EncoderSeqDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seq, int novolatilefuncs)
{
	fprintf(src, "\t\t// [%s]\n", __FUNCTION__);

	NamedType* e;
	char* szConverted = FixName(td->definedName);
	fprintf(src, "\t\tif (newContext.bAddTypes)\n");
	fprintf(src, "\t\t\tt._type = \"%s\";\n", szConverted);

	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		if (IsDeprecatedNoOutputMember(m, td, e->fieldName))
			continue;

		BasicType* pBaseType = e->type->basicType;
		enum BasicTypeChoiceId choiceId = pBaseType->choiceId;
		if (choiceId == BASICTYPE_LOCALTYPEREF || choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			if (strcmp(e->type->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
				choiceId = BASICTYPE_ASNSYSTEMTIME;
			else
			{
				BasicType* pBaseType2 = GetBaseBasicType(e->type->basicType, NULL);
				if (IsSimpleType(pBaseType2->choiceId))
				{
					pBaseType = pBaseType2;
					choiceId = pBaseType2->choiceId;
				}
			}
		}
		if (choiceId == BASICTYPE_OCTETCONTAINING && pBaseType->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
			choiceId = BASICTYPE_UTF8_STR;

		Print_JSON_EncoderNamedType(src, mods, m, choiceId, e, "\t\t");
	}
	free(szConverted);
}

enum BasicTypeChoiceId TSResolveImportedType(NamedType* obj)
{
	enum BasicTypeChoiceId type = obj->type->basicType->choiceId;
	if (type == BASICTYPE_LOCALTYPEREF || type == BASICTYPE_IMPORTTYPEREF)
	{
		if (strcmp(obj->type->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
			type = BASICTYPE_ASNSYSTEMTIME;
		else
		{
			enum BasicTypeChoiceId baseType = GetBaseBasicTypeChoiceId(obj->type->basicType);
			if (IsSimpleType(baseType))
				type = baseType;
		}
	}
	return type;
}

void Print_BER_EncoderSeqDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seq, int novolatilefuncs)
{
	fprintf(src, "\n\t\t// [%s]\n", __FUNCTION__);

	NamedType* e;
	char* szConverted = FixName(td->definedName);
	int iLines = 0;

	// Validate the mandatory (non optional) objects on block (we only set them once the full set is valid)
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		if (IsDeprecatedNoOutputMember(m, td, e->fieldName))
			continue;

		enum BasicTypeChoiceId type = TSResolveImportedType(e);
		if (type == BASICTYPE_EXTENSION)
			continue;
		Print_BER_EncoderValidateProperty(src, mods, m, type, e, "\t\t");
		iLines++;
		if (e->type->optional)
			iLines++;
	}

	if (iLines)
	{
		// If the validation succeeded we assign them to the asn1 object
		fprintf(src, "\t\tif (errors.validateResult(newContext, \"%s\")) {\n", td->definedName);
		FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
		{
			if (IsDeprecatedNoOutputMember(m, td, e->fieldName))
				continue;

			enum BasicTypeChoiceId type = TSResolveImportedType(e);
			Print_BER_EncoderAssignProperty(src, mods, m, type, e, "\t\t\t");
		}
		fprintf(src, "\t\t\treturn result;\n");
		fprintf(src, "\t\t}\n");

		free(szConverted);
	}
	else
		fprintf(src, "\n\t\treturn result;\n");
}

void Print_JSON_DecoderNamedType(FILE* src, Module* m, ModuleList* mods, NamedType* type, BasicType* pBasicType, const char* szIndent)
{
	enum BasicTypeChoiceId choiceID = pBasicType->choiceId;

	if (choiceID == BASICTYPE_LOCALTYPEREF || choiceID == BASICTYPE_IMPORTTYPEREF)
	{
		if (strcmp(type->type->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
			choiceID = BASICTYPE_ASNSYSTEMTIME;
		else
		{
			BasicType* pBasicType2 = GetBaseBasicType(pBasicType, NULL);
			if (IsSimpleType(pBasicType2->choiceId))
			{
				pBasicType = pBasicType2;
				choiceID = pBasicType->choiceId;
			}
		}
	}

	const char* szFieldName = type->fieldName;
	const char* szClassName = type->type->cxxTypeRefInfo->className;
	bool bOptional = type->type->optional ? true : false;
	const char* szOptional = bOptional ? "true" : "false";
	const char* szOptional2 = bOptional ? " && s." : "";
	const char* szOptional3 = bOptional ? type->fieldName : "";

	char* szAccessName = FixName(szFieldName);

	if (choiceID == BASICTYPE_OCTETCONTAINING && pBasicType->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
		choiceID = BASICTYPE_UTF8_STR;

	switch (choiceID)
	{
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_INTEGER:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_REAL:
		case BASICTYPE_UTF8_STR:
			fprintf(src, "%sTSConverter.fillJSONParam(s, t, \"%s\", \"%s\", errors, context, %s);\n", szIndent, szAccessName, GetJSONType(choiceID), szOptional);
			break;
		case BASICTYPE_ASNSYSTEMTIME:
			fprintf(src, "%sif (TSConverter.validateParam(s, \"%s\", \"string\", errors, context, %s)%s%s)\n", szIndent, szAccessName, szOptional, szOptional2, szOptional3);
			fprintf(src, "%s\tt.%s = new Date(s.%s);\n", szIndent, szAccessName, szAccessName);
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(src, "%sif (TSConverter.validateParam(s, \"%s\", \"string\", errors, context, %s)%s%s)\n", szIndent, szAccessName, szOptional, szOptional2, szOptional3);
			fprintf(src, "%s\tt.%s = TSConverter.decode64(s.%s as unknown as string);\n", szIndent, szAccessName, szAccessName);
			break;
		case BASICTYPE_LOCALTYPEREF:
		case BASICTYPE_IMPORTTYPEREF:
			{
				char* pBuffer = _strdup(szAccessName);
				_strlwr_s(pBuffer, strlen(szAccessName) + 1);
				if (bOptional)
					fprintf(src, "%st.%s = ", szIndent, szAccessName);
				else
					fprintf(src, "%sconst _%s = ", szIndent, pBuffer);

				if (choiceID == BASICTYPE_IMPORTTYPEREF)
				{
					Module* mod = GetImportModuleRefByClassName(szClassName, mods, m);
					fprintf(src, "%s_Converter.", GetNameSpace(mod));
				}

				fprintf(src, "%s_Converter.fromJSON(s.%s, errors, newContext, \"%s\", %s);\n", szClassName, szAccessName, szAccessName, szOptional);
				if (!bOptional)
				{
					fprintf(src, "%sif (_%s)\n", szIndent, pBuffer);
					fprintf(src, "%s\tt.%s = _%s;\n", szIndent, szAccessName, pBuffer);
				}
				free(pBuffer);
			}
			break;
		case BASICTYPE_ANY:
			fprintf(src, "%sif (TSConverter.validateAnyParam(s, \"%s\", errors, newContext, %s))\n", szIndent, szAccessName, szOptional);
			fprintf(src, "%s\t// eslint-disable-next-line @typescript-eslint/no-unsafe-assignment\n", szIndent);
			fprintf(src, "%s\tt.%s = s.%s;\n", szIndent, szAccessName, szAccessName);
			break;
		case BASICTYPE_EXTENSION:
		case BASICTYPE_NULL:
			// Do Nothing here
			break;
		default:
			snacc_exit("unknown choice %d", choiceID);
	}

	free(szAccessName);
}

void Print_BER_DecoderNamedType(FILE* src, Module* m, ModuleList* mods, NamedType* type, enum BasicTypeChoiceId choice, const char* szIndent)
{
	const char* szFieldName = type->fieldName;
	const char* szClassName = type->type->cxxTypeRefInfo->className;
	bool bOptional = type->type->optional ? true : false;
	const char* szOptional = bOptional ? ", true" : "";

	char* szAccessName = FixName(szFieldName);

	switch (choice)
	{
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_INTEGER:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_REAL:
		case BASICTYPE_UTF8_STR:
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_ASNSYSTEMTIME:
		case BASICTYPE_ANY:
		case BASICTYPE_NULL:
			fprintf(src, "%sTSConverter.fillASN1Param(s, t, \"%s\", \"%s\", errors, newContext%s);\n", szIndent, szAccessName, GetBERType(choice), szOptional);
			break;
		case BASICTYPE_LOCALTYPEREF:
		case BASICTYPE_IMPORTTYPEREF:
			{
				char* pBuffer = _strdup(szAccessName);
				_strlwr_s(pBuffer, strlen(szAccessName) + 1);
				fprintf(src, "%s", szIndent);
				if (bOptional)
					fprintf(src, "t.%s = ", szAccessName);
				else
					fprintf(src, "const _%s = ", pBuffer);

				if (choice == BASICTYPE_IMPORTTYPEREF)
				{
					Module* mod = GetImportModuleRefByClassName(szClassName, mods, m);
					fprintf(src, "%s_Converter.", GetNameSpace(mod));
				}
				enum BasicTypeChoiceId rootBasicType = GetBaseBasicTypeChoiceId(type->type->basicType);
				if (rootBasicType == BASICTYPE_SEQUENCEOF || rootBasicType == BASICTYPE_SETOF)
					rootBasicType = BASICTYPE_SEQUENCE;
				if (rootBasicType == BASICTYPE_CHOICE)
					fprintf(src, "%s_Converter.fromBER(s.getValueByName(\"%s\"), errors, newContext, \"%s\"%s);\n", szClassName, szAccessName, szAccessName, szOptional);
				else
					fprintf(src, "%s_Converter.fromBER(s.getTypedValueByName(asn1ts.%s, \"%s\"), errors, newContext, \"%s\"%s);\n", szClassName, GetBERType(rootBasicType), szAccessName, szAccessName, szOptional);
				if (!bOptional)
				{
					fprintf(src, "%sif (_%s)\n", szIndent, pBuffer);
					fprintf(src, "%s\tt.%s = _%s;\n", szIndent, szAccessName, pBuffer);
				}
				free(pBuffer);
			}
		case BASICTYPE_EXTENSION:
			// Do Nothing here
			break;
		default:
			snacc_exit("unknown choice %d", choice);
	}

	free(szAccessName);
}

void Print_JSON_DecoderSeqDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seq, int novolatilefuncs, const char* szIndent)
{
	fprintf(src, "\n%s// [%s]\n", szIndent, __FUNCTION__);

	char* szConverted = FixName(td->definedName);
	NamedType* e;
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		if (IsDeprecatedNoOutputMember(m, td, e->fieldName))
			continue;

		Print_JSON_DecoderNamedType(src, m, mods, e, e->type->basicType, szIndent);
	}
	free(szConverted);
}

void Print_BER_DecoderSeqDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seq, int novolatilefuncs, const char* szIndent)
{
	fprintf(src, "\n%s// [%s]\n", szIndent, __FUNCTION__);
	char* szConverted = FixName(td->definedName);

	NamedType* e;

	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		if (IsDeprecatedNoOutputMember(m, td, e->fieldName))
			continue;

		enum BasicTypeChoiceId type = e->type->basicType->choiceId;

		if (type == BASICTYPE_LOCALTYPEREF || type == BASICTYPE_IMPORTTYPEREF)
		{
			if (strcmp(e->type->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
				type = BASICTYPE_ASNSYSTEMTIME;
			else
			{
				enum BasicTypeChoiceId baseType = GetBaseBasicTypeChoiceId(e->type->basicType);
				if (IsSimpleType(baseType))
					type = baseType;
			}
		}

		Print_BER_DecoderNamedType(src, m, mods, e, type, szIndent);
	}
	free(szConverted);
}

void Print_JSON_DecoderImportTypeRef(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seq, int novolatilefuncs, const char* szIndent)
{
	fprintf(src, "\n%s// [%s]\n", szIndent, __FUNCTION__);
	const char* szClassName = td->type->basicType->a.importTypeRef->link->definedName;
	Module* mod = GetImportModuleRefByClassName(szClassName, mods, m);
	const char* szNameSpace = GetNameSpace(mod);
	fprintf(src, "%st = %s_Converter.%s_Converter.fromJSON(s, errors, newContext, undefined, optional);\n", szIndent, szNameSpace, szClassName);
}

void Print_JSON_EncoderImportTypeRef(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seq, int novolatilefuncs)
{
	fprintf(src, "\t\t// [%s]\n", __FUNCTION__);
	Module* mod = GetImportModuleRefByClassName(td->type->basicType->a.importTypeRef->link->definedName, mods, m);
	const char* szNameSpace = GetNameSpace(mod);
	fprintf(src, "\t\tconst t = %s_Converter.%s_Converter.toJSON(s, errors, newContext, name);\n", szNameSpace, td->type->basicType->a.importTypeRef->link->definedName);
}

void Print_BER_DecoderImportTypeRef(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seq, int novolatilefuncs, const char* szIndent)
{
	fprintf(src, "%s// [%s]\n", szIndent, __FUNCTION__);
	const char* szClassName = td->type->basicType->a.importTypeRef->link->definedName;
	Module* mod = GetImportModuleRefByClassName(szClassName, mods, m);
	const char* szNameSpace = GetNameSpace(mod);
	fprintf(src, "%sconst t = %s_Converter.%s_Converter.fromBER(s, errors, newContext, name, optional);\n", szIndent, szNameSpace, szClassName);
}

void Print_BER_EncoderImportTypeRef(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seq, int novolatilefuncs)
{
	fprintf(src, "\n\t\t// [%s]\n", __FUNCTION__);
	const char* szElementName = td->type->basicType->a.importTypeRef->link->definedName;
	Module* mod = GetImportModuleRefByClassName(szElementName, mods, m);
	const char* szNameSpace = GetNameSpace(mod);

	fprintf(src, "\t\tconst v = %s_Converter.%s_Converter.toBER(s, errors, newContext, name, optional);\n", szNameSpace, szElementName);
	fprintf(src, "\t\tif (errors.validateResult(newContext, \"%s\")) {\n", szElementName);
	fprintf(src, "\t\t\tif (v)\n");
	fprintf(src, "\t\t\t\tt.push(v);\n");
	fprintf(src, "\t\t\treturn result;\n");
	fprintf(src, "\t\t}\n");
}

void Print_JSON_DecoderLocalTypeRef(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seq, int novolatilefuncs, const char* szIndent)
{
	fprintf(src, "\n%s// [%s]\n", szIndent, __FUNCTION__);
	fprintf(src, "%st = %s_Converter.fromJSON(s, errors, newContext, name, optional);\n", szIndent, td->type->basicType->a.localTypeRef->link->definedName);
}

void Print_JSON_EncoderLocalTypeRef(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seq, int novolatilefuncs)
{
	fprintf(src, "\t\t// [%s]\n", __FUNCTION__);
	fprintf(src, "\t\tconst t = %s_Converter.toJSON(s, errors, newContext, name);\n", td->type->basicType->a.importTypeRef->link->definedName);
}

void Print_BER_DecoderLocalTypeRef(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seq, int novolatilefuncs, const char* szIndent)
{
	fprintf(src, "\n%s// [%s]\n", szIndent, __FUNCTION__);
	fprintf(src, "%st = %s_Converter.fromBER(s, errors, newContext, name, optional);\n", szIndent, td->type->basicType->a.localTypeRef->link->definedName);
}

void Print_BER_EncoderLocalTypeRef(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seq, int novolatilefuncs)
{
	fprintf(src, "\n\t\t// [%s]\n", __FUNCTION__);

	const char* szElementName = td->type->basicType->a.importTypeRef->link->definedName;

	fprintf(src, "\t\tconst v = %s_Converter.toBER(s, errors, newContext, name, optional);\n", szElementName);
	fprintf(src, "\t\tif (errors.validateResult(newContext, \"%s\")) {\n", szElementName);
	fprintf(src, "\t\t\tif (v)\n");
	fprintf(src, "\t\t\t\tt.push(v);\n");
	fprintf(src, "\t\t\treturn result;\n");
	fprintf(src, "\t\t}\n");
}

void Print_JSON_EncoderCodeStructuredType(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs)
{
	enum BasicTypeChoiceId choice = td->type->basicType->choiceId;
	switch (choice)
	{
		case BASICTYPE_SEQUENCEOF: /* list types */
		case BASICTYPE_SETOF:
			Print_JSON_EncoderSetOfDefCode(src, mods, m, td, td->type, novolatilefuncs);
			break;
		case BASICTYPE_CHOICE:
			Print_JSON_EncoderChoiceDefCode(src, mods, m, td, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SEQUENCE:
			Print_JSON_EncoderSeqDefCode(src, mods, m, td, td->type, novolatilefuncs);
			break;
		case BASICTYPE_IMPORTTYPEREF:
			Print_JSON_EncoderImportTypeRef(src, mods, m, td, td->type, novolatilefuncs);
			break;
		case BASICTYPE_LOCALTYPEREF:
			Print_JSON_EncoderLocalTypeRef(src, mods, m, td, td->type, novolatilefuncs);
			break;
		default:
			snacc_exit("unknown choice %d", choice);
	}
}

void Print_BER_EncoderCodeStructuredType(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs)
{
	enum BasicTypeChoiceId choice = td->type->basicType->choiceId;
	switch (choice)
	{
		case BASICTYPE_SEQUENCEOF: /* list types */
		case BASICTYPE_SETOF:
			Print_BER_EncoderSetOfDefCode(src, mods, m, td, td->type, novolatilefuncs);
			break;
		case BASICTYPE_CHOICE:
			Print_BER_EncoderChoiceDefCode(src, mods, m, td, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SEQUENCE:
			Print_BER_EncoderSeqDefCode(src, mods, m, td, td->type, novolatilefuncs);
			break;
		case BASICTYPE_IMPORTTYPEREF:
			Print_BER_EncoderImportTypeRef(src, mods, m, td, td->type, novolatilefuncs);
			break;
		case BASICTYPE_LOCALTYPEREF:
			Print_BER_EncoderLocalTypeRef(src, mods, m, td, td->type, novolatilefuncs);
			break;
		default:
			snacc_exit("unknown choice %d", choice);
	}
}

// ********************************************************************************************************************************************
// ********************************************************************************************************************************************
// ********************************************************************************************************************************************
// ********************************************************************************************************************************************

void Print_JSON_DecoderCodeStructuredType(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs, const char* szIdendt)
{
	enum BasicTypeChoiceId choice = td->type->basicType->choiceId;
	switch (choice)
	{
		case BASICTYPE_SEQUENCEOF: /* list types */
		case BASICTYPE_SETOF:
			Print_JSON_DecoderSetOfDefCode(src, mods, m, td, td->type, novolatilefuncs, szIdendt);
			break;
		case BASICTYPE_CHOICE:
			Print_JSON_DecoderChoiceDefCode(src, mods, m, td, td->type, novolatilefuncs, szIdendt);
			break;
		case BASICTYPE_SEQUENCE:
			Print_JSON_DecoderSeqDefCode(src, mods, m, td, td->type, novolatilefuncs, szIdendt);
			break;
		case BASICTYPE_IMPORTTYPEREF:
			Print_JSON_DecoderImportTypeRef(src, mods, m, td, td->type, novolatilefuncs, szIdendt);
			break;
		case BASICTYPE_LOCALTYPEREF:
			Print_JSON_DecoderLocalTypeRef(src, mods, m, td, td->type, novolatilefuncs, szIdendt);
			break;
		case BASICTYPE_ENUMERATED:
			// PrintTSDecoderNamedType(src, td->type->typeName, BASICTYPE_ENUMERATED);
			break;
		default:
			snacc_exit("unknown choice %d", choice);
	}
}

void Print_BER_DecoderCodeStructuredType(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs, const char* szIdendt)
{
	enum BasicTypeChoiceId choice = td->type->basicType->choiceId;
	switch (choice)
	{
		case BASICTYPE_SEQUENCEOF: /* list types */
		case BASICTYPE_SETOF:
			Print_BER_DecoderSetOfDefCode(src, mods, m, td, td->type, novolatilefuncs, szIdendt);
			break;
		case BASICTYPE_CHOICE:
			Print_BER_DecoderChoiceDefCode(src, mods, m, td, td->type, novolatilefuncs, szIdendt);
			break;
		case BASICTYPE_SEQUENCE:
			Print_BER_DecoderSeqDefCode(src, mods, m, td, td->type, novolatilefuncs, szIdendt);
			break;
		case BASICTYPE_IMPORTTYPEREF:
			Print_BER_DecoderImportTypeRef(src, mods, m, td, td->type, novolatilefuncs, szIdendt);
			break;
		case BASICTYPE_LOCALTYPEREF:
			Print_BER_DecoderLocalTypeRef(src, mods, m, td, td->type, novolatilefuncs, szIdendt);
			break;
		case BASICTYPE_ENUMERATED:
			// PrintTSDecoderNamedType(src, td->type->typeName, BASICTYPE_ENUMERATED);
			break;
		default:
			snacc_exit("unknown choice %d", choice);
	}
}

// ********************************************************************************************************************************************
// ********************************************************************************************************************************************
// ********************************************************************************************************************************************
// ********************************************************************************************************************************************

void PrintTSEncoderDecoderCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs, int printEncoders, int printDecoders)
{
	char* szConverted = FixName(td->definedName);

	const char* szNameSpace = GetNameSpace(m);
	// Simple Typen, also Typen die auf oberster Ebene nur einen anderen Namen bekomme haben
	// Bspw: AsnSystemTime ::= REAL
	// brauchen keinen Encoder Decoder

	enum BasicTypeChoiceId type = td->type->basicType->choiceId;

	if (!IsSimpleType(type))
	{
		fprintf(src, "\n// [%s]\n", __FUNCTION__);
		fprintf(src, "export class %s_Converter {", szConverted);
		if (printEncoders)
		{
			fprintf(src, "\n\tpublic static toJSON(s: %s.%s, errors?: ConverterErrors, context?: IEncodeContext, name?: string): %s.%s", szNameSpace, szConverted, szNameSpace, szConverted);
			if (type != BASICTYPE_SEQUENCEOF && type != BASICTYPE_SETOF)
			{
				// Sequence of only contains the elements, no additional stuff
				fprintf(src, " & INamedType");
			}
			fprintf(src, " | undefined {\n");

			// An array cannot create any erros while converting
			fprintf(src, "\t\terrors ||= new ConverterErrors();\n");
			fprintf(src, "\t\terrors.storeState();\n");
			fprintf(src, "\t\tconst newContext = TSConverter.addEncodeContext(context, name, \"%s\");\n\n", szConverted);

			if (type == BASICTYPE_SEQUENCEOF || type == BASICTYPE_SETOF)
				fprintf(src, "\t\tconst t = [] as %s.%s;\n\n", szNameSpace, szConverted);
			else if (type != BASICTYPE_IMPORTTYPEREF && type != BASICTYPE_LOCALTYPEREF)
				fprintf(src, "\t\tconst t = {} as %s.%s & INamedType;\n\n", szNameSpace, szConverted);

			if (strcmp(szConverted, "AsnOptionalParam") == 0)
			{
				fprintf(src, "\t\t// It is not possible to encode a single AsnOptionalParam into the ucserver notation. Needs the AsnOptionalParameters envelop!\n");
				fprintf(src, "\t\tif (newContext?.bUCServerOptionalParams)\n");
				fprintf(src, "\t\t\tdebugger;\n\n");
			}
			else if (strcmp(szConverted, "AsnOptionalParameters") == 0)
			{
				fprintf(src, "\t\tif (newContext?.bUCServerOptionalParams) {\n");
				fprintf(src, "\t\t\tconst params = EAsnOptionalParametersConverter.toJSON(s, errors, newContext, name);\n");
				fprintf(src, "\t\t\tif (errors.validateResult(newContext, \"EAsnOptionalParameters\"))\n");
				fprintf(src, "\t\t\t\treturn params;\n");
				fprintf(src, "\t\t\treturn undefined;\n");
				fprintf(src, "\t\t}\n");
			}

			Print_JSON_EncoderCodeStructuredType(src, mods, m, td, novolatilefuncs);

			fprintf(src, "\n\t\tif (errors.validateResult(newContext, \"%s\"))\n", td->definedName);
			fprintf(src, "\t\t\treturn t;\n\n");
			fprintf(src, "\t\treturn undefined;\n\n");
			fprintf(src, "\t}\n");
		}
		if (printDecoders)
		{
			fprintf(src, "\n\tpublic static fromJSON(data: string | object | undefined, errors?: ConverterErrors, context?: IDecodeContext, name?: string, optional?: boolean): %s.%s | undefined {\n", szNameSpace, szConverted);
			fprintf(src, "\t\terrors ||= new ConverterErrors();\n");
			fprintf(src, "\t\terrors.storeState();\n");
			fprintf(src, "\t\tconst newContext = TSConverter.addDecodeContext(context, name, \"%s\");\n\n", szConverted);
			fprintf(src, "\t\tlet t: %s.%s | undefined;\n", szNameSpace, szConverted);

			fprintf(src, "\t\tconst s = TSConverter.prepareJSONData<%s.%s>(data, errors, newContext, optional);\n", szNameSpace, szConverted);
			fprintf(src, "\t\tif (s) {");
			enum BasicTypeChoiceId choice = td->type->basicType->choiceId;
			if (choice != BASICTYPE_LOCALTYPEREF && choice != BASICTYPE_IMPORTTYPEREF)
			{
				fprintf(src, "\n");
				if (type == BASICTYPE_SEQUENCEOF || type == BASICTYPE_SETOF)
					fprintf(src, "\t\t\tt = new %s.%s();", szNameSpace, szConverted);
				else
					fprintf(src, "\t\t\tt = %s.%s[\"initEmpty\"].call(0);", szNameSpace, szConverted);
			}

			const char* szIdendt = "\t\t\t";

			if (strcmp(szConverted, "AsnOptionalParam") == 0)
			{
				fprintf(src, "\n");
				fprintf(src, "\t\t\t// It is not possible to decode a single AsnOptionalParam from the ucserver notation. Needs the AsnOptionalParameters envelop!\n");
				fprintf(src, "\t\t\tif (s.key === undefined)\n");
				fprintf(src, "\t\t\t\tdebugger;\n");
				fprintf(src, "\t\t\telse {");
				szIdendt = "\t\t\t\t";
			}
			else if (strcmp(szConverted, "AsnOptionalParameters") == 0)
			{
				fprintf(src, "\n");
				fprintf(src, "\t\t\tif (s.length === undefined) {\n");
				fprintf(src, "\t\t\t\t// Proprietary UCServer AsnOptionalParameters decoding\n");
				fprintf(src, "\t\t\t\tEAsnOptionalParametersConverter.fromJSON(s, t, errors, context, name, optional);\n");
				fprintf(src, "\t\t\t} else {");
				szIdendt = "\t\t\t\t";
			}
			Print_JSON_DecoderCodeStructuredType(src, mods, m, td, novolatilefuncs, szIdendt);

			if (strcmp(szIdendt, "\t\t\t\t") == 0)
				fprintf(src, "\t\t\t}\n");
			fprintf(src, "\t\t}\n");

			fprintf(src, "\t\tif (errors.validateResult(newContext, \"%s\"))\n", td->definedName);
			fprintf(src, "\t\t\treturn t;\n\n");

			fprintf(src, "\t\treturn undefined;\n");
			fprintf(src, "\t}\n");
		}
		if (printEncoders)
		{
			fprintf(src, "\n\tpublic static toBER(s: %s.%s | undefined, errors?: ConverterErrors, context?: IEncodeContext, name?: string, optional?: boolean | number): ", szNameSpace, szConverted);
			if (td->type->basicType->choiceId == BASICTYPE_CHOICE)
				fprintf(src, "asn1ts.BaseBlock");
			else
				fprintf(src, "asn1ts.Sequence");
			fprintf(src, " | undefined {\n");
			fprintf(src, "\t\tname ||= \"%s\";\n", szConverted);
			fprintf(src, "\t\tif (!s) {\n");
			fprintf(src, "\t\t\tTSConverter.addMissingError(errors, context, name, optional);\n");
			fprintf(src, "\t\t\treturn undefined;\n");
			fprintf(src, "\t\t}\n\n");

			if (td->type->basicType->choiceId != BASICTYPE_CHOICE)
			{
				fprintf(src, "\t\tconst result = new asn1ts.Sequence(TSConverter.getASN1TSConstructorParams(name, optional));\n");
				fprintf(src, "\t\tconst t = result.valueBlock.value;\n");
			}
			else
				fprintf(src, "\t\tlet t: asn1ts.BaseBlock | undefined;\n");

			fprintf(src, "\t\terrors ||= new ConverterErrors();\n");
			fprintf(src, "\t\terrors.storeState();\n");
			fprintf(src, "\t\tconst newContext = TSConverter.addEncodeContext(context, name, \"%s\");\n", szConverted);

			Print_BER_EncoderCodeStructuredType(src, mods, m, td, novolatilefuncs);

			fprintf(src, "\t\treturn undefined;\n");
			fprintf(src, "\t}\n");
		}
		if (printDecoders)
		{
			fprintf(src, "\n\tpublic static fromBER(data: Uint8Array | asn1ts.BaseBlock | undefined, errors?: ConverterErrors, context?: IDecodeContext, name?: string, optional?: boolean): %s.%s | undefined {\n", szNameSpace, szConverted);
			fprintf(src, "\t\terrors ||= new ConverterErrors();\n");
			fprintf(src, "\t\terrors.storeState();\n");
			fprintf(src, "\t\tconst newContext = TSConverter.addDecodeContext(context, name, \"%s\");\n\n", szConverted);

			fprintf(src, "\t\tlet t: %s.%s | undefined;\n", szNameSpace, szConverted);

			fprintf(src, "\t\tconst s = TSConverter.prepareASN1BERData(%s.%s.getASN1Schema, data, errors, newContext, optional);\n", szNameSpace, szConverted);
			if (td->type->basicType->choiceId != BASICTYPE_CHOICE)
				fprintf(src, "\t\tif (asn1ts.Sequence.typeGuard(s)) {");
			else
				fprintf(src, "\t\tif (s) {");

			enum BasicTypeChoiceId choice = td->type->basicType->choiceId;
			if (choice != BASICTYPE_IMPORTTYPEREF && choice != BASICTYPE_LOCALTYPEREF)
			{
				fprintf(src, "\n");
				if (type == BASICTYPE_SEQUENCEOF || type == BASICTYPE_SETOF)
					fprintf(src, "\t\t\tt = new %s.%s();", szNameSpace, szConverted);
				else
					fprintf(src, "\t\t\tt = %s.%s[\"initEmpty\"].call(0);", szNameSpace, szConverted);
			}

			Print_BER_DecoderCodeStructuredType(src, mods, m, td, novolatilefuncs, "\t\t\t");

			fprintf(src, "\t\t}\n\n");

			fprintf(src, "\t\tif (errors.validateResult(newContext, \"%s\"))\n", td->definedName);
			fprintf(src, "\t\t\treturn t;\n\n");

			fprintf(src, "\t\treturn undefined;\n");
			fprintf(src, "\t}\n");
		}
		fprintf(src, "}\n");
	}
	free(szConverted);
}

void PrintTSConverterCode(FILE* src, ModuleList* mods, Module* m, long longJmpVal, int printTypes, int printValues, int printEncoders, int printDecoders, int printTSONEncDec, int novolatilefuncs)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);

	// Comments
	PrintTSConverterComments(src, m);

	// Includes
	PrintTSConverterImports(src, mods, m);

	// Root types
	PrintTSRootTypes(src, m, "_Converter");

	TypeDef* td;
	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		if (IsDeprecatedNoOutputSequence(m, td->definedName))
			continue;
		PrintTSEncoderDecoderCode(src, mods, m, td, novolatilefuncs, printEncoders, printDecoders);
	}
} /* PrintTSConverterCode */