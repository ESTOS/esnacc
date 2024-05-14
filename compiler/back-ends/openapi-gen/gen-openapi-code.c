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
 * 2016 ESTOS/stm
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include "gen-openapi-code.h"
#include "../../core/asn_comments.h"
#include "../str-util.h"
#include "compiler/back-ends/structure-util.h"
#include "compiler/core/asn1module.h"
#include "snacc.h"

char* replace_str(const char* str, const char* orig, const char* rep)
{
	if (!str || !orig || !rep)
		return NULL;

	// Get the length of the strings
	size_t len_orig = strlen(orig);
	if (!len_orig)
		return NULL;
	size_t len_rep = strlen(rep);

	// Get the amount of replaces we have to handle (to be able to calculate the length)
	int count = 0;
	char* tmp = strstr(str, orig);
	while (tmp)
	{
		count++;
		tmp += len_orig;
		tmp = strstr(tmp, orig);
	}

	// Source len + len difference * count of replacers + null byte
	size_t resultSize = strlen(str) + (len_rep - len_orig) * count + 1;
	char* result = malloc(resultSize);

	if (!result)
		return NULL;

	memset(result, 0x00, resultSize);
	tmp = result;
	while (count--)
	{
		// Get the first position for a replacement
		const char* ins = strstr(str, orig);
		// Get the length information of the replacement (this is the amount of data we need to copy)
		size_t len_front = ins - str;

		// Copy the data prior to the replacement into the result buffer
		strncpy_s(tmp, resultSize, str, len_front);
		tmp += len_front;
		resultSize -= len_front;

		// Add the replacer
		strcat_s(tmp, resultSize, rep);
		tmp += len_rep;
		resultSize -= len_rep;

		// Correct the starting position
		str += len_front + len_orig;
	}
	strcat_s(tmp, resultSize, str);
	return result;
}

char* replace_multi(const char* str, const char** orig, const char** rep)
{
	char* result = _strdup(str);
	int i = 0;
	while (true)
	{
		const char* o = orig[i];
		const char* r = rep[i];
		if (!o || !r)
			break;
		char* replaced = replace_str(result, o, r);
		if (replaced != NULL)
		{
			free(result);
			result = replaced;
		}
		i++;
	}
	return result;
}

static void PrintOpenApiTypeDefinitions(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* tp, char* tabs);

static void PrintOpenApiStringType(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* tp, char* tabs)
{
	if (td != NULL)
		fprintf(src, "%s\"%s\": {\n", tabs, td->definedName);
	else
		fprintf(src, "{\n");
	fprintf(src, "%s\t\"type\": \"string\"\n", tabs);
	fprintf(src, "%s}", tabs);
}

static void PrintOpenApiTimeType(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* tp, char* tabs)
{
	if (td != NULL)
		fprintf(src, "%s\"%s\": {\n", tabs, td->definedName);
	else
		fprintf(src, "{\n");
	fprintf(src, "%s\t\"type\": \"string\",\n", tabs);
	fprintf(src, "%s\t\"format\": \"date-time\"\n", tabs);
	fprintf(src, "%s}", tabs);
}

static void PrintOpenApiPrimitiveType(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* tp, char* tabs)
{
	if (td != NULL)
		fprintf(src, "%s\"%s\": {", tabs, td->definedName);
	else
		fprintf(src, "{");

	switch (tp->basicType->choiceId)
	{
		case BASICTYPE_ANY:		/* 16 */
		case BASICTYPE_UNKNOWN: /* 0 */
			fprintf(src, "}");
			break;
		case BASICTYPE_BOOLEAN: /* 1 */
			fprintf(src, "\n%s\t\"type\": \"boolean\"\n%s}", tabs, tabs);
			break;
		case BASICTYPE_INTEGER: /* 2 */
			fprintf(src, "\n%s\t\"type\": \"integer\"\n%s}", tabs, tabs);
			break;
		case BASICTYPE_NULL: /* 5 */
			fprintf(src, "\n%s\t\"nullable\": \"true\"\n%s}", tabs, tabs);
			break;
		case BASICTYPE_REAL: /* 7 */
			fprintf(src, "\n%s\t\"type\": \"number\",\n%s\t\"format\": \"float\"\n%s}", tabs, tabs, tabs);
			break;
		case BASICTYPE_OCTETSTRING:
			fprintf(src, "\n%s\t\"type\": \"string\",\n%s\t\"format\": \"byte\"\n%s}", tabs, tabs, tabs);
			break;
		default:
			exit(1);
			break;
	}
}

void PrintOpenApiSequence(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* tp, char* tabs)
{
	if (td != NULL)
		fprintf(src, "%s\"%s\": {\n", tabs, FixName(td->definedName));
	else
		fprintf(src, "{\n");

	NamedType* n;
	fprintf(src, "%s\t\"type\": \"object\"", tabs);

	asnsequencecomment comment;
	if (td != NULL && GetSequenceComment_UTF8(m->moduleName, td->definedName, &comment))
	{
		fprintf(src, ",\n%s\t\"description\": \"", tabs);
		if (comment.szCategory[0])
			fprintf(src, "### Category \\n%s\\n", comment.szCategory);
		if (comment.szShort[0])
			fprintf(src, "### Short \\n%s\\n", comment.szShort);
		if (comment.szLong[0])
			fprintf(src, "### Long \\n%s\\n", comment.szLong);
		if (comment.i64Deprecated)
			fprintf(src, "### Deprecated \\n%d\\n", comment.i64Deprecated ? 1 : 0);
		if (comment.i64Deprecated > 1)
			fprintf(src, "### deprecated_timestamp \\n%lld\\n", comment.i64Deprecated);
		if (comment.szDeprecated[0])
			fprintf(src, "### Deprecated_comment \\n%s\\n", comment.szDeprecated);
		if (comment.i64Added)
			fprintf(src, "### added_timestamp \\n%lld\\n", comment.i64Added);
		fprintf(src, "### Private \\n%d\"", comment.iPrivate);
	}

	bool bFirst = true;

	FOR_EACH_LIST_ELMT(n, tp->basicType->choiceId == BASICTYPE_SEQUENCE ? tp->basicType->a.sequence : tp->basicType->a.set)
	{
		if (IsDeprecatedNoOutputMember(m, td, n->fieldName) || n->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;
		if (bFirst)
			fprintf(src, ",\n%s\t\"properties\": {\n", tabs);
		else
			fprintf(src, ",\n");
		fprintf(src, "%s\t\t\"%s\": ", tabs, n->fieldName);
		char newTabs[100];
		sprintf_s(newTabs, 100, "%s\t", tabs);
		// Insert Member comments
		PrintOpenApiTypeDefinitions(src, mods, m, NULL, n->type, newTabs);
		bFirst = false;
	}

	if (bFirst == false)
		fprintf(src, "\n%s\t}", tabs);

	bFirst = true;
	FOR_EACH_LIST_ELMT(n, tp->basicType->choiceId == BASICTYPE_SEQUENCE ? tp->basicType->a.sequence : tp->basicType->a.set)
	{
		if (IsDeprecatedNoOutputMember(m, td, n->fieldName) || n->type->optional || n->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;
		if (bFirst)
			fprintf(src, ",\n%s\t\"required\": [", tabs);
		else
			fprintf(src, ",");
		fprintf(src, "\n%s\t\t\"%s\"", tabs, n->fieldName);
		bFirst = false;
	}
	if (bFirst == false)
		fprintf(src, "\n%s\t]", tabs);

	/* close class definition */
	fprintf(src, "\n%s}", tabs);
}

void PrintOpenApiSequenceOf(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* tp, char* tabs)
{
	if (td != NULL)
		fprintf(src, "%s\"%s\": {\n", tabs, FixName(td->definedName));
	else
		fprintf(src, "{\n");

	fprintf(src, "%s\t\"type\": \"array\",\n%s\t\"items\": ", tabs, tabs);
	char newTabs[100] = {0};
	sprintf_s(newTabs, 100, "%s", tabs);
	PrintOpenApiTypeDefinitions(src, mods, m, NULL, tp->basicType->choiceId == BASICTYPE_SEQUENCEOF ? tp->basicType->a.sequenceOf : tp->basicType->a.setOf, newTabs);
	fprintf(src, "\n%s}", tabs);
}

void PrintOpenApiEnum(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* tp, char* tabs)
{
	if (td != NULL)
		fprintf(src, "%s\"%s\": {\n", tabs, td->definedName);
	else
		fprintf(src, "{\n");

	//	NamedType *e;
	//	enum BasicTypeChoiceId tmpTypeId;
	CNamedElmt* n;
	fprintf(src, "%s\t\"type\": \"integer\"", tabs);

	if (HasNamedElmts(tp) != 0)
	{
		bool bFirst = true;
		FOR_EACH_LIST_ELMT(n, tp->cxxTypeRefInfo->namedElmts)
		{
			if (IsDeprecatedNoOutputMember(m, td, n->name))
				continue;
			if (bFirst)
				fprintf(src, ",\n%s\t\"oneOf\": [", tabs);
			else
				fprintf(src, ",");
			fprintf(src, "\n%s\t\t{\n%s\t\t\t\"title\": \"%s\",\n%s\t\t\t\"const\": %d\n%s\t\t}", tabs, tabs, n->name, tabs, n->value, tabs);
			bFirst = false;
		}
		if (bFirst == FALSE)
			fprintf(src, "]");
	}

	/* close class definition */
	fprintf(src, "}");
}

void PrintOpenApiChoice(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* tp, char* tabs)
{
	if (td != NULL)
		fprintf(src, "%s\"%s\": {\n", tabs, td->definedName);
	else
		fprintf(src, "{\n");

	//	NamedType *e;
	//	enum BasicTypeChoiceId tmpTypeId;
	NamedType* n;

	bool bFirst = true;
	FOR_EACH_LIST_ELMT(n, tp->basicType->a.sequence)
	{
		if (IsDeprecatedNoOutputMember(m, td, n->fieldName))
			continue;
		if (bFirst)
		{
			bFirst = false;
			fprintf(src, "%s\t\"oneOf\": [\n%s\t\t", tabs, tabs);
		}
		else
			fprintf(src, ",\n%s\t\t", tabs);
		char newTabs[100] = {0};
		sprintf_s(newTabs, 100, "%s\t", tabs);
		PrintOpenApiTypeDefinitions(src, mods, m, NULL, n->type, newTabs);
	}
	fprintf(src, "\n\t%s]", tabs);

	/* close class definition */
	fprintf(src, "\n%s}", tabs);
}

void PrintOpenApiRef(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* tp, char* tabs)
{
	if (td != NULL)
		fprintf(src, "%s\"%s\": {\n", tabs, td->definedName);
	else
		fprintf(src, "{\n");

	switch (tp->basicType->choiceId)
	{
		case BASICTYPE_LOCALTYPEREF:
			fprintf(src, "%s\t\"$ref\": \"#/components/schemas/%s\"\n%s}", tabs, tp->basicType->a.localTypeRef->typeName, tabs);
			break;
		case BASICTYPE_IMPORTTYPEREF:
			fprintf(src, "%s\t\"$ref\": \"#/components/schemas/%s\"\n%s}", tabs, tp->basicType->a.importTypeRef->typeName, tabs);
			break;
		default:
			exit(1);
			break;
	}
}

static void PrintOpenApiTypeDefinitions(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* tp, char* tabs)
{
	char newTabs[100];
	sprintf_s(newTabs, 100, "%s\t", tabs);

	// Special cases for special types
	if (td != NULL && strcmp(td->definedName, "AsnSystemTime") == 0)
		PrintOpenApiTimeType(src, mods, m, td, tp, newTabs);

	else
		switch (tp->basicType->choiceId)
		{
			// Primitive Types
			case BASICTYPE_UNKNOWN: /* 0 */
			case BASICTYPE_BOOLEAN: /* 1 */
			case BASICTYPE_INTEGER: /* 2 */
			case BASICTYPE_NULL:	/* 5 */
			case BASICTYPE_REAL:	/* 7 */
			// Base64 String
			case BASICTYPE_OCTETSTRING: /* 4 */
			case BASICTYPE_ANY:			/* 16 */
				PrintOpenApiPrimitiveType(src, mods, m, td, tp, newTabs);
				break;

			// Always Integer Enum
			case BASICTYPE_ENUMERATED: /* 8 */
				PrintOpenApiEnum(src, mods, m, td, tp, newTabs);
				break;

			// Objects
			case BASICTYPE_SEQUENCE: /* 9 */
			case BASICTYPE_SET:		 /* 11 */
				PrintOpenApiSequence(src, mods, m, td, tp, newTabs);
				break;

			// Arrays
			case BASICTYPE_SEQUENCEOF: /* 10 */
			case BASICTYPE_SETOF:	   /* 12 */
				PrintOpenApiSequenceOf(src, mods, m, td, tp, newTabs);
				break;

			// OneOf
			case BASICTYPE_CHOICE: /* 13 */
				PrintOpenApiChoice(src, mods, m, td, tp, newTabs);
				break;

			// Refs
			case BASICTYPE_LOCALTYPEREF:  /* 18 */
			case BASICTYPE_IMPORTTYPEREF: /* 19 */
				PrintOpenApiRef(src, mods, m, td, tp, newTabs);
				break;

			// String
			case BASICTYPE_NUMERIC_STR:		 /* 22 */
			case BASICTYPE_PRINTABLE_STR:	 /* 23 */
			case BASICTYPE_UNIVERSAL_STR:	 /* 24 */
			case BASICTYPE_IA5_STR:			 /* 25 */
			case BASICTYPE_BMP_STR:			 /* 26 */
			case BASICTYPE_UTF8_STR:		 /* 27 */
			case BASICTYPE_UTCTIME:			 /* 28 */
			case BASICTYPE_GENERALIZEDTIME:	 /* 29 */
			case BASICTYPE_GRAPHIC_STR:		 /* 30 */
			case BASICTYPE_VISIBLE_STR:		 /* 31 aka ISO646String */
			case BASICTYPE_GENERAL_STR:		 /* 32 */
			case BASICTYPE_OBJECTDESCRIPTOR: /* 33 */
			case BASICTYPE_VIDEOTEX_STR:	 /* 34 */
			case BASICTYPE_T61_STR:			 /* 35 */
			case BASICTYPE_OID:				 /* 6 */
			case BASICTYPE_BITSTRING:		 /* 3 */
				PrintOpenApiStringType(src, mods, m, td, tp, newTabs);
				break;

			// Date
			case BASICTYPE_ASNSYSTEMTIME: /* estos special where we encode a time into a real value based on VariantTime */
				PrintOpenApiTimeType(src, mods, m, td, tp, newTabs);
				break;

			// Unsupported
			case BASICTYPE_EXTERNAL:			 /* 36 */
			case BASICTYPE_OCTETCONTAINING:		 /* 37 */
			case BASICTYPE_BITCONTAINING:		 /* 38 */
			case BASICTYPE_RELATIVE_OID:		 /* 39 JKG: added 11/Jan/2004 */
			case BASICTYPE_EXTENSION:			 /*40 JKG:  added 21/Jan/2004 */
			case BASICTYPE_SEQUENCET:			 /* 41 Deepak: read Note above */
			case BASICTYPE_OBJECTCLASS:			 /* 42 Deepak: added on 11/Dec/2002 */
			case BASICTYPE_OBJECTCLASSFIELDTYPE: /* 43 Deepak: added on 04/Feb/2003 */
			case BASICTYPE_MACROTYPE:			 /* 20 */
			case BASICTYPE_MACRODEF:			 /* 21 */
			case BASICTYPE_ANYDEFINEDBY:		 /* 17 */
			case BASICTYPE_SELECTION:			 /* 14 */
			case BASICTYPE_COMPONENTSOF:		 /* 15 */
				fprintf(src, "// [UNSUPPORTED TYPE]\n");
				break;
			default:
				fprintf(src, "// [UNKNOWN TYPE]\n");
				/* TBD: print error? */
				break;
		}

} /* PrintCxxTypeDefCode */

int GetOpenApiROSEDetails(ValueDef* vd, char** ppszArgument, char** ppszResult, char** ppszError, Type** argumentType, Type** resultType, Type** errorType)
{
	int bRetval = 0;
	if (vd->value->type != NULL)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (vd->value->type->basicType->a.macroType->choiceId == MACROTYPE_ROSOPERATION)
			{
				if (vd->value->type->basicType->a.macroType->a.rosOperation->arguments &&
					((vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF && vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.localTypeRef->typeName) || (vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.importTypeRef->typeName)))
				{
					// there is an argument
					if (vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF && vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.localTypeRef->typeName)
					{
						*ppszArgument = vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.localTypeRef->typeName;
						if (argumentType != NULL)
							*argumentType = vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type;
					}
					else if (vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.importTypeRef->typeName)
					{
						*ppszArgument = vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.importTypeRef->typeName;
						if (argumentType != NULL)
							*argumentType = vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type;
					}

					*ppszResult = NULL;
					*ppszError = NULL;

					if (vd->value->type->basicType->a.macroType->a.rosOperation->result)
					{
						if (vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF && vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->a.localTypeRef->typeName)
						{
							*ppszResult = vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->a.localTypeRef->typeName;
							if (resultType != NULL)
								*resultType = vd->value->type->basicType->a.macroType->a.rosOperation->result->type;
						}
						else if (vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->a.importTypeRef->typeName)
						{
							*ppszResult = vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->a.importTypeRef->typeName;
							if (resultType != NULL)
								*resultType = vd->value->type->basicType->a.macroType->a.rosOperation->result->type;
						}
					}

					if (vd->value->type->basicType->a.macroType->a.rosOperation->errors && vd->value->type->basicType->a.macroType->a.rosOperation->errors->count)
					{
						TypeOrValue* first = (TypeOrValue*)FIRST_LIST_ELMT(vd->value->type->basicType->a.macroType->a.rosOperation->errors);
						if (first->choiceId == TYPEORVALUE_TYPE)
						{
							if (first->a.type->basicType->choiceId == BASICTYPE_LOCALTYPEREF && first->a.type->basicType->a.localTypeRef->typeName)
							{
								// local defined
								*ppszError = first->a.type->basicType->a.localTypeRef->typeName;
								if (errorType != NULL)
									*errorType = first->a.type->basicType->a.localTypeRef->link->type;
							}
							else if (first->a.type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && first->a.type->basicType->a.importTypeRef->typeName)
							{
								// imported
								*ppszError = first->a.type->basicType->a.importTypeRef->typeName;
								if (errorType != NULL)
									*errorType = first->a.type->basicType->a.importTypeRef->link->type;
							}
						}
					}
					bRetval = 1;
				}
			}
		}
	}
	return bRetval;
}

static int PrintOpenApiOperation(FILE* src, Module* mod, ValueDef* vd)
{
	char* pszArgument = NULL;
	char* pszResult = NULL;
	char* pszError = NULL;

	if (IsDeprecatedNoOutputOperation(mod, vd->definedName))
		return 1;

	// Optionally add check for Private Operations and dont show it

	if (GetOpenApiROSEDetails(vd, &pszArgument, &pszResult, &pszError, NULL, NULL, NULL))
	{
		fprintf(src, "\t\t\"/%s\": {\n", vd->definedName);
		fprintf(src, "\t\t\t\"post\": {\n");

		asnoperationcomment comment;
		if (GetOperationComment_UTF8(mod->moduleName, vd->definedName, &comment))
		{
			fprintf(src, "\t\t\t\t\"tags\": [\"%s\"],\n", comment.szCategory);

			const char* orig[] = {"\\n", "\\t", 0};
			const char* rep[] = {"\\n\\n", "", 0};
			if (strlen(comment.szShort) != 0)
			{

				char* szShort = replace_multi(comment.szShort, &orig[0], &rep[0]);
				if (szShort != NULL)
				{
					fprintf(src, "\t\t\t\t\"summary\": \"%s\",\n", szShort);
					free(szShort);
				}
			}
			if (strlen(comment.szLong) != 0)
			{
				char* szLong = replace_multi(comment.szLong, &orig[0], &rep[0]);
				if (szLong != NULL)
				{
					fprintf(src, "\t\t\t\t\"description\": \"%s\",\n", szLong);
					free(szLong);
				}
			}
			fprintf(src, "\t\t\t\t\"deprecated\": %s,\n", comment.i64Deprecated ? "true" : "false");
			int opid = vd->value->basicValue->a.integer;
			fprintf(src, "\t\t\t\t\"operationId\": \"%d\",\n", opid);
		}

		fprintf(src, "\t\t\t\t\"requestBody\": {\n");
		fprintf(src, "\t\t\t\t\t\"content\": {\n");
		fprintf(src, "\t\t\t\t\t\t\"application/json\": {\n");
		fprintf(src, "\t\t\t\t\t\t\t\"schema\": {\n");
		fprintf(src, "\t\t\t\t\t\t\t\t\"$ref\": \"#/components/schemas/%s\"\n", pszArgument);
		fprintf(src, "\t\t\t\t\t\t\t}\n");
		fprintf(src, "\t\t\t\t\t\t}\n");
		fprintf(src, "\t\t\t\t\t}\n");
		fprintf(src, "\t\t\t\t}");
		if (pszResult)
		{
			fprintf(src, ",\n");
			fprintf(src, "\t\t\t\t\"responses\": {\n");
			fprintf(src, "\t\t\t\t\t\"200\": {\n");
			fprintf(src, "\t\t\t\t\t\t\"description\": \"Response\",\n");
			fprintf(src, "\t\t\t\t\t\t\"content\": {\n");
			fprintf(src, "\t\t\t\t\t\t\t\"application/json\": {\n");
			fprintf(src, "\t\t\t\t\t\t\t\t\"schema\": {\n");
			fprintf(src, "\t\t\t\t\t\t\t\t\t\"$ref\": \"#/components/schemas/%s\"\n", pszResult);
			fprintf(src, "\t\t\t\t\t\t\t\t}\n");
			fprintf(src, "\t\t\t\t\t\t\t}\n");
			fprintf(src, "\t\t\t\t\t\t}\n");
			fprintf(src, "\t\t\t\t\t}");
			if (pszError)
			{
				fprintf(src, ",\n");
				fprintf(src, "\t\t\t\t\t\"500\": {\n");
				fprintf(src, "\t\t\t\t\t\t\"description\": \"Error\",\n");
				fprintf(src, "\t\t\t\t\t\t\"content\": {\n");
				fprintf(src, "\t\t\t\t\t\t\t\"application/json\": {\n");
				fprintf(src, "\t\t\t\t\t\t\t\t\"schema\": {\n");
				fprintf(src, "\t\t\t\t\t\t\t\t\t\"$ref\": \"#/components/schemas/%s\"\n", pszError);
				fprintf(src, "\t\t\t\t\t\t\t\t}\n");
				fprintf(src, "\t\t\t\t\t\t\t}\n");
				fprintf(src, "\t\t\t\t\t\t}\n");
			}
			else
			{
				fprintf(src, "\n");
			}
			fprintf(src, "\t\t\t\t\t}\n");
			fprintf(src, "\t\t\t\t}\n");
		}
		else
		{
			fprintf(src, "\n");
		}

		fprintf(src, "\t\t\t}\n");
		fprintf(src, "\t\t}");
		return 0;
	}
	return 1;
}
/*
 * prints PrintOpenApiPaths
 */
static void PrintOpenApiPaths(FILE* src, Module* m)
{
	/*
	hdr = hdr;
	vd = vd;
	bEvents = bEvents;
	m = m;
	*/
	ValueDef* vd;

	fprintf(src, "\t\"paths\": {\n");

	// Now generate the invoke messages
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (PrintOpenApiOperation(src, m, vd) == 0)
			{
				if (m->valueDefs->curr->next)
					fprintf(src, ",\n");
				else
					fprintf(src, "\n");
			}
		}
	}

	fprintf(src, "\t},\n");

} /* PrintOpenApiPaths */

void PrintOpenApiInfo(FILE* src, ModuleList* mods, Module* m)
{
	fprintf(src, "\t\"info\": {\n");

	fprintf(src, "\t\t\"title\": \"%s\"", m->moduleName);

	asnmodulecomment comment;
	if (GetModuleComment_UTF8(m->moduleName, &comment))
	{
		fprintf(src, ",\n");
		fprintf(src, "\t\t\"version\": \"0.0.1\",\n");
		const char* orig[] = {"\\n", "\\t", 0};
		const char* rep[] = {"\\n\\n", "", 0};
		if (strlen(comment.szShort) != 0)
		{

			char* szShort = replace_multi(comment.szShort, &orig[0], &rep[0]);
			if (szShort != NULL)
			{
				fprintf(src, "\t\t\"summary\": \"%s\",\n", szShort);
				free(szShort);
			}
		}
		if (strlen(comment.szLong) != 0)
		{
			char* szLong = replace_multi(comment.szLong, &orig[0], &rep[0]);
			if (szLong != NULL)
			{
				fprintf(src, "\t\t\"description\": \"%s\"\n", szLong);
				free(szLong);
			}
		}
		// Cant be used in OpenApi for the module info
		/*
		fprintf(src, "\t\t\"category\": \"%s\",\n", comment.szCategory);
		fprintf(src, "\t\t\"deprecated\": %d,\n", comment.i64Deprecated ? 1 : 0);
		if (comment.i64Deprecated > 1)
			fprintf(src, "\t\t\"deprecated_timestamp\": %lld,\n", comment.i64Deprecated);
		fprintf(src, "\t\t\"private\": %d\n", comment.iPrivate);
		*/
	}
	else
		fprintf(src, "\n");

	fprintf(src, "\t},\n");
}

static Module* GetImportModuleRef(char* Impname, ModuleList* mods)
{
	Module* currMod = NULL;
	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		/* Find the import Module in the Modules and
		 * return the header file name
		 */
		if ((strcmp(Impname, currMod->modId->name) == 0))
			break;
	}
	return currMod;
}

bool PrintOpenApiImports(FILE* src, ModuleList* mods, Module* m)
{
	Module* currMod;
	AsnListNode* currModTmp;
	bool printedSomething = false;

	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		if ((strcmp(m->jsFileName, currMod->jsFileName) == 0))
		{
			// Code to see the import module list AND load possible "namespace" refs.
			ImportModuleList* ModLists;
			ImportModule* impMod;

			ModLists = currMod->imports;
			currModTmp = mods->curr; // RWC;
			FOR_EACH_LIST_ELMT(impMod, ModLists)
			{
				ImportElmt* impElmt;

				// fprintf(src, "// Imports from %s\n", impMod->modId->name);

				if (impMod->moduleRef == NULL)
					impMod->moduleRef = GetImportModuleRef(impMod->modId->name, mods);
				if (printedSomething == FALSE)
					fprintf(src, "\n");

				FOR_EACH_LIST_ELMT(impElmt, impMod->importElmts)
				{
					{
						fprintf(src, "\t\t\t\"%s\": {\n", impElmt->name);
						if (!impMod->moduleRef)
						{
							snacc_exit("Invalid parameter, == NULL");
							return TRUE;
						}

						fprintf(src, "\t\t\t\t\"$ref\": \"%s.json", impMod->moduleRef->moduleName);
						fprintf(src, "#/components/schemas/%s\"\n", impElmt->name);
						fprintf(src, "\t\t\t}");
					}
					if (impMod->importElmts->curr->next)
						fprintf(src, ",\n");
				}
				if (ModLists->curr->next)
					fprintf(src, ",\n");
				printedSomething = TRUE;
			}
			mods->curr = currModTmp; // RWC;RESET loop control
		}
	}

	return printedSomething;
}

void PrintOpenApiCode(FILE* src, ModuleList* mods, Module* m, long longJmpVal, int printTypes, int printValues, int printEncoders, int printDecoders, int printTSONEncDec, int novolatilefuncs)
{
	// Top Open
	fprintf(src, "{\n");
	fprintf(src, "\t\"openapi\": \"3.1.0\",\n");

	PrintOpenApiInfo(src, mods, m);

	PrintOpenApiPaths(src, m);

	fprintf(src, "\t\"components\": {\n");
	fprintf(src, "\t\t\"schemas\": {");

	bool importsPrinted = PrintOpenApiImports(src, mods, m);

	bool bIsFirst = true;
	TypeDef* td;
	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		if (IsDeprecatedNoOutputSequence(m, td->definedName))
			if (!bIsFirst)
				continue;
		if (bIsFirst && importsPrinted)
			fprintf(src, ",\n");
		if (bIsFirst && importsPrinted == FALSE)
			fprintf(src, "\n");
		PrintOpenApiTypeDefinitions(src, mods, m, td, td->type, "\t\t");
		bIsFirst = false;
		if (m->typeDefs->curr->next)
			fprintf(src, ",\n");
		else
			fprintf(src, "\n");
	}
	fprintf(src, "\t\t}\n");
	fprintf(src, "\t}\n");
	fprintf(src, "}\n");

} /* PrintJsonDocCode */

/* EOF gen-code.c (for back-ends/TS-gen) */
