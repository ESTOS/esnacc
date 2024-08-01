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

#include "gen-jsondoc-code.h"
#include "../../core/asn_comments.h"
#include "../../core/time_helpers.h"
#include "../str-util.h"

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

static void PrintJsonDocNativeType(FILE* hdr, int basicTypeChoiseId)
{
	switch (basicTypeChoiseId)
	{
		case BASICTYPE_BOOLEAN:
			fprintf(hdr, "boolean");
			break;
		case BASICTYPE_INTEGER:
			fprintf(hdr, "number");
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(hdr, "string");
			break;
		case BASICTYPE_ENUMERATED:
			fprintf(hdr, "number"); // FIXME
			break;
		case BASICTYPE_REAL:
			fprintf(hdr, "number");
			break;
		case BASICTYPE_UTF8_STR:
			fprintf(hdr, "string");
			break;
		case BASICTYPE_UTCTIME:
			fprintf(hdr, "string");
			break;
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
			fprintf(hdr, "Object");
			break;
		default:
			exit(1);
			break;
	}
}

static void PrintJsonDocType(FILE* hdr, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* t)
{
	// fprintf(hdr, "{type: '");

	// fprintf(hdr, "[PrintJsonDocType] ");
	if (t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_SEQUENCEOF)
	{
		// fprintf(hdr, "[SEQUENCE OF] ");
		if (strcmp(t->cxxTypeRefInfo->className, "AsnOptionalParameters") == 0) // ESTOS special 'native Object' AsnOptionalParamaters
		{
			// fprintf(hdr, "{}"); // AsnOptionalParameters are objects not arrays
		}

		else if (strcmp(t->cxxTypeRefInfo->className, "UTF8StringList") == 0 || strcmp(t->cxxTypeRefInfo->className, "SEQInteger") == 0 || strcmp(t->cxxTypeRefInfo->className, "AsnContactIDs") == 0)
		{
			// fprintf(hdr, "[]");
		}
		else
		{
			// fprintf(hdr, "[Listtype]");
			//  fprintf(hdr, "', // type: %s", t->cxxTypeRefInfo->className);
		}
		fprintf(hdr, "%s", t->cxxTypeRefInfo->className);
	}
	else if (t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED)
	{
		fprintf(hdr, "%s", t->cxxTypeRefInfo->className);
		// fprintf(hdr, "[ENUMERATED] ");
		//  fprintf(hdr, "number");
	}
	else if (t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_CHOICE)
	{
		// fprintf(hdr, "[BASICTYPE_CHOICE] %s", t->cxxTypeRefInfo->className);
		fprintf(hdr, "%s", t->cxxTypeRefInfo->className);
	}
	else
	{
		// fprintf(hdr, "[BASIC TYPE] ");
		switch (t->basicType->choiceId)
		{
			case BASICTYPE_BOOLEAN:
			case BASICTYPE_INTEGER:
			case BASICTYPE_OCTETSTRING:
			case BASICTYPE_OCTETCONTAINING:
			case BASICTYPE_ENUMERATED:
			case BASICTYPE_REAL:
			case BASICTYPE_UTF8_STR:
			case BASICTYPE_UTCTIME:
			case BASICTYPE_UNKNOWN:
			case BASICTYPE_NULL:
				PrintJsonDocNativeType(hdr, t->basicType->choiceId);
				break;
			case BASICTYPE_SEQUENCEOF:
				fprintf(hdr, "%s[]", t->cxxTypeRefInfo->className);
				break;
			case BASICTYPE_EXTENSION:
				fprintf(hdr, "Object");
				break;
			case BASICTYPE_IMPORTTYPEREF:
			case BASICTYPE_LOCALTYPEREF:
				if (strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
					fprintf(hdr, "string"); // AsnSystemTime is defined as REAL in the Asn1 file, but is transmitted as a string in the TS.

				else if (strcmp(t->cxxTypeRefInfo->className, "AsnContactID") == 0)
					fprintf(hdr, "string");
				else
					fprintf(hdr, "%s", t->cxxTypeRefInfo->className);
				break;
			default:
				fprintf(hdr, "[UNKNOWN BASIC TYPE] %s", t->cxxTypeRefInfo->className);
				break;
		}
	}
} /* PrintCxxType */

static void PrintJsonDocBitstringDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* enumerated, int novolatilefuncs)
{
	//	NamedType *e;
	//	enum BasicTypeChoiceId tmpTypeId;
	CNamedElmt* n;

	fprintf(src, "\t\t\t\"type\" : \"bitstring\",\n");
	fprintf(src, "\t\t\t\"values\" : [\n");
	if (HasNamedElmts(td->type) != 0)
	{
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			// fprintf(src, "\t\t\t\t\"%s\" : %d", n->name, 0x00000001 << n->value);
			// if (((td->type->cxxTypeRefInfo->namedElmts)->curr->next && ((td->type->cxxTypeRefInfo->namedElmts)->curr->next->data) != NULL))
			//	fprintf(src, ",");
			// fprintf(src, "\n");
			fprintf(src, "\t\t\t\t{\n");
			fprintf(src, "\t\t\t\t\t\"name\" : \"%s\"", n->name);
			// fprintf(src, "\t\t\t\t\t\"typeName\" : \"number\",\n"); bitstring contains only names, text and numbers(=value)
			asnmembercomment comment;
			if (GetMemberComment_UTF8(m->moduleName, td->definedName, n->name, &comment))
			{
				if (comment.szShort[0])
					fprintf(src, ",\n\t\t\t\t\t\"short\" : \"%s\"", comment.szShort);
				if (comment.i64Deprecated)
					fprintf(src, ",\n\t\t\t\t\t\"deprecated\" : %d", comment.i64Deprecated ? 1 : 0);
				if (comment.i64Deprecated > 1)
					fprintf(src, ",\n\t\t\t\t\t\"deprecated_timestamp\": %lld", comment.i64Deprecated);
				if (comment.szDeprecated[0])
					fprintf(src, ",\n\t\t\t\t\t\"deprecated_comment\": \"%s\"", comment.szDeprecated);
				if (comment.i64Added)
					fprintf(src, ",\n\t\t\t\t\t\"added_timestamp\": %lld", comment.i64Added);
				if (comment.iPrivate)
					fprintf(src, ",\n\t\t\t\t\t\"private\" : %d", comment.iPrivate);
			}

			fprintf(src, ",\n\t\t\t\t\t\"value\" : %d", n->value);

			fprintf(src, "\n\t\t\t\t}");

			// fprintf(src, "\t\t\t\t\t\"%s\" : %d", n->name, n->value);
			if (((td->type->cxxTypeRefInfo->namedElmts)->curr->next && ((td->type->cxxTypeRefInfo->namedElmts)->curr->next->data) != NULL))
				fprintf(src, ",");
			fprintf(src, "\n");
		}
	}
	fprintf(src, "\t\t\t]\n");
} /* PrintJsonDocBitstringDefCode */

static void PrintJsonDocEnumDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* enumerated, int novolatilefuncs)
{
	//	NamedType *e;
	//	enum BasicTypeChoiceId tmpTypeId;
	CNamedElmt* n;

	fprintf(src, ",\n\t\t\t\"type\" : \"enum\"");
	fprintf(src, ",\n\t\t\t\"values\" : [");
	if (HasNamedElmts(td->type) != 0)
	{
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			fprintf(src, "\n\t\t\t\t{");
			fprintf(src, "\n\t\t\t\t\t\"name\" : \"%s\"", n->name);
			fprintf(src, ",\n\t\t\t\t\t\"typeName\" : \"number\"");
			asnmembercomment comment;
			if (GetMemberComment_UTF8(m->moduleName, td->definedName, n->name, &comment))
			{
				if (comment.szShort[0])
					fprintf(src, ",\n\t\t\t\t\t\"short\" : \"%s\"", comment.szShort);
				if (comment.i64Deprecated)
					fprintf(src, ",\n\t\t\t\t\t\"deprecated\" : %d", comment.i64Deprecated ? 1 : 0);
				if (comment.i64Deprecated > 1)
					fprintf(src, ",\n\t\t\t\t\t\"deprecated_timestamp\": %lld", comment.i64Deprecated);
				if (comment.szDeprecated[0])
					fprintf(src, ",\n\t\t\t\t\t\"deprecated_comment\": \"%s\"", comment.szDeprecated);
				if (comment.i64Added)
					fprintf(src, ",\n\t\t\t\t\t\"added_timestamp\": %lld", comment.i64Added);
				if (comment.iPrivate)
					fprintf(src, ",\n\t\t\t\t\t\"private\" : %d", comment.iPrivate);
			}

			fprintf(src, ",\n\t\t\t\t\t\"value\" : %d", n->value);

			fprintf(src, "\n\t\t\t\t}");

			// fprintf(src, "\t\t\t\t\t\"%s\" : %d", n->name, n->value);
			if (((td->type->cxxTypeRefInfo->namedElmts)->curr->next && ((td->type->cxxTypeRefInfo->namedElmts)->curr->next->data) != NULL))
				fprintf(src, ",");
			fprintf(src, "\n");
		}
	}
	fprintf(src, "\t\t\t]\n");
} /* PrintJsonDocEnumDefCode */

static void PrintJsonDocChoiceDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* choice, int novolatilefuncs)
{
	NamedType* e;

	fprintf(src, ",\n\t\t\t\"type\" : \"choice\"");
	fprintf(src, ",\n\t\t\t\"values\" : [\n");

	FOR_EACH_LIST_ELMT(e, choice->basicType->a.sequence)
	{
		fprintf(src, "\t\t\t\t{\n");

		fprintf(src, "\t\t\t\t\t\"name\" : \"%s\"", e->fieldName);

		asnmembercomment comment;
		if (GetMemberComment_UTF8(m->moduleName, td->definedName, e->fieldName, &comment))
		{
			if (comment.szShort[0])
				fprintf(src, ",\n\t\t\t\t\t\"short\" : \"%s\"", comment.szShort);
			if (comment.i64Deprecated)
				fprintf(src, ",\n\t\t\t\t\t\"deprecated\" : %d", comment.i64Deprecated ? 1 : 0);
			if (comment.i64Deprecated > 1)
				fprintf(src, ",\n\t\t\t\t\t\"deprecated_timestamp\": %lld", comment.i64Deprecated);
			if (comment.szDeprecated[0])
				fprintf(src, ",\n\t\t\t\t\t\"deprecated_comment\": \"%s\"", comment.szDeprecated);
			if (comment.i64Added)
				fprintf(src, ",\n\t\t\t\t\t\"added_timestamp\": %lld", comment.i64Added);
			if (comment.iPrivate)
				fprintf(src, ",\n\t\t\t\t\t\"private\" : %d", comment.iPrivate);
		}

		fprintf(src, ",\n\t\t\t\t\t\"typeName\" : \"");
		PrintJsonDocType(src, mods, m, td, choice, e->type);
		fprintf(src, "\"");

		fprintf(src, "\n\t\t\t\t}");
	}

	fprintf(src, "\t\t\t]\n");
} /* PrintJsonDocChoiceDefCode */

static void PrintJsonDocSeqDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* seq, int novolatilefuncs)
{
	NamedType* e;

	fprintf(src, ",\n\t\t\t\"type\" : \"sequence\"");
	fprintf(src, ",\n\t\t\t\"values\" : [");
	bool bHadElements = false;

	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
		{
			bHadElements = true;
			fprintf(src, "\n\t\t\t\t{");

			fprintf(src, "\n\t\t\t\t\t\"name\" : \"%s\"", e->fieldName);
			fprintf(src, ",\n\t\t\t\t\t\"typeName\" : \"");
			PrintJsonDocType(src, mods, m, td, seq, e->type);
			fprintf(src, "\"");

			asnmembercomment comment;
			if (GetMemberComment_UTF8(m->moduleName, td->definedName, e->fieldName, &comment))
			{
				if (comment.szShort[0])
					fprintf(src, ",\n\t\t\t\t\t\"short\" : \"%s\"", comment.szShort);
				if (comment.szLinkedType[0])
					fprintf(src, ",\n\t\t\t\t\t\"linkedType\" : \"%s\"", comment.szLinkedType);
				if(comment.i64Deprecated)
					fprintf(src, ",\n\t\t\t\t\t\"deprecated\" : %d", comment.i64Deprecated ? 1 : 0);
				if (comment.i64Deprecated > 1)
					fprintf(src, ",\n\t\t\t\t\t\"deprecated_timestamp\": %lld", comment.i64Deprecated);
				if (comment.szDeprecated[0])
					fprintf(src, ",\n\t\t\t\t\t\"deprecated_comment\": \"%s\"", comment.szDeprecated);
				if (comment.i64Added)
					fprintf(src, ",\n\t\t\t\t\t\"added_timestamp\": %lld", comment.i64Added);
				if (comment.iPrivate)
					fprintf(src, ",\n\t\t\t\t\t\"private\" : %d", comment.iPrivate);
			}
			else
			{
				// error
				// int xxx = 0;
			}

			// default value ??
			// PrintJsonDocDefaultValue(src, mods, m, td, seq, e->type);

			if (e->type->optional)
				fprintf(src, ",\n\t\t\t\t\t\"optional\" : 1");

			fprintf(src, "\n\t\t\t\t}");
			if (seq->basicType->a.sequence->curr->next)
			{
				NamedType* enext = (NamedType*)seq->basicType->a.sequence->curr->next->data;
				if (enext->type->basicType->choiceId != BASICTYPE_EXTENSION)
					fprintf(src, ",");
			}
		}
	}
	if (bHadElements)
		fprintf(src, "\n\t\t\t");
	fprintf(src, "]\n");

	// OptionalParams ...
	/*
	if (e->type->basicType->choiceId == BASICTYPE_EXTENSION) {
		continue;
	}
	*/
} /* PrintCxxSeqDefCode */

static void PrintJsonDocListClass(FILE* src, TypeDef* td, Type* lst, Module* m, ModuleList* mods)
{
	struct NamedType p_etemp;
	NamedType* p_e;
	p_e = &p_etemp;
	p_e->type = lst->basicType->a.setOf;

	switch (lst->basicType->a.setOf->basicType->choiceId)
	{
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_INTEGER:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_REAL:
		case BASICTYPE_UTF8_STR:
		case BASICTYPE_UTCTIME:
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
			fprintf(src, "\t\t\t\"type\" : \"array\",\n");
			fprintf(src, "\t\t\t\"elementType\" : \"");
			PrintJsonDocNativeType(src, lst->basicType->a.setOf->basicType->choiceId);
			fprintf(src, "\"\n");

			return;
		default:
			break;
	}

	fprintf(src, "\t\t\t\"type\" : \"array\",\n");
	fprintf(src, "\t\t\t\"elementType\" : \"%s\"\n", p_e->type->cxxTypeRefInfo->className);
}

static void PrintJsonDocSetOfDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* setOf, int novolatilefuncs)
{
	/*
	if (strcmp(td->cxxTypeDefInfo->className, "AsnOptionalParameters") == 0) // ESTOS special 'native Object' AsnOptionalParamaters
	{
		fprintf(src, "export class AsnOptionalParameters extends Object { };\n\n");
		return;
	}
	*/

	/* do class */
	PrintJsonDocListClass(src, td, setOf, m, mods);

} /* PrintJsonDocSetOfDefCode */

static void PrintJsonDocimpleDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* setOf, int novolatilefuncs)
{
	fprintf(src, "\t\t\t\"type\" : \"");
	PrintJsonDocNativeType(src, td->type->basicType->choiceId);
	fprintf(src, "\"\n");
}

static void PrintJsonDocTypeDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs)
{
	fprintf(src, "\t\t{\n");
	fprintf(src, "\t\t\t\"typeName\" : \"%s\"", td->definedName);

	asnsequencecomment comment;
	if (GetSequenceComment_UTF8(m->moduleName, td->definedName, &comment))
	{
		if (comment.szCategory[0])
			fprintf(src, ",\n\t\t\t\"category\" : \"%s\"", comment.szCategory);
		if (comment.szShort[0])
			fprintf(src, ",\n\t\t\t\"short\" : \"%s\"", comment.szShort);
		if (comment.szLong[0])
			fprintf(src, ",\n\t\t\t\"long\" : \"%s\"", comment.szLong);
		if (comment.i64Deprecated)
			fprintf(src, ",\n\t\t\t\"deprecated\" : %d", comment.i64Deprecated ? 1 : 0);
		if (comment.i64Deprecated > 1)
			fprintf(src, ",\n\t\t\t\"deprecated_timestamp\": %lld", comment.i64Deprecated);
		if (comment.szDeprecated[0])
			fprintf(src, ",\n\t\t\t\"deprecated_comment\": \"%s\"", comment.szDeprecated);
		if (comment.i64Added)
			fprintf(src, ",\n\t\t\t\"added_timestamp\": %lld", comment.i64Added);
		if(comment.iPrivate)
			fprintf(src, ",\n\t\t\t\"private\" : %d", comment.iPrivate);
	}

	switch (td->type->basicType->choiceId)
	{
		case BASICTYPE_BOOLEAN:		/* library type */
		case BASICTYPE_REAL:		/* library type */
		case BASICTYPE_OCTETSTRING: /* library type */
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_NULL:	 /* library type */
		case BASICTYPE_EXTERNAL: /* library type */
		case BASICTYPE_OID:		 /* library type */
		case BASICTYPE_RELATIVE_OID:
		case BASICTYPE_INTEGER:			 /* library type */
		case BASICTYPE_NUMERIC_STR:		 /* 22 */
		case BASICTYPE_PRINTABLE_STR:	 /* 23 */
		case BASICTYPE_UNIVERSAL_STR:	 /* 24 */
		case BASICTYPE_IA5_STR:			 /* 25 */
		case BASICTYPE_BMP_STR:			 /* 26 */
		case BASICTYPE_UTF8_STR:		 /* 27 */
		case BASICTYPE_UTCTIME:			 /* 28 tag 23 */
		case BASICTYPE_GENERALIZEDTIME:	 /* 29 tag 24 */
		case BASICTYPE_GRAPHIC_STR:		 /* 30 tag 25 */
		case BASICTYPE_VISIBLE_STR:		 /* 31 tag 26  aka ISO646String */
		case BASICTYPE_GENERAL_STR:		 /* 32 tag 27 */
		case BASICTYPE_OBJECTDESCRIPTOR: /* 33 tag 7 */
		case BASICTYPE_VIDEOTEX_STR:	 /* 34 tag 21 */
		case BASICTYPE_T61_STR:			 /* 35 tag 20 */
			PrintJsonDocimpleDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			// fprintf(src, "// [SIMPLEDEF]\n");
			break;
		case BASICTYPE_BITSTRING: /* library type */
			PrintJsonDocBitstringDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SEQUENCEOF: /* list types */
		case BASICTYPE_SETOF:
			// fprintf(src, "// [BASICTYPE_SEQUENCEOF/BASICTYPE_SETOF]\n");
			PrintJsonDocSetOfDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_IMPORTTYPEREF: /* type references */

			fprintf(src, ",\n\t\t\t\"type\" : \"%s\"", "alias");
			fprintf(src, ",\n\t\t\t\"alias\" : \"%s\"", td->type->basicType->a.importTypeRef->typeName);

			break;
		case BASICTYPE_LOCALTYPEREF:

			fprintf(src, ",\n\t\t\t\"type\" : \"%s\"", "alias");
			fprintf(src, ",\n\t\t\t\"alias\" : \"%s\"", td->type->basicType->a.localTypeRef->typeName);

			break;
		case BASICTYPE_ANYDEFINEDBY: /* ANY types */
		case BASICTYPE_ANY:
			fprintf(src, "\n// [BASICTYPE_ANY]\n");
			// PrintCxxAnyDefCode (src, hdr, mods, m, r, td, NULL, td->type);
			break;
		case BASICTYPE_CHOICE:
			PrintJsonDocChoiceDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_ENUMERATED: /* library type */
			PrintJsonDocEnumDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SET:
			fprintf(src, "\n// [BASICTYPE_SET]\n");
			// PrintCxxSetDefCode (src, hdr, mods, m, r, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SEQUENCE:
			PrintJsonDocSeqDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_COMPONENTSOF:
		case BASICTYPE_SELECTION:
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_MACRODEF:
		case BASICTYPE_MACROTYPE:
			fprintf(src, "\n// [SWITCH DO NOTHING]\n");
			/* do nothing */
			break;
		default:
			fprintf(src, "\n// [UNKNOWN TYPE]\n");
			/* TBD: print error? */
			break;
	}

	fprintf(src, "\t\t}");
} /* PrintCxxTypeDefCode */

int GetJsonROSEDetails(ValueDef* vd, char** ppszArgument, char** ppszResult, char** ppszError, Type** argumentType, Type** resultType, Type** errorType)
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

static int PrintJsonDocOperation(FILE* src, Module* mod, ValueDef* vd)
{
	char* pszArgument = NULL;
	char* pszResult = NULL;
	char* pszError = NULL;

	if (GetJsonROSEDetails(vd, &pszArgument, &pszResult, &pszError, NULL, NULL, NULL))
	{
		fprintf(src, "\t\t{\n");
		fprintf(src, "\t\t\t\"typeName\" : \"%s\",\n", vd->definedName);

		asnoperationcomment comment;
		if (GetOperationComment_UTF8(mod->moduleName, vd->definedName, &comment))
		{
			if (comment.szCategory[0])
				fprintf(src, "\t\t\t\"category\" : \"%s\",\n", comment.szCategory);
			if (comment.szShort[0])
				fprintf(src, "\t\t\t\"short\" : \"%s\",\n", comment.szShort);
			if (comment.szLong[0])
				fprintf(src, "\t\t\t\"long\" : \"%s\",\n", comment.szLong);
			if (comment.i64Deprecated)
				fprintf(src, "\t\t\t\"deprecated\" : %d,\n", comment.i64Deprecated ? 1 : 0);
			if (comment.i64Deprecated > 1)
				fprintf(src, "\t\t\t\"deprecated_timestamp\": %lld,\n", comment.i64Deprecated);
			if (comment.szDeprecated[0])
				fprintf(src, "\t\t\t\"deprecated_comment\": \"%s\",\n", comment.szDeprecated);
			if (comment.i64Added)
				fprintf(src, "\t\t\t\"added_timestamp\": %lld,\n", comment.i64Added);
			if (comment.iPrivate)
				fprintf(src, "\t\t\t\"private\" : %d,\n", comment.iPrivate);
			int opid = vd->value->basicValue->a.integer;
			fprintf(src, "\t\t\t\"operationid\" : %d,\n", opid);
		}

		fprintf(src, "\t\t\t\"arg\" : \"%s\"", pszArgument);
		if (pszResult)
		{
			fprintf(src, ",\n");
			fprintf(src, "\t\t\t\"res\" : \"%s\"", pszResult);

			if (pszError)
			{
				fprintf(src, ",\n");
				fprintf(src, "\t\t\t\"err\" : \"%s\"\n", pszError);
			}
			else
			{
				fprintf(src, "\n");
			}
		}
		else
			fprintf(src, "\n");

		fprintf(src, "\t\t}");
		return 0;
	}
	return 1;
}
/*
 * prints PrintJsonDocOperations
 */
static void PrintJsonDocOperations(FILE* src, Module* m)
{
	/*
	hdr = hdr;
	vd = vd;
	bEvents = bEvents;
	m = m;
	*/
	ValueDef* vd;

	fprintf(src, "\t\"operations\": [\n");

	// Now generate the invoke messages
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (PrintJsonDocOperation(src, m, vd) == 0)
			{
				if (m->valueDefs->curr->next)
					fprintf(src, ",\n");
				else
					fprintf(src, "\n");
			}
		}
	}

	fprintf(src, "\t],\n");

} /* PrintROSEInvoke */

void PrintJsonDocModule(FILE* src, ModuleList* mods, Module* m)
{
	fprintf(src, "\t\"module\": {\n");

	fprintf(src, "\t\t\"name\": \"%s\"", m->moduleName);

	asnmodulecomment comment;
	if (GetModuleComment_UTF8(m->moduleName, &comment))
	{
		if (comment.szCategory[0])
			fprintf(src, ",\n\t\t\"category\" : \"%s\"", comment.szCategory);
		if (comment.szShort[0])
			fprintf(src, ",\n\t\t\"short\": \"%s\"", comment.szShort);
		if (comment.szLong[0])
			fprintf(src, ",\n\t\t\"long\": \"%s\"", comment.szLong);
		if (comment.i64Deprecated)
			fprintf(src, ",\n\t\t\"deprecated\": %d", comment.i64Deprecated ? 1 : 0);
		if (comment.i64Deprecated > 1)
			fprintf(src, ",\n\t\t\"deprecated_timestamp\": %lld", comment.i64Deprecated);
		if (comment.szDeprecated[0])
			fprintf(src, ",\n\t\t\"deprecated_comment\": \"%s\"", comment.szDeprecated);
		if (comment.i64Added)
			fprintf(src, ",\n\t\t\"added_timestamp\": %lld", comment.i64Added);
		if (comment.iPrivate)
			fprintf(src, ",\n\t\t\"private\": %d", comment.iPrivate);
	}
	if (gMajorInterfaceVersion >= 0) {

		fprintf(src, ",\n\t\t\"version\": {");
		long long lMinorModuleVersion = GetModuleMinorVersion(m->moduleName);
		fprintf(src, "\n\t\t\t\"lastChange\": \"%s\"", ConvertUnixTimeToISO(lMinorModuleVersion));
		fprintf(src, ",\n\t\t\t\"majorVersion\": %i", gMajorInterfaceVersion);
		fprintf(src, ",\n\t\t\t\"minorVersion\": %lld", lMinorModuleVersion);
		fprintf(src, ",\n\t\t\t\"version\": \"%i.%lld.0\"", gMajorInterfaceVersion, lMinorModuleVersion);
		fprintf(src, "\n\t\t}");
	}

	fprintf(src, "\n\t},\n");
}

void PrintJsonDocImports(FILE* src, ModuleList* mods, Module* m)
{
	Module* currMod;
	AsnListNode* currModTmp;

	fprintf(src, "\t\"imports\": [\n");

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

				fprintf(src, "\t\t{\n");
				fprintf(src, "\t\t\t\"elements\": [\n");

				FOR_EACH_LIST_ELMT(impElmt, impMod->importElmts)
				{
					/*
					if (strcmp(impElmt->name, "AsnoptionalParameters") == 0
						|| strcmp(impElmt->name, "UTF8StringList") == 0
						|| strcmp(impElmt->name, "SEQInteger") == 0
						|| strcmp(impElmt->name, "AsnContactIDs") == 0
						|| strcmp(impElmt->name, "AsnSystemTime") == 0)
					{
						continue;
					}
					else
					*/
					{
						fprintf(src, "\t\t\t\t\"%s\"", impElmt->name);
					}
					if (impMod->importElmts->curr->next)
						fprintf(src, ",\n");
				}
				fprintf(src, "\n");
				fprintf(src, "\t\t\t],\n");

				if (!impMod->moduleRef)
				{
					snacc_exit("Invalid parameter, == NULL");
					return;
				}
				fprintf(src, "\t\t\t\"module\" : \"%s\"", impMod->moduleRef->moduleName);

				fprintf(src, "\n");
				fprintf(src, "\t\t}");
				if (ModLists->curr->next)
					fprintf(src, ",");
				fprintf(src, "\n");
			}
			mods->curr = currModTmp; // RWC;RESET loop control
		}
	}
	fprintf(src, "\t],\n");
}

void PrintJsonDocTypeDefinitions(FILE* src, ModuleList* mods, Module* m)
{
	fprintf(src, "\t\"types\": [\n");

	TypeDef* td;
	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		PrintJsonDocTypeDefCode(src, mods, m, td, 0);
		if (m->typeDefs->curr->next)
			fprintf(src, ",\n");
		else
			fprintf(src, "\n");
	}

	fprintf(src, "\t]\n");
}

void PrintJsonDocCodeOne(FILE* src, ModuleList* mods, Module* m)
{
	// Top Open
	fprintf(src, "{\n");

	PrintJsonDocModule(src, mods, m);

	PrintJsonDocImports(src, mods, m);

	PrintJsonDocOperations(src, m);

	PrintJsonDocTypeDefinitions(src, mods, m);

	fprintf(src, "}\n");

} /* PrintJsonDocCodeOne */


void PrintJsonDocCode(ModuleList* allMods) {
	Module* currMod;
	AsnListNode* saveMods;
	FILE* srcFilePtr;
	// FILE		*hdrInterfaceFilePtr;
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
		currMod->jsFileName = MakeJsonDocFileName(currMod->baseFilePath);

		if (ObjIsDefined(fNames, currMod->jsFileName, StrObjCmp))
		{
			fprintf(errFileG, "Ack! ERROR---file name conflict for generated swift file with name `%s'.\n\n", currMod->jsFileName);
			fprintf(errFileG, "This usually means the max file name length is truncating the file names.\n");
			fprintf(errFileG, "Try re-naming the modules with shorter names or increasing the argument to -mf option (if you are using it).\n");
			fprintf(errFileG, "This error can also be caused by 2 modules having the same name but different OBJECT IDENTIFIERs.");
			fprintf(errFileG, "Try renaming the modules to correct this.\n");
			fNameConflict = TRUE;
		}
		else
		{
			DefineObj(&fNames, currMod->jsFileName);
		}

		if (fNameConflict)
			return;

		FreeDefinedObjs(&fNames);
	}
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			if (fopen_s(&srcFilePtr, currMod->jsFileName, "wt") != 0 || srcFilePtr == NULL)
			{
				perror("fopen");
			}
			else
			{
				saveMods = allMods->curr;
				PrintJsonDocCodeOne(srcFilePtr, allMods, currMod);
				allMods->curr = saveMods;
				fclose(srcFilePtr);
			}
		}
	}
}

/* EOF gen-code.c (for back-ends/TS-gen) */
