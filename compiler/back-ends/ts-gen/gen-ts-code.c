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
#include "../../core/asn_comments.h"
#include <inttypes.h>
#include <assert.h>

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
		BasicType* pBasicType = t->basicType;
		enum BasicTypeChoiceId choiceId = pBasicType->choiceId;
		if (choiceId == BASICTYPE_IMPORTTYPEREF || choiceId == BASICTYPE_LOCALTYPEREF)
		{
			if (strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
				choiceId = BASICTYPE_UTCTIME;
			else if (choiceId == BASICTYPE_IMPORTTYPEREF && t->basicType->a.importTypeRef->link)
			{
				if (IsSimpleType(t->basicType->a.importTypeRef->link->type->basicType->choiceId))
				{
					pBasicType = t->basicType->a.importTypeRef->link->type->basicType;
					choiceId = pBasicType->choiceId;
				}
			}
			else if (choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link)
			{
				if (IsSimpleType(t->basicType->a.localTypeRef->link->type->basicType->choiceId))
				{
					pBasicType = t->basicType->a.localTypeRef->link->type->basicType;
					choiceId = pBasicType->choiceId;
				}
			}
			else
			{
				snacc_exit("Invalid parameter, choiceID is %i but the associated link is NULL", choiceId);
				return;
			}
		}

		if (choiceId == BASICTYPE_OCTETCONTAINING && pBasicType->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
			choiceId = BASICTYPE_UTF8_STR;

		switch (choiceId)
		{
			case BASICTYPE_BOOLEAN:
				fprintf(hdr, "false");
				break;
			case BASICTYPE_INTEGER:
			case BASICTYPE_ENUMERATED:
				fprintf(hdr, "0");
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
					Module* mod = GetImportModuleRefByClassName(t->cxxTypeRefInfo->className, mods, m);
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
	BasicType* basicType = t->basicType;
	enum BasicTypeChoiceId choiceId = basicType->choiceId;

	if (choiceId == BASICTYPE_IMPORTTYPEREF || choiceId == BASICTYPE_LOCALTYPEREF)
	{
		if (strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
			choiceId = BASICTYPE_UTCTIME;
		else if (choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			if (IsSimpleType(t->basicType->a.importTypeRef->link->type->basicType->choiceId))
			{
				basicType = t->basicType->a.importTypeRef->link->type->basicType;
				choiceId = basicType->choiceId;
			}
		}
		else if (choiceId == BASICTYPE_LOCALTYPEREF)
		{
			if (IsSimpleType(t->basicType->a.localTypeRef->link->type->basicType->choiceId))
			{
				basicType = t->basicType->a.localTypeRef->link->type->basicType;
				choiceId = basicType->choiceId;
			}
		}
	}

	if (choiceId == BASICTYPE_OCTETCONTAINING && basicType->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
		choiceId = BASICTYPE_UTF8_STR;

	switch (choiceId)
	{
		case BASICTYPE_BOOLEAN:
			fprintf(hdr, ": boolean");
			break;
		case BASICTYPE_INTEGER:
		case BASICTYPE_REAL:
		case BASICTYPE_ENUMERATED:
			fprintf(hdr, ": number");
			break;
		case BASICTYPE_UTF8_STR:
			fprintf(hdr, ": string");
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(hdr, ": Uint8Array");
			break;
		case BASICTYPE_SEQUENCEOF:
			fprintf(hdr, ": %s[]", t->cxxTypeRefInfo->className);
			break;
		case BASICTYPE_NULL:
			fprintf(hdr, ": null");
			break;
		case BASICTYPE_UTCTIME:
			fprintf(hdr, ": Date"); // AsnSystemTime is defined as REAL in the Asn1 file, but is transmitted as a string in the TS.
			break;
		case BASICTYPE_IMPORTTYPEREF:
			{
				Module* mod = GetImportModuleRefByClassName(t->cxxTypeRefInfo->className, mods, m);
				if (mod)
				{
					char* szConverted = FixName(t->cxxTypeRefInfo->className);
					fprintf(hdr, ": %s.%s", GetNameSpace(mod), szConverted);
					free(szConverted);
				}
				break;
			}
		case BASICTYPE_LOCALTYPEREF:
			{
				char* szConverted = FixName(t->basicType->a.localTypeRef->link->definedName);
				fprintf(hdr, ": %s", szConverted);
				free(szConverted);
				break;
			}
		case BASICTYPE_ANY:
			fprintf(hdr, ": any");
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

	/* Static method to get all own properties as key string */
	fprintf(src, "\tpublic static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {\n");
	fprintf(src, "\t\tconst p = [");
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.sequence)
	{
		if (!e->type->optional && e->fieldName != NULL) 
		{
			char* szConverted2 = FixName(e->fieldName);
			fprintf(src, "\n\t\t\t\"%s\",", szConverted2);
			free(szConverted2);
		}
	}
	fprintf(src, "\n\t\t];\n");

	fprintf(src, "\t\tif (bIncludeOptionals) {");
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.sequence)
	{
		if (e->type->optional && e->fieldName != NULL) 
		{
			char* szConverted2 = FixName(e->fieldName);
			fprintf(src, "\n\t\t\tp.push(\"%s\");", szConverted2);
			free(szConverted2);
		}
	}
	fprintf(src, "\n\t\t};\n");
	fprintf(src, "\t\treturn p;\n");
	fprintf(src, "\t};\n");

	fprintf(src, "\n");


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

	// Wieviele Pflicht Elemente hat die Sequence?
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION || e->type->optional)
			continue;
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
		fprintf(src, "\t\tTSDeprecatedCallback.deprecatedObject(%" PRId64 ", moduleName, this);\n", comment.i64Deprecated);
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
	fprintf(src, "\t}\n\n");

		/* Static method to get all own properties as key string */
	fprintf(src, "\tpublic static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {\n");
	fprintf(src, "\t\tconst p = [");
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		if (!e->type->optional && e->fieldName != NULL) 
		{
			char* szConverted2 = FixName(e->fieldName);
			fprintf(src, "\n\t\t\t\"%s\",", szConverted2);
			free(szConverted2);
		}
	}
	fprintf(src, "\n\t\t];\n");

	fprintf(src, "\t\tif (bIncludeOptionals) {");
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		if (e->type->optional && e->fieldName != NULL) 
		{
			char* szConverted2 = FixName(e->fieldName);
			fprintf(src, "\n\t\t\tp.push(\"%s\");", szConverted2);
			free(szConverted2);
		}
	}
	fprintf(src, "\n\t\t};\n");
	fprintf(src, "\t\treturn p;\n");
	fprintf(src, "\t};\n");

	fprintf(src, "\n");

	/* Write out properties */
	// fprintf(src, "\tprops: {\n");
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		// OptionalParams ...
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;

		if (IsDeprecatedNoOutputMember(m, td, e->fieldName))
			continue;

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

	printModuleComment(src, RemovePath(m->baseFileName), COMMENTSTYLE_TYPESCRIPT);
}

void PrintTSCode(FILE* src, ModuleList* mods, Module* m, long longJmpVal, int printTypes, int printValues, int printEncoders, int printDecoders, int printTSONEncDec, int novolatilefuncs)
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

/* EOF gen-code.c (for back-ends/TS-gen) */
