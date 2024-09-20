/*
 *   compiler/back_ends/TS_gen/gen_js_code.c - routines for printing C++ code from type trees
 *
 *   assumes that the type tree has already been run through the
 *   c++ type generator (c++_gen/types.c).
 *
 *  This was hastily written - it has some huge routines in it.
 *  Needs a lot of cleaning up and modularization...
 *
 * Mike Sample
 * 92
 * Copyright (C) 1991, 1992 Michael Sample
 *           and the University of British Columbia
 *
 * 2022 ESTOS/JF
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include "gen-ts-code.h"
#include "../str-util.h"
#include "../structure-util.h"
#include "../comment-util.h"
#include "gen-ts-combined.h"
#include "gen-ts-converter.h"
#include "gen-ts-rose.h"
#include "../../core/asn_comments.h"
#include "../../core/time_helpers.h"
#include <inttypes.h>
#include <assert.h>

int sortstring(const void* str1, const void* str2)
{
	const char* rec1 = *(char**)str1;
	const char* rec2 = *(char**)str2;
	return strcmp(rec1, rec2);
}

void PrintTSNativeType(FILE* hdr, BasicType* pBasicType)
{
	enum BasicTypeChoiceId choiceId = pBasicType->choiceId;

	if (choiceId == BASICTYPE_OCTETCONTAINING && pBasicType->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
		choiceId = BASICTYPE_UTF8_STR;

	switch (choiceId)
	{
		case BASICTYPE_BOOLEAN:
			fprintf(hdr, "boolean");
			break;
		case BASICTYPE_INTEGER:
		case BASICTYPE_REAL:
			fprintf(hdr, "number");
			break;
		case BASICTYPE_ENUMERATED:
			fprintf(hdr, "number");
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(hdr, "Uint8Array");
			break;
		case BASICTYPE_UTF8_STR:
			fprintf(hdr, "string");
			break;
		case BASICTYPE_NULL:
			fprintf(hdr, "object");
			break;
		default:
			snacc_exit("Invalid basicTypeChoiseId %d", choiceId);
	}
}

void PrintTS_JSON_DefaultValue(FILE* hdr, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* t)
{
	// fprintf(hdr, "/*[PrintTSDefaultValue]*/");
	if (t->optional)
	{
		fprintf(hdr, "undefined");
		return;
	}

	if (t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED)
	{
		CNamedElmt* n;
		Type* enumType = t->basicType->a.localTypeRef->link->type;
		if (HasNamedElmts(enumType) != 0)
		{
			FOR_EACH_LIST_ELMT_NOITERATE(n, enumType->cxxTypeRefInfo->namedElmts)
			{
				// fprintf(hdr, ", default: %s.%s", t->cxxTypeRefInfo->className, n->name);
				fprintf(hdr, "%s.%s", t->cxxTypeRefInfo->className, n->name);
				break;
			}
		}
	}
	else
	{
		Type* baset = t;
		Module* mod = NULL;
		enum BasicTypeChoiceId choiceId = t->basicType->choiceId;
		if (choiceId == BASICTYPE_IMPORTTYPEREF || choiceId == BASICTYPE_LOCALTYPEREF)
		{
			if (strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
				choiceId = BASICTYPE_UTCTIME;
			else if (t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
			{
				if (IsSimpleType(t->basicType->a.importTypeRef->link->type->basicType->choiceId))
				{
					mod = GetImportModuleRefByClassName(t->cxxTypeRefInfo->className, mods, m);
					t = t->basicType->a.importTypeRef->link->type;
					choiceId = t->basicType->choiceId;
				}
			}
			else if (t->basicType->choiceId == BASICTYPE_LOCALTYPEREF)
			{
				if (IsSimpleType(t->basicType->a.localTypeRef->link->type->basicType->choiceId))
				{
					t = t->basicType->a.importTypeRef->link->type;
					choiceId = t->basicType->choiceId;
				}
			}
		}

		if (choiceId == BASICTYPE_OCTETCONTAINING && t->basicType->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
			choiceId = BASICTYPE_UTF8_STR;

		switch (choiceId)
		{
			case BASICTYPE_BOOLEAN:
				fprintf(hdr, "false");
				break;
			case BASICTYPE_INTEGER:
				fprintf(hdr, "0");
				break;
			case BASICTYPE_ENUMERATED:
				if (mod)
					fprintf(hdr, "%s.", mod->moduleName);
				ValueDef* e = FIRST_LIST_ELMT(t->basicType->a.enumerated);
				fprintf(hdr, "%s.%s", baset->cxxTypeRefInfo->className, e->definedName);
				break;
			case BASICTYPE_OCTETSTRING:
			case BASICTYPE_OCTETCONTAINING:
				fprintf(hdr, "new Uint8Array(0)");
				break;
			case BASICTYPE_UTCTIME:
				fprintf(hdr, "new Date()");
				break;
			case BASICTYPE_REAL:
				fprintf(hdr, "0.0");
				break;
			case BASICTYPE_UTF8_STR:
				fprintf(hdr, "\"\"");
				break;
			case BASICTYPE_IMPORTTYPEREF:
				{
					mod = GetImportModuleRefByClassName(t->cxxTypeRefInfo->className, mods, m);
					if (mod)
					{
						const char* szNameSpace = GetNameSpace(mod);
						char* szConverted = FixName(t->cxxTypeRefInfo->className);
						enum BasicTypeChoiceId baseType = GetBaseBasicTypeChoiceId(t->basicType);
						if (baseType == BASICTYPE_SEQUENCEOF || baseType == BASICTYPE_SETOF)
							fprintf(hdr, "new %s.%s()", szNameSpace, szConverted);
						else if (baseType == BASICTYPE_ENUMERATED)
						{
							if (!t->basicType->a.importTypeRef->link)
							{
								snacc_exit("Invalid parameter, BASICTYPE_IMPORTTYPEREF but the associated link is NULL");
								return;
							}
							Type* pType = t->basicType->a.importTypeRef->link->type;
							CNamedElmt* pFirstEnumValue = (CNamedElmt*)pType->cxxTypeRefInfo->namedElmts->first->data;
							fprintf(hdr, "%s.%s.%s", szNameSpace, szConverted, pFirstEnumValue->name);
						}
						else
							fprintf(hdr, "%s.%s[\"initEmpty\"].call(0)", szNameSpace, szConverted);
						free(szConverted);
					}
					break;
				}
			case BASICTYPE_LOCALTYPEREF:
				{
					char* szConverted = FixName(t->cxxTypeRefInfo->className);
					enum BasicTypeChoiceId baseType = GetBaseBasicTypeChoiceId(t->basicType);
					if (baseType == BASICTYPE_SEQUENCEOF || baseType == BASICTYPE_SETOF)
						fprintf(hdr, "new %s()", szConverted);
					else
						fprintf(hdr, "%s[\"initEmpty\"].call(0)", szConverted);
					free(szConverted);
				}
				break;
			case BASICTYPE_NULL:
				fprintf(hdr, "null");
				break;
			case BASICTYPE_ANY:
				fprintf(hdr, "undefined");
				break;
			default:
				snacc_exit("Unknown choiceId %d", choiceId);
		}
	}
}

/**
 * Add the declaration Type of a member
 */
void PrintTSType(FILE* hdr, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* t)
{
	Type* baset = t;
	Module* mod = NULL;
	enum BasicTypeChoiceId choiceId = t->basicType->choiceId;
	if (choiceId == BASICTYPE_IMPORTTYPEREF || choiceId == BASICTYPE_LOCALTYPEREF)
	{
		if (strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
			choiceId = BASICTYPE_UTCTIME;
		else if (choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			if (IsSimpleType(t->basicType->a.importTypeRef->link->type->basicType->choiceId))
			{
				mod = GetImportModuleRefByClassName(t->cxxTypeRefInfo->className, mods, m);
				t = t->basicType->a.importTypeRef->link->type;
				choiceId = t->basicType->choiceId;
			}
		}
		else if (choiceId == BASICTYPE_LOCALTYPEREF)
		{
			if (IsSimpleType(t->basicType->a.localTypeRef->link->type->basicType->choiceId))
			{
				t = t->basicType->a.importTypeRef->link->type;
				choiceId = t->basicType->choiceId;
			}
		}
	}
	if (choiceId == BASICTYPE_OCTETCONTAINING && t->basicType->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
		choiceId = BASICTYPE_UTF8_STR;

	fprintf(hdr, ": ");
	switch (choiceId)
	{
		case BASICTYPE_BOOLEAN:
			fprintf(hdr, "boolean");
			break;
		case BASICTYPE_INTEGER:
		case BASICTYPE_REAL:
			fprintf(hdr, "number");
			break;
		case BASICTYPE_ENUMERATED:
			if (mod)
				fprintf(hdr, "%s.", mod->moduleName);
			fprintf(hdr, "%s", baset->cxxTypeRefInfo->className);
			break;
		case BASICTYPE_UTF8_STR:
			fprintf(hdr, "string");
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(hdr, "Uint8Array");
			break;
		case BASICTYPE_SEQUENCEOF:
			fprintf(hdr, "%s[]", t->cxxTypeRefInfo->className);
			break;
		case BASICTYPE_NULL:
			fprintf(hdr, "null");
			break;
		case BASICTYPE_UTCTIME:
			fprintf(hdr, "Date"); // AsnSystemTime is defined as REAL in the Asn1 file, but is transmitted as a string in the TS.
			break;
		case BASICTYPE_IMPORTTYPEREF:
			{
				mod = GetImportModuleRefByClassName(t->cxxTypeRefInfo->className, mods, m);
				if (mod)
				{
					char* szConverted = FixName(t->cxxTypeRefInfo->className);
					fprintf(hdr, "%s.%s", GetNameSpace(mod), szConverted);
					free(szConverted);
				}
				break;
			}
		case BASICTYPE_LOCALTYPEREF:
			{
				char* szConverted = FixName(t->basicType->a.localTypeRef->link->definedName);
				fprintf(hdr, "%s", szConverted);
				free(szConverted);
				break;
			}
		case BASICTYPE_ANY:
			fprintf(hdr, "any");
			break;
		default:
			snacc_exit("Unknown choiceId %d", choiceId);
	}
} /* PrintCxxType */

void PrintTSEnumDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* enumerated, int novolatilefuncs)
{
	//	NamedType *e;
	//	enum BasicTypeChoiceId tmpTypeId;
	CNamedElmt* n;
	fprintf(src, "// [%s]\n", __FUNCTION__);
	{
		char* szConverted = FixName(td->definedName);
		fprintf(src, "export enum %s {\n", szConverted);
		free(szConverted);
	}
	if (HasNamedElmts(td->type) != 0)
	{
		bool bSomeThingAdded = false;
		bool bFirst = true;
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			if (IsDeprecatedNoOutputMember(m, td, n->name))
				continue;
			if (!bFirst)
				fprintf(src, ",\n");
			printMemberComment(src, m, td, n->name, "\t", COMMENTSTYLE_TYPESCRIPT);
			char* szConverted = FixName(n->name);
			fprintf(src, "\t%s = %d", szConverted, n->value);
			bSomeThingAdded = true;
			free(szConverted);
			bFirst = false;
		}
		if (bSomeThingAdded)
			fprintf(src, "\n");
	}

	/* close class definition */
	fprintf(src, "}\n");
} /* PrintTSEnumDefCode */

void PrintTSChoiceDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* choice, int novolatilefuncs)
{
	NamedType* e;
	//	CxxTRI *cxxtri=NULL;
	//	int inTailOptElmts;
	//	enum BasicTypeChoiceId tmpTypeId;
	//	int allOpt;

	// DEFINE PER encode/decode tmp vars.
	NamedType** pSeqElementNamedType = NULL;
	//	int collectionCounter = 0;

	char* szConverted = FixName(td->definedName);
	fprintf(src, "// [%s]\n", __FUNCTION__);

	/* put class spec in hdr file */
	fprintf(src, "export class %s {\n", szConverted);
	// fprintf(src, "\ttype: \"%s\",\n", td->definedName);

	fprintf(src, "\tpublic constructor(that?: %s) {\n", szConverted);
	fprintf(src, "\t\tObject.assign(this, that);\n");
	fprintf(src, "\t}\n\n");
	fprintf(src, "\tprivate static initEmpty(): %s {\n", szConverted);
	fprintf(src, "\t\treturn new %s(", szConverted);
	fprintf(src, ");\n\t}\n\n");

	fprintf(src, "\tpublic static type = \"%s\";\n\n", szConverted);

	fprintf(src, "\tpublic static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Choice {\n");
	fprintf(src, "\t\treturn new asn1ts.Choice({\n");
	fprintf(src, "\t\t\tname: \"%s\",\n", szConverted);
	fprintf(src, "\t\t\t...params,\n");
	fprintf(src, "\t\t\tvalue: [\n");

	FOR_EACH_LIST_ELMT(e, choice->basicType->a.sequence)
	{
		char szOptionalParam[128] = {0};
		int id = GetContextID(e->type);
		if (id >= 0)
			sprintf_s(szOptionalParam, sizeof(szOptionalParam), ", idBlock: { optionalID: %i }", id);

		if (choice->basicType->a.sequence->curr->prev)
			fprintf(src, ",\n");
		const char* szFieldName = e->fieldName;
		const char* szPropertyName = "";
		BasicType* type = ResolveBasicTypeReferences(e->type->basicType, &szPropertyName);
		enum BasicTypeChoiceId choiceId = type->choiceId;

		if (choiceId == BASICTYPE_SEQUENCEOF || choiceId == BASICTYPE_SETOF)
		{
			choiceId = ResolveArrayRootType(type, &szPropertyName)->choiceId;
			if (IsSimpleType(choiceId))
				fprintf(src, "\t\t\t\tnew asn1ts.%s({ name: \"%s\"%s })", GetBERType(choiceId), szFieldName, szOptionalParam);
			else
				fprintf(src, "\t\t\t\t%s.getASN1Schema({ name: \"%s\"%s })", szPropertyName, szFieldName, szOptionalParam);
		}
		else
		{
			if (IsSimpleType(choiceId))
				fprintf(src, "\t\t\t\tnew asn1ts.%s({ name: \"%s\"%s })", GetBERType(choiceId), szFieldName, szOptionalParam);
			else
				fprintf(src, "\t\t\t\t%s.getASN1Schema({ name: \"%s\"%s })", szPropertyName, szFieldName, szOptionalParam);
		}
	}

	fprintf(src, "\n");
	fprintf(src, "\t\t\t]\n");
	fprintf(src, "\t\t});\n");
	fprintf(src, "\t}\n\n");

	/* Write out properties */
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.sequence)
	{
		if (choice->basicType->a.sequence->curr->prev)
			fprintf(src, "\n");
		fprintf(src, "\t");
		{
			char* szConverted2 = FixName(e->fieldName);
			fprintf(src, "public %s?", szConverted2);
			free(szConverted2);
		}
		PrintTSType(src, mods, m, td, choice, e->type);
		fprintf(src, ";");
	}

	fprintf(src, "\n");

	if (pSeqElementNamedType)
		free(pSeqElementNamedType);

	/* close class definition */
	fprintf(src, "}\n");
	free(szConverted);
} /* PrintTSChoiceDefCode */

void PrintTSSeqDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* seq, int novolatilefuncs)
{
	NamedType* e;
	//	CxxTRI *cxxtri=NULL;
	//	int inTailOptElmts;
	//	enum BasicTypeChoiceId tmpTypeId;
	//	int allOpt;

	// DEFINE PER encode/decode tmp vars.
	NamedType** pSeqElementNamedType = NULL;

	char* szConverted = FixName(td->definedName);
	fprintf(src, "// [%s]\n", __FUNCTION__);

	printSequenceComment(src, m, td, COMMENTSTYLE_TYPESCRIPT);

	/* put class spec in hdr file */
	fprintf(src, "export class %s {\n", szConverted);
	// fprintf(src, "\ttype: \"%s\",\n", td->definedName);

	int iMandatoryFields = 0;
	int iOptionalFields = 0;

	// Wieviele Pflicht Elemente hat die Sequence?
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;
		if (e->type->optional)
			iOptionalFields++;
		else
			iMandatoryFields++;
	}

	// Jetzt schreiben wir den Konstruktor mit dem einen Argumenten was unserer eigenen Klasse entspricht
	// Damit erzwingen wir die dedizierte Angabe der pflicht Attribute beim Konstruktor aufruf
	fprintf(src, "\tpublic constructor(that");

	// Haben wir keine Pflicht Elemente ist auch das Attribut am Konstruktor Optional
	if (!iMandatoryFields)
		fprintf(src, "?");

	fprintf(src, ": %s) {\n", szConverted);
	if (IsDeprecatedFlaggedSequence(m, td->definedName))
	{
		asnsequencecomment comment;
		GetSequenceComment_UTF8(m->moduleName, td->definedName, &comment);
		fprintf(src, "\t\tTSDeprecatedCallback.deprecatedObject(%lld, MODULE_NAME, this);\n", comment.i64Deprecated);
	}
	fprintf(src, "\t\tObject.assign(this, that);\n");
	fprintf(src, "\t}\n\n");

	// and a static initEmpty() method to be able to create an empty object if necessary
	fprintf(src, "\tprivate static initEmpty(): %s {\n", szConverted);
	fprintf(src, "\t\treturn new %s(", szConverted);
	if (iMandatoryFields)
	{
		fprintf(src, "{\n");
		bool bFirst = true;
		FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
		{
			if (e->type->basicType->choiceId == BASICTYPE_EXTENSION || e->type->optional)
				continue;

			if (!bFirst)
				fprintf(src, ",\n");

			char* szConverted2 = FixName(e->fieldName);
			fprintf(src, "\t\t\t%s: ", szConverted2);
			free(szConverted2);

			PrintTS_JSON_DefaultValue(src, mods, m, td, seq, e->type);
			bFirst = false;
		}
		fprintf(src, "\n\t\t}");
	}
	fprintf(src, ");\n\t}\n\n");

	// and a static initEmpty() method to be able to create an empty object if necessary
	fprintf(src, "\tpublic static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {\n");
	if (iMandatoryFields)
	{
		fprintf(src, "\t\tconst p = [\n");
		bool bFirst = true;
		FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
		{
			if (e->type->basicType->choiceId == BASICTYPE_EXTENSION || e->type->optional)
				continue;
			if (!bFirst)
				fprintf(src, ",\n");
			char* szConverted2 = FixName(e->fieldName);
			fprintf(src, "\t\t\t\"%s\"", szConverted2);
			free(szConverted2);
			bFirst = false;
		}
		if (!bFirst)
			fprintf(src, "\n");
		fprintf(src, "\t\t];\n");
	}
	else
		fprintf(src, "\t\tconst p: string[] = [];\n");

	if (iOptionalFields)
	{
		fprintf(src, "\t\tif (bIncludeOptionals)%s\n", iOptionalFields > 1 ? " {" : "");
		fprintf(src, "\t\t\tp.push(%s", iOptionalFields > 1 ? "\n" : "");

		bool bFirst = true;
		FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
		{
			if (e->type->basicType->choiceId == BASICTYPE_EXTENSION || !e->type->optional)
				continue;
			if (!bFirst)
				fprintf(src, ",\n");
			char* szConverted2 = FixName(e->fieldName);
			fprintf(src, "%s\"%s\"", iOptionalFields > 1 ? "\t\t\t\t" : "", szConverted2);
			free(szConverted2);
			bFirst = false;
		}
		if (!bFirst && iOptionalFields > 1)
			fprintf(src, "\n");
		fprintf(src, "%s);\n", iOptionalFields > 1 ? "\t\t\t" : "");
		if (iOptionalFields > 1)
			fprintf(src, "\t\t}\n");
	}

	fprintf(src, "\t\treturn p;\n");
	fprintf(src, "\t}\n\n");

	fprintf(src, "\tpublic static type = \"%s\";\n\n", szConverted);

	// This method provides the schema for the asn1ts validator
	fprintf(src, "\tpublic static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {\n");
	fprintf(src, "\t\treturn new asn1ts.Sequence({\n");
	fprintf(src, "\t\t\tname: \"%s\",\n", szConverted);
	fprintf(src, "\t\t\t...params,\n");
	fprintf(src, "\t\t\tvalue: [\n");

	bool bFirst = true;
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		enum BasicTypeChoiceId choiceId = e->type->basicType->choiceId;

		if (IsDeprecatedNoOutputMember(m, td, e->fieldName))
			continue;

		if (!bFirst)
			fprintf(src, ",\n");

		fprintf(src, "\t\t\t\t");

		const bool bOptional = e->type->optional ? true : false;
		// 		const bool bImplicit = e->type->implicit ? true : false;

		char szOptionalParam[128] = {0};
		int iOptionalID = -1;
		if (bOptional)
		{
			iOptionalID = GetContextID(e->type);
			if (iOptionalID > -1)
				sprintf_s(szOptionalParam, sizeof(szOptionalParam), ", idBlock: { optionalID: %i }", iOptionalID);
			else
				sprintf_s(szOptionalParam, sizeof(szOptionalParam), "%s", ", optional: true");
		}

		char* szConverted2 = FixName(e->fieldName);
		if (choiceId == BASICTYPE_EXTENSION)
			fprintf(src, "new asn1ts.%s()", GetBERType(e->type->basicType->choiceId));
		else if (IsSimpleType(choiceId))
			fprintf(src, "new asn1ts.%s({ name: \"%s\"%s })", GetBERType(e->type->basicType->choiceId), szConverted2, szOptionalParam);
		else if (choiceId == BASICTYPE_LOCALTYPEREF || choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			const char* szModuleName = "";
			const char* szModuleNameDelimiter = "";
			if (choiceId == BASICTYPE_IMPORTTYPEREF)
			{
				Module* mod = GetImportModuleRefByClassName(e->type->cxxTypeRefInfo->className, mods, m);
				if (mod)
				{
					szModuleName = GetNameSpace(mod);
					szModuleNameDelimiter = ".";
				}
				else
					snacc_exit("unable to find imported module for %s", szConverted2);
			}
			const char* szTypeName = NULL;
			BasicType* pBase = GetBaseBasicType(e->type->basicType, &szTypeName);
			if (pBase->choiceId == BASICTYPE_SEQUENCE || pBase->choiceId == BASICTYPE_SEQUENCEOF || pBase->choiceId == BASICTYPE_CHOICE)
				fprintf(src, "%s%s%s.getASN1Schema({ name: \"%s\"%s })", szModuleName, szModuleNameDelimiter, szTypeName, szConverted2, szOptionalParam);
			else
				fprintf(src, "new asn1ts.%s({ name: \"%s\"%s })", GetBERType(pBase->choiceId), szConverted2, szOptionalParam);
		}
		else
			snacc_exit("unknown choiceId %d", choiceId);
		if (szConverted2)
		{
			free(szConverted2);
			szConverted2 = NULL;
		}

		bFirst = false;
	}
	fprintf(src, "\n\t\t\t]\n");
	fprintf(src, "\t\t});\n");
	fprintf(src, "\t}\n");

	bFirst = true;
	/* Write out properties */
	// fprintf(src, "\tprops: {\n");
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		// OptionalParams ...
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;

		if (IsDeprecatedNoOutputMember(m, td, e->fieldName))
			continue;

		if (bFirst)
		{
			bFirst = false;
			fprintf(src, "\n");
		}

		printMemberComment(src, m, td, e->fieldName, "\t", COMMENTSTYLE_TYPESCRIPT);

		enum BasicTypeChoiceId choiceId = e->type->basicType->choiceId;
		if (choiceId == BASICTYPE_IMPORTTYPEREF || choiceId == BASICTYPE_LOCALTYPEREF)
		{
			if (strcmp(e->type->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
				choiceId = BASICTYPE_UTCTIME;
			else if (choiceId == BASICTYPE_IMPORTTYPEREF)
			{
				if (IsSimpleType(e->type->basicType->a.importTypeRef->link->type->basicType->choiceId))
					choiceId = e->type->basicType->a.importTypeRef->link->type->basicType->choiceId;
			}
			else if (choiceId == BASICTYPE_LOCALTYPEREF)
			{
				if (IsSimpleType(e->type->basicType->a.localTypeRef->link->type->basicType->choiceId))
					choiceId = e->type->basicType->a.localTypeRef->link->type->basicType->choiceId;
			}
		}

		if (choiceId == BASICTYPE_ANY)
			fprintf(src, "\t// eslint-disable-next-line @typescript-eslint/no-explicit-any\n");

		{
			char* szConverted2 = FixName(e->fieldName);
			fprintf(src, "\tpublic %s", szConverted2);
			free(szConverted2);
		}

		if (e->type->optional)
			fprintf(src, "?");
		else
			fprintf(src, "!");

		PrintTSType(src, mods, m, td, seq, e->type);

		fprintf(src, ";\n");
	}
	// fprintf(src, "\n\t}");

	/* Write out collections */
	// FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	//{
	//	if (collectionCounter == 0) {
	//		fprintf(src, ",\n\tcollections: {\n");
	//	}
	//	else {
	//		fprintf(src, ",");
	//		fprintf(src, "\n");
	//	}

	//	// vars
	//	fprintf(src, "\t");
	//	fprintf(src, "%s: %s", e->fieldName, e->type->cxxTypeRefInfo->className);
	//
	//	collectionCounter++;
	//}
	// if (collectionCounter > 0) {
	//	fprintf(src, "\n\t}");
	//}

	if (pSeqElementNamedType)
		free(pSeqElementNamedType);

	/* close class definition */
	fprintf(src, "}\n");
	free(szConverted);
} /* PrintCxxSeqDefCode */

void PrintTSListClass(FILE* src, TypeDef* td, Type* type, Module* m, ModuleList* mods)
{
	char* szName = FixName(td->cxxTypeDefInfo->className);
	fprintf(src, "// [%s]\n", __FUNCTION__);

	const char* szBaseNameArg = NULL;
	char* szBaseName = NULL;
	BasicType* pBase = GetBaseBasicType(type->basicType, &szBaseNameArg);
	if (pBase->choiceId == BASICTYPE_LOCALTYPEREF || pBase->choiceId == BASICTYPE_IMPORTTYPEREF)
		szBaseName = FixName(szBaseNameArg);

	switch (pBase->choiceId)
	{
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_INTEGER:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_REAL:
		case BASICTYPE_UTF8_STR:
			// fprintf(src, "//  %s [No collections of primitive Types %s]\n", td->cxxTypeDefInfo->className, p_e->type->cxxTypeRefInfo->className);
			fprintf(src, "export class %s extends Array<", szName);
			PrintTSNativeType(src, pBase);
			fprintf(src, "> {\n");
			break;
		case BASICTYPE_LOCALTYPEREF:
			fprintf(src, "export class %s extends Array<%s> {\n", szName, szBaseName);
			break;
		case BASICTYPE_IMPORTTYPEREF:
			{
				Module* mod = GetImportModuleRefByClassName(szBaseNameArg, mods, m);
				if (mod)
				{
					const char* szNameSpace = GetNameSpace(mod);
					fprintf(src, "export class %s extends Array<%s.%s> {\n", szName, szNameSpace, szBaseName);
				}
				break;
			}
		default:
			snacc_exit("unsupported choice %i in [PrintTSListClass]", pBase->choiceId);
	}
	fprintf(src, "\tpublic static getASN1Schema(params?: asn1ts.SequenceOfParams): asn1ts.SequenceOf {\n");
	fprintf(src, "\t\treturn new asn1ts.SequenceOf({\n");
	fprintf(src, "\t\t\t...params,\n");
	if (IsSimpleType(pBase->choiceId))
		fprintf(src, "\t\t\tvalue: new asn1ts.%s()\n", GetBERType(pBase->choiceId));
	else if (pBase->choiceId == BASICTYPE_LOCALTYPEREF)
	{
		const char* szBaseName2 = NULL;
		BasicType* pSubBase = GetBaseBasicType(pBase, &szBaseName2);
		if (IsSimpleType(pSubBase->choiceId))
			fprintf(src, "\t\t\tvalue: new asn1ts.%s()\n", GetBERType(pSubBase->choiceId));
		else
			fprintf(src, "\t\t\tvalue: %s.getASN1Schema()\n", szBaseName2);
	}
	else if (pBase->choiceId == BASICTYPE_IMPORTTYPEREF)
	{
		Module* mod = GetImportModuleRefByClassName(pBase->a.importTypeRef->typeName, mods, m);
		if (mod)
		{
			const char* szNameSpace = GetNameSpace(mod);
			const char* szBaseName2 = NULL;
			BasicType* pSubBase = GetBaseBasicType(pBase, &szBaseName2);
			if (IsSimpleType(pSubBase->choiceId))
				fprintf(src, "\t\t\tvalue: new asn1ts.SequenceOf({ value: new asn1ts.%s() })\n", GetBERType(pSubBase->choiceId));
			else
				fprintf(src, "\t\t\tvalue: new asn1ts.SequenceOf({ value: %s.%s.getASN1Schema() })\n", szNameSpace, szBaseName2);
		}
	}
	else
	{
		snacc_exit("unknown pBase->choiceId %d", pBase->choiceId);
	}
	fprintf(src, "\t\t});\n");
	fprintf(src, "\t}\n");
	fprintf(src, "}\n");

	if (szBaseName)
		free(szBaseName);
	free(szName);
}

void PrintTSSetOfDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* setOf, int novolatilefuncs)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);

	/* do class */
	PrintTSListClass(src, td, setOf, m, mods);

} /* PrintTSSetOfDefCode */

void PrintTSimpleDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* setOf, int novolatilefuncs)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);

	fprintf(src, "export type %s = ", td->definedName);
	PrintTSNativeType(src, td->type->basicType);
	fprintf(src, ";\n");

	if (td->type->basicType->choiceId == BASICTYPE_INTEGER && td->type->basicType->a.integer->count)
	{
		fprintf(src, "export enum %senum {\n", td->definedName);
		int iAddComma = 0;
		ValueDef* td2;
		FOR_EACH_LIST_ELMT(td2, td->type->basicType->a.integer)
		{
			if (iAddComma)
				fprintf(src, ",\n");
			fprintf(src, "\t%s = %i", td2->definedName, td2->value->basicValue->a.integer);
			iAddComma = 1;
		}
		fprintf(src, "\n}\n");
	}
}

void PrintTSTypeDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);
	switch (td->type->basicType->choiceId)
	{
		case BASICTYPE_BOOLEAN:		/* library type */
		case BASICTYPE_REAL:		/* library type */
		case BASICTYPE_OCTETSTRING: /* library type */
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_INTEGER:	 /* library type */
		case BASICTYPE_UTF8_STR: /* 27 */
			PrintTSimpleDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SEQUENCEOF: /* list types */
		case BASICTYPE_SETOF:
			PrintTSSetOfDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_IMPORTTYPEREF: /* type references */
			{
				Module* mod = GetImportModuleRefByClassName(td->type->basicType->a.importTypeRef->typeName, mods, m);
				if (mod)
				{
					const char* szNameSpace = GetNameSpace(mod);
					fprintf(src, "export class %s extends %s.%s {\n", td->definedName, szNameSpace, td->type->basicType->a.importTypeRef->typeName);
					fprintf(src, "}\n");
				}
			}
			break;
		case BASICTYPE_LOCALTYPEREF:
			fprintf(src, "export { %s as %s };\n", td->type->basicType->a.localTypeRef->typeName, td->definedName);
			break;
		case BASICTYPE_CHOICE:
			PrintTSChoiceDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_ENUMERATED: /* library type */
			PrintTSEnumDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SEQUENCE:
			PrintTSSeqDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_BITSTRING:
			snacc_exit("Unknown td->type->basicType->choiceId %s not supported for TypeScript!", "BASICTYPE_BITSTRING");
			break;
		default:
			snacc_exit("Unknown td->type->basicType->choiceId %d", td->type->basicType->choiceId);
			break;
	}
} /* PrintCxxTypeDefCode */

/*
 * prints PrintROSEInvoke
 */
void PrintTSROSEInvoke(FILE* hdr, Module* m, int bEvents, ValueDef* vd)
{
} /* PrintROSEInvoke */

void PrintTSTypeDecl(FILE* src, TypeDef* td)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);

	switch (td->type->basicType->choiceId)
	{
		case BASICTYPE_COMPONENTSOF:
		case BASICTYPE_SELECTION:
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_MACRODEF:
		case BASICTYPE_MACROTYPE:
			return; /* do nothing */

		default:
			if (IsNewType(td->type))
				fprintf(src, "// typedef %s\n", td->cxxTypeDefInfo->className);
	}
}

void PrintTSComments(FILE* src, Module* m)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);

	fprintf(src, "/*\n");
	fprintf(src, " * %s\n", RemovePath(m->tsFileName));
	fprintf(src, " * \"%s\" ASN.1 stubs.\n", m->modId->name);
	write_snacc_header(src, " * ");
	fprintf(src, " */\n\n");

	fprintf(src, "// prettier-ignore\n");
	fprintf(src, ESLINT_DISABLE);

	printModuleComment(src, RemovePath(m->baseFilePath), COMMENTSTYLE_TYPESCRIPT);
}

void PrintTSCodeOne(FILE* src, ModuleList* mods, Module* m, long longJmpVal, int printTypes, int printValues, int printEncoders, int printDecoders, int printTSONEncDec, int novolatilefuncs)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);

	// Comments
	PrintTSComments(src, m);

	// Imports
	PrintTSImports(src, mods, m, false, true, true);

	// Root types
	PrintTSRootTypes(src, m, "");

	bool bIsFirst = true;
	TypeDef* td;
	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		if (IsDeprecatedNoOutputSequence(m, td->definedName))
			if (!bIsFirst)
				continue;
		fprintf(src, "\n");
		PrintTSTypeDefCode(src, mods, m, td, novolatilefuncs);
		bIsFirst = false;
	}

} /* PrintTSCode */

void PrintTSCode(ModuleList* allMods, long longJmpVal, int genTypes, int genValues, int genJSONEncDec, int genTSROSEStubs, int genPrinters, int genPrintersXML, int genFree, int novolatilefuncs, int genROSEDecoders)
{
	Module* currMod;
	AsnListNode* saveMods;
	FILE* srcFilePtr = NULL;
	FILE* encdecFilePtr = NULL;
	// FILE		*hdrForwardDecl;
	DefinedObj* fNames;
	int fNameConflict = FALSE;

	/*
	 * Make names for each module's encoder/decoder src and hdr files
	 * so import references can be made via include files
	 * check for truncation --> name conflicts & exit if nec
	 */
	fNames = NewObjList();
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (ObjIsDefined(fNames, currMod->tsFileName, StrObjCmp) || ObjIsDefined(fNames, currMod->tsConverterFileName, StrObjCmp))
		{
			fprintf(errFileG, "Ack! ERROR---file name conflict for generated typescript files with names `%s' and `%s'.\n\n", currMod->tsFileName, currMod->tsConverterFileName);
			fprintf(errFileG, "This usually means the max file name length is truncating the file names.\n");
			fprintf(errFileG, "Try re-naming the modules with shorter names or increasing the argument to -mf option (if you are using it).\n");
			fprintf(errFileG, "This error can also be caused by 2 modules with the same names but different OBJECT IDENTIFIERs.");
			fprintf(errFileG, "Try renaming the modules to correct this.\n");
			fNameConflict = TRUE;
		}
		else
		{
			DefineObj(&fNames, currMod->tsFileName);
			DefineObj(&fNames, currMod->tsConverterFileName);
		}

		if (fNameConflict)
			return;

		FreeDefinedObjs(&fNames);
	}

	if (gMajorInterfaceVersion >= 0 && genVersionFile)
	{
		FILE* versionFile = NULL;
		char* szVersionFile = MakeFileName("Asn1InterfaceVersion.ts", "");
		if (fopen_s(&versionFile, szVersionFile, "wt") != 0 || versionFile == NULL)
			perror("fopen");
		else
		{
			fprintf(versionFile, "/*\n");
			write_snacc_header(versionFile, " * ");
			fprintf(versionFile, "*/\n\n");
			fprintf(versionFile, "// prettier-ignore\n");
			fprintf(versionFile, "/* eslint-disable */\n\n");
			long long lMaxMinorVersion = GetMaxModuleMinorVersion();
			fprintf(versionFile, "export class Asn1InterfaceVersion {\n");
			char* szISODate = ConvertUnixTimeToISO(lMaxMinorVersion);
			fprintf(versionFile, "\tpublic static lastChange = \"%s\";\n", szISODate);
			free(szISODate);
			fprintf(versionFile, "\tpublic static majorVersion = %i;\n", gMajorInterfaceVersion);
			char* szNumericDate = ConvertUnixTimeToNumericDate(lMaxMinorVersion);
			fprintf(versionFile, "\tpublic static minorVersion = %s;\n", szNumericDate);
			fprintf(versionFile, "\tpublic static version = \"%i.0.%s\";\n", gMajorInterfaceVersion, szNumericDate);
			free(szNumericDate);
			fprintf(versionFile, "}\n");
			fclose(versionFile);
		}
		free(szVersionFile);
	}

	FILE* typesFile = NULL;
	char* szTypes = MakeFileName("types.ts", "");
	if (fopen_s(&typesFile, szTypes, "wt") != 0 || typesFile == NULL)
		perror("fopen");
	else
	{
		fprintf(typesFile, "/**\n");
		fprintf(typesFile, " * This file combines exports from asn1 files under one name\n");
		fprintf(typesFile, " *\n");
		write_snacc_header(typesFile, " * ");
		fprintf(typesFile, " */\n\n");

		char* strings[1000];
		memset(strings, 0x00, sizeof(strings));
		int iCount = 0;

		FOR_EACH_LIST_ELMT(currMod, allMods)
		{
			strings[iCount] = MakeFileNameWithoutOutputPath(currMod->baseFilePath, "");
			iCount++;
		}

		qsort(strings, iCount, sizeof(*strings), sortstring);

		iCount = 0;
		for (iCount = 0; iCount < 1000; iCount++)
		{
			// As - is an unsupported variable replace it with _
			char* szModName = strings[iCount];
			if (!szModName)
				break;

			char* varName = Asn1FieldName2CFieldName(szModName);

			fprintf(typesFile, "export * as %s from \"./%s%s\";\n", varName, szModName, getCommonJSFileExtension());
			if (genJSONEncDec)
			{
				char szFileName[_MAX_PATH] = {0};
				strcat_s(szFileName, _MAX_PATH - 1, szModName);
				strcat_s(szFileName, _MAX_PATH - 1, "_Converter.ts");
				char* szConverterFileName = MakeFileName(szFileName, "");
				FILE* exists = NULL;
				if (fopen_s(&exists, szConverterFileName, "r") == 0)
				{
					fprintf(typesFile, "export * as %s_Converter from \"./%s_Converter%s\";\n", varName, szModName, getCommonJSFileExtension());
					fclose(exists);
				}
			}

			free(varName);
			free(szModName);
		}

		fclose(typesFile);
	}
	free(szTypes);

	FILE* methodsFile = NULL;
	char* szMethods = MakeFileName("methods.ts", "");
	if (fopen_s(&methodsFile, szMethods, "wt") != 0 || methodsFile == NULL)
		perror("fopen");
	else
	{
		fprintf(methodsFile, "/**\n");
		fprintf(methodsFile, " * This file exports all specified ROSE methods as arrays\n");
		fprintf(methodsFile, " *\n");
		write_snacc_header(methodsFile, " * ");
		fprintf(methodsFile, " */\n\n");
		fprintf(methodsFile, "export interface IROSEMethod {\n");
		fprintf(methodsFile, "\tname: string;\n");
		fprintf(methodsFile, "\tid: number;\n");
		fprintf(methodsFile, "}\n\n");
		fprintf(methodsFile, "export interface IROSEMethodOverview {\n");
		fprintf(methodsFile, "\tname: string;\n");
		fprintf(methodsFile, "\tmethods: IROSEMethod[];\n");
		fprintf(methodsFile, "}\n\n");

#define COUNT 5000
		char* strings[COUNT];
		memset(strings, 0x00, sizeof(strings));
		int iCountStrings = 0;
		char* interfaces[COUNT];
		memset(interfaces, 0x00, sizeof(interfaces));
		int iCountInterfaces = 0;

		FOR_EACH_LIST_ELMT(currMod, allMods)
		{
			if (currMod->ImportedFlag == FALSE)
			{
				if (HasROSEOperations(currMod))
				{
					char* baseName = _strdup(currMod->moduleName);
					{
						char szBuffer[512] = {0};
						char* szReadPos = currMod->moduleName;
						int iPos = 0;
						while (*szReadPos)
						{
							if (*szReadPos != '-' && *szReadPos != '_')
							{
								szBuffer[iPos] = *szReadPos;
								iPos++;
							}
							szReadPos++;
						}
						szBuffer[iPos] = 0;

						// Take into account that the length in bytes does not contain the terminating 0 byte
						strcpy_s(baseName, strlen(baseName) + 1, szBuffer);

						interfaces[iCountInterfaces] = baseName;
						iCountInterfaces++;
					}

					size_t baseLen = strlen(baseName) + 3 + 10; // 2*space and null byte at the end + at least 4 digit operationID

					ValueDef* vd;
					FOR_EACH_LIST_ELMT(vd, currMod->valueDefs)
					{
						if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
						{
							const char* pszFunction = vd->definedName;
							size_t len = strlen(pszFunction) + baseLen;
							char szOperationID[10] = {0};
							int iValue = -1;
							if (vd->value->basicValue->choiceId == BASICVALUE_INTEGER)
								iValue = vd->value->basicValue->a.integer;
							sprintf_s(szOperationID, 10, "%d", iValue);
							char* szBuffer = malloc(len);
							szBuffer[0] = 0;
							strcat_s(szBuffer, len, baseName);
							strcat_s(szBuffer, len, " ");
							strcat_s(szBuffer, len, szOperationID);
							strcat_s(szBuffer, len, " ");
							strcat_s(szBuffer, len, pszFunction);
							strings[iCountStrings] = szBuffer;
							iCountStrings++;
						}
					}
				}
			}
		}
		free(szMethods);

		qsort(strings, iCountStrings, sizeof(*strings), sortstring);

		iCountStrings = 0;
		char* szLastModule = NULL;
		int bAddComma = FALSE;
		for (iCountStrings = 0; iCountStrings < COUNT; iCountStrings++)
		{
			char* szData = strings[iCountStrings];
			if (!szData)
				break;

#ifdef _WIN32
			char* next_token = NULL;
			char* szModule = strtok_s(szData, " ", &next_token);
			char* szID = strtok_s(NULL, " ", &next_token);
			char* szMethod = strtok_s(NULL, " ", &next_token);
#else  // _WIN32
			char* szModule = strtok(szData, " ");
			char* szID = strtok(NULL, " ");
			char* szMethod = strtok(NULL, " ");
#endif // _WIN32

			if (!szModule || !szID || !szMethod)
				continue;

			if (!szLastModule || strcmp(szLastModule, szModule) != 0)
			{
				if (szLastModule)
					fprintf(methodsFile, "\n];\n\n");
				fprintf(methodsFile, "export const methods%s: IROSEMethod[] = [\n", szModule);
				szLastModule = szModule;
				bAddComma = FALSE;
			}
			if (bAddComma)
				fprintf(methodsFile, ",\n");
			fprintf(methodsFile, "\t{ name: \"%s\", id: %s }", szMethod, szID);
			bAddComma = TRUE;
		}

		if (szLastModule)
			fprintf(methodsFile, "\n];\n\n");

		if (iCountInterfaces)
		{
			qsort(interfaces, iCountInterfaces, sizeof(*interfaces), sortstring);

			fprintf(methodsFile, "export const roseInterfaceMethods: IROSEMethodOverview[] = [\n");
			bAddComma = FALSE;
			for (iCountInterfaces = 0; iCountInterfaces < COUNT; iCountInterfaces++)
			{
				char* szInterface = interfaces[iCountInterfaces];
				if (!szInterface)
					break;
				if (bAddComma)
					fprintf(methodsFile, ",\n");
				fprintf(methodsFile, "\t{ name: \"%s\", methods: methods%s }", szInterface, szInterface);
				bAddComma = TRUE;
			}

			fprintf(methodsFile, "\n];\n");
		}

		for (iCountStrings = 0; iCountStrings < COUNT; iCountStrings++)
		{
			char* szData = strings[iCountStrings];
			if (!szData)
				break;
			free(szData);
		}

		for (iCountInterfaces = 0; iCountInterfaces < COUNT; iCountInterfaces++)
		{
			char* szInterface = interfaces[iCountInterfaces];
			if (!szInterface)
				break;
			free(szInterface);
		}

		fclose(methodsFile);
	}

	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			if (fopen_s(&srcFilePtr, currMod->tsFileName, "wt") != 0 || srcFilePtr == NULL)
				perror("fopen");
			else
			{
				saveMods = allMods->curr;
				PrintTSCodeOne(srcFilePtr, allMods, currMod, longJmpVal, genTypes, genValues, genJSONEncDec, genJSONEncDec, genJSONEncDec, novolatilefuncs);
				allMods->curr = saveMods;
				fclose(srcFilePtr);
			}
		}
	}

	if (genJSONEncDec)
	{
		SaveTSConverterFilesToOutputDirectory(gszOutputPath);

		FOR_EACH_LIST_ELMT(currMod, allMods)
		{
			if (currMod->ImportedFlag == FALSE)
			{
				if (!ContainsConverters(currMod))
					continue;
				if (fopen_s(&encdecFilePtr, currMod->tsConverterFileName, "wt") != 0 || encdecFilePtr == NULL)
				{
					perror("fopen");
				}
				else
				{
					saveMods = allMods->curr;
					PrintTSConverterCode(encdecFilePtr, allMods, currMod, longJmpVal, genTypes, genValues, genJSONEncDec, genJSONEncDec, genJSONEncDec, novolatilefuncs);
					allMods->curr = saveMods;
					fclose(encdecFilePtr);
				}
			}
		}
	}

	SaveTSROSEFilesToOutputDirectory(genTSROSEStubs, gszOutputPath);

	if (genTSROSEStubs)
	{
		FOR_EACH_LIST_ELMT(currMod, allMods)
		{
			if (currMod->ImportedFlag == FALSE)
			{
				if (HasROSEOperations(currMod))
				{
					if (IsDeprecatedNoOutputModule(currMod))
						continue;

					saveMods = allMods->curr;

					char szStubFileName[_MAX_PATH];
					strcpy_s(szStubFileName, _MAX_PATH - 1, gszOutputPath);
					strcat_s(szStubFileName, _MAX_PATH - 1, currMod->ROSEClassName);
					strcat_s(szStubFileName, _MAX_PATH - 1, ".ts");
					FILE* stubFilePtr = NULL;
					if (fopen_s(&stubFilePtr, szStubFileName, "wt") != 0 || stubFilePtr == NULL)
					{
						perror("fopen tsROSEClientFileName");
					}
					else
					{
						allMods->curr = saveMods;
						PrintTSROSECode(stubFilePtr, allMods, currMod);
						fclose(stubFilePtr);
					}

					if (genTSROSEStubs & 0x06)
					{
						// Client Interfaces
						char szClientInterfaceFileName[_MAX_PATH];
						strcpy_s(szClientInterfaceFileName, _MAX_PATH - 1, gszOutputPath);
						strcat_s(szClientInterfaceFileName, _MAX_PATH - 1, currMod->ROSEClassName);
						strcat_s(szClientInterfaceFileName, _MAX_PATH - 1, "_Interface.ts");

						FILE* roseClientInterfaceFilePtr = NULL;

						if (fopen_s(&roseClientInterfaceFilePtr, szClientInterfaceFileName, "wt") != 0 || roseClientInterfaceFilePtr == NULL)
						{
							perror("fopen tsROSEClientFileName");
						}
						else
						{
							allMods->curr = saveMods;
							PrintTSROSEInterfaceCode(roseClientInterfaceFilePtr, allMods, currMod);
							fclose(roseClientInterfaceFilePtr);
						}
					}

					if (genTSROSEStubs & 0x01)
					{
						// Server (Node) stub
						char szServerInterfaceFileName[_MAX_PATH];
						strcpy_s(szServerInterfaceFileName, _MAX_PATH - 1, gszOutputPath);
						strcat_s(szServerInterfaceFileName, _MAX_PATH - 1, currMod->ROSEClassName);
						strcat_s(szServerInterfaceFileName, _MAX_PATH - 1, "_Interface.ts");

						FILE* roseServerInterfaceFilePtr = NULL;

						if (fopen_s(&roseServerInterfaceFilePtr, szServerInterfaceFileName, "wt") != 0 || roseServerInterfaceFilePtr == NULL)
						{
							perror("fopen tsROSEClientFileName");
						}
						else
						{
							allMods->curr = saveMods;
							PrintTSROSEInterfaceCode(roseServerInterfaceFilePtr, allMods, currMod);
							fclose(roseServerInterfaceFilePtr);
						}
					}

					allMods->curr = saveMods;
				}
			}
		}
	}
}

/* EOF gen-code.c (for back-ends/TS-gen) */
