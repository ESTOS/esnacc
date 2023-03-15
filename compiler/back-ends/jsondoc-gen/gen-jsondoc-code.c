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
#include "../str-util.h"

static Module *GetImportModuleRef(char *Impname, ModuleList *mods)
{
	Module *currMod = NULL;
	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		/* Find the import Module in the Modules and
		* return the header file name
		*/
		if ((strcmp(Impname, currMod->modId->name) == 0))
		{
			break;
		}
	}
	return currMod;
}


static void PrintJsonDocNativeType(FILE *hdr, int basicTypeChoiseId) {
	switch (basicTypeChoiseId) {
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
		fprintf(hdr, "number"); //FIXME
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

static void PrintJsonDocType(FILE *hdr, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *t)
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

		else if (strcmp(t->cxxTypeRefInfo->className, "UTF8StringList") == 0 || strcmp(t->cxxTypeRefInfo->className, "SEQInteger") == 0 || strcmp(t->cxxTypeRefInfo->className, "AsnContactIDs") == 0) {
			// fprintf(hdr, "[]");
		}
		else
		{
			//fprintf(hdr, "[Listtype]");
			// fprintf(hdr, "', // type: %s", t->cxxTypeRefInfo->className);
		}
		fprintf(hdr, "%s", t->cxxTypeRefInfo->className);
	}
	else if (t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED)
	{
		fprintf(hdr, "%s", t->cxxTypeRefInfo->className);
		//fprintf(hdr, "[ENUMERATED] ");
		// fprintf(hdr, "number");
	}
	else if (t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_CHOICE)
	{
		// fprintf(hdr, "[BASICTYPE_CHOICE] %s", t->cxxTypeRefInfo->className);
		fprintf(hdr, "%s", t->cxxTypeRefInfo->className);
	}
	else
	{
		// fprintf(hdr, "[BASIC TYPE] ");
		switch (t->basicType->choiceId) {
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
			{
				fprintf(hdr, "string"); // AsnSystemTime ist im Asn1-file als REAL definiert, wird aber im TS als String �bermittelt.
			}
			else if (strcmp(t->cxxTypeRefInfo->className, "AsnContactID") == 0)
			{
				fprintf(hdr, "string");
			}
			else
			{
				fprintf(hdr, "%s", t->cxxTypeRefInfo->className);
			}
			break;
		default:
			fprintf(hdr, "[UNKNOWN BASIC TYPE] %s", t->cxxTypeRefInfo->className);
			break;
		}
	}
} /* PrintCxxType */


static void PrintJsonDocBitstringDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *enumerated, int novolatilefuncs)
{
	//	NamedType *e;
	//	enum BasicTypeChoiceId tmpTypeId;
	CNamedElmt *n;

	fprintf(src, "\t\t\t\"type\" : \"bitstring\",\n");
	fprintf(src, "\t\t\t\"values\" : [\n");
	if (HasNamedElmts(td->type) != 0) {
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			//fprintf(src, "\t\t\t\t\"%s\" : %d", n->name, 0x00000001 << n->value);
			//if (((td->type->cxxTypeRefInfo->namedElmts)->curr->next && ((td->type->cxxTypeRefInfo->namedElmts)->curr->next->data) != NULL))
			//	fprintf(src, ",");
			//fprintf(src, "\n");
			fprintf(src, "\t\t\t\t{\n");
			fprintf(src, "\t\t\t\t\t\"name\" : \"%s\",\n", n->name);
			//fprintf(src, "\t\t\t\t\t\"typeName\" : \"number\",\n"); bitstring enth�lt nur namen, text und zahl(=value)
			asnmembercomment comment;
			if (GetMemberComment_UTF8(td->definedName, n->name, &comment))
			{
				fprintf(src, "\t\t\t\t\t\"short\" : \"%s\",\n", comment.szShort);
				fprintf(src, "\t\t\t\t\t\"deprecated\" : %d,\n", comment.iDeprecated);
				fprintf(src, "\t\t\t\t\t\"private\" : %d,\n", comment.iPrivate);
			}

			fprintf(src, "\t\t\t\t\t\"value\" : %d\n", n->value);

			fprintf(src, "\t\t\t\t}");

			//fprintf(src, "\t\t\t\t\t\"%s\" : %d", n->name, n->value);
			if (((td->type->cxxTypeRefInfo->namedElmts)->curr->next && ((td->type->cxxTypeRefInfo->namedElmts)->curr->next->data) != NULL))
				fprintf(src, ",");
			fprintf(src, "\n");
		}
	}
	fprintf(src, "\t\t\t]\n");
} /* PrintJsonDocBitstringDefCode */

static void PrintJsonDocEnumDefCode(FILE *src, ModuleList *mods, Module *m,
	TypeDef *td, Type *parent, Type *enumerated, int novolatilefuncs)
{
	//	NamedType *e;
	//	enum BasicTypeChoiceId tmpTypeId;
	CNamedElmt *n;

	fprintf(src, "\t\t\t\"type\" : \"enum\",\n");
	fprintf(src, "\t\t\t\"values\" : [\n");
	if (HasNamedElmts(td->type) != 0) {
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			fprintf(src, "\t\t\t\t{\n");
			fprintf(src, "\t\t\t\t\t\"name\" : \"%s\",\n", n->name);
			fprintf(src, "\t\t\t\t\t\"typeName\" : \"number\",\n");
			asnmembercomment comment;
			if (GetMemberComment_UTF8(td->definedName, n->name, &comment))
			{
				fprintf(src, "\t\t\t\t\t\"short\" : \"%s\",\n", comment.szShort);
				fprintf(src, "\t\t\t\t\t\"deprecated\" : %d,\n", comment.iDeprecated);
				fprintf(src, "\t\t\t\t\t\"private\" : %d,\n", comment.iPrivate);
			}

			fprintf(src, "\t\t\t\t\t\"value\" : %d\n", n->value);

			fprintf(src, "\t\t\t\t}");

			//fprintf(src, "\t\t\t\t\t\"%s\" : %d", n->name, n->value);
			if (((td->type->cxxTypeRefInfo->namedElmts)->curr->next && ((td->type->cxxTypeRefInfo->namedElmts)->curr->next->data) != NULL))
				fprintf(src, ",");
			fprintf(src, "\n");

		}
	}
	fprintf(src, "\t\t\t]\n");
} /* PrintJsonDocEnumDefCode */

static void PrintJsonDocChoiceDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *choice, int novolatilefuncs)
{
	NamedType *e;

	fprintf(src, "\t\t\t\"type\" : \"choice\",\n");
	fprintf(src, "\t\t\t\"values\" : [\n");

	FOR_EACH_LIST_ELMT(e, choice->basicType->a.sequence)
	{
		fprintf(src, "\t\t\t\t{\n");

		fprintf(src, "\t\t\t\t\t\"name\" : \"%s\",\n", e->fieldName);

		asnmembercomment comment;
		if (GetMemberComment_UTF8(td->definedName, e->fieldName, &comment))
		{
			fprintf(src, "\t\t\t\t\t\"short\" : \"%s\",\n", comment.szShort);
			fprintf(src, "\t\t\t\t\t\"deprecated\" : %d,\n", comment.iDeprecated);
			fprintf(src, "\t\t\t\t\t\"private\" : %d,\n", comment.iPrivate);
		}

		fprintf(src, "\t\t\t\t\t\"typeName\" : \"");
		PrintJsonDocType(src, mods, m, td, choice, e->type);
		fprintf(src, "\"\n");

		//choice is never optional
		//fprintf(src, "\t\t\t\t\t\"optional\" : %d\n", choice->optional ? 1 : 0);

		fprintf(src, "\t\t\t\t}");
		if (choice->basicType->a.sequence->curr->next)
			fprintf(src, ",");
		fprintf(src, "\n");
	}

	fprintf(src, "\t\t\t]\n");
} /* PrintJsonDocChoiceDefCode */

static void PrintJsonDocSeqDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *seq, int novolatilefuncs)
{
	NamedType *e;


	fprintf(src, "\t\t\t\"type\" : \"sequence\",\n");
	fprintf(src, "\t\t\t\"values\" : [\n");

	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
		{
			fprintf(src, "\t\t\t\t{\n");

			fprintf(src, "\t\t\t\t\t\"name\" : \"%s\",\n", e->fieldName);
			fprintf(src, "\t\t\t\t\t\"typeName\" : \"");
			PrintJsonDocType(src, mods, m, td, seq, e->type);
			fprintf(src, "\",\n");



			asnmembercomment comment;
			if (GetMemberComment_UTF8(td->definedName, e->fieldName, &comment))
			{
				fprintf(src, "\t\t\t\t\t\"short\" : \"%s\",\n", comment.szShort);
				if (comment.szLinkedType[0])
					fprintf(src, "\t\t\t\t\t\"linkedType\" : \"%s\",\n", comment.szLinkedType);
				fprintf(src, "\t\t\t\t\t\"deprecated\" : %d,\n", comment.iDeprecated);
				fprintf(src, "\t\t\t\t\t\"private\" : %d,\n", comment.iPrivate);
			}
			else
			{
				//error
				//int xxx = 0;
			}

			//default value ??
			//PrintJsonDocDefaultValue(src, mods, m, td, seq, e->type);

			//if (e->type->optional)
			fprintf(src, "\t\t\t\t\t\"optional\" : %d\n", e->type->optional ? 1 : 0);


			fprintf(src, "\t\t\t\t}");
			if (seq->basicType->a.sequence->curr->next)
			{
				NamedType* enext = (NamedType*)seq->basicType->a.sequence->curr->next->data;
				if (enext->type->basicType->choiceId != BASICTYPE_EXTENSION)
					fprintf(src, ",");
			}
			fprintf(src, "\n");
		}
	}

	fprintf(src, "\t\t\t]\n");

	// OptionalParams ...
	/*
	if (e->type->basicType->choiceId == BASICTYPE_EXTENSION) {
		continue;
	}
	*/
} /* PrintCxxSeqDefCode */

static void PrintJsonDocListClass(FILE *src, TypeDef *td, Type *lst, Module* m, ModuleList *mods)
{
	struct NamedType p_etemp;
	NamedType* p_e;
	p_e = &p_etemp;
	p_e->type = lst->basicType->a.setOf;

	switch (lst->basicType->a.setOf->basicType->choiceId) {
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

static void PrintJsonDocSetOfDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *setOf, int novolatilefuncs)
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

static void PrintJsonDocimpleDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *setOf, int novolatilefuncs)
{
	fprintf(src, "\t\t\t\"type\" : \"");
	PrintJsonDocNativeType(src, td->type->basicType->choiceId);
	fprintf(src, "\"\n");
}

static void PrintJsonDocTypeDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, int novolatilefuncs)
{
	fprintf(src, "\t\t{\n");
	fprintf(src, "\t\t\t\"typeName\" : \"%s\",\n", td->definedName);

	asnsequencecomment comment;
	if (GetSequenceComment_UTF8(td->definedName, &comment))
	{
		fprintf(src, "\t\t\t\"category\" : \"%s\",\n", comment.szCategory);
		fprintf(src, "\t\t\t\"short\" : \"%s\",\n", comment.szShort);
		fprintf(src, "\t\t\t\"long\" : \"%s\",\n", comment.szLong);
		fprintf(src, "\t\t\t\"private\" : %d,\n", comment.iPrivate);
		fprintf(src, "\t\t\t\"deprecated\" : \"%d\",\n", comment.iDeprecated);
	}


	switch (td->type->basicType->choiceId)
	{
	case BASICTYPE_BOOLEAN:  /* library type */
	case BASICTYPE_REAL:  /* library type */
	case BASICTYPE_OCTETSTRING:  /* library type */
	case BASICTYPE_OCTETCONTAINING:
	case BASICTYPE_NULL:  /* library type */
	case BASICTYPE_EXTERNAL:		/* library type */
	case BASICTYPE_OID:  /* library type */
	case BASICTYPE_RELATIVE_OID:
	case BASICTYPE_INTEGER:  /* library type */
	case BASICTYPE_NUMERIC_STR:  /* 22 */
	case BASICTYPE_PRINTABLE_STR: /* 23 */
	case BASICTYPE_UNIVERSAL_STR: /* 24 */
	case BASICTYPE_IA5_STR:      /* 25 */
	case BASICTYPE_BMP_STR:      /* 26 */
	case BASICTYPE_UTF8_STR:     /* 27 */
	case BASICTYPE_UTCTIME:      /* 28 tag 23 */
	case BASICTYPE_GENERALIZEDTIME: /* 29 tag 24 */
	case BASICTYPE_GRAPHIC_STR:     /* 30 tag 25 */
	case BASICTYPE_VISIBLE_STR:     /* 31 tag 26  aka ISO646String */
	case BASICTYPE_GENERAL_STR:     /* 32 tag 27 */
	case BASICTYPE_OBJECTDESCRIPTOR:	/* 33 tag 7 */
	case BASICTYPE_VIDEOTEX_STR:	/* 34 tag 21 */
	case BASICTYPE_T61_STR:			/* 35 tag 20 */
		PrintJsonDocimpleDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
		//fprintf(src, "// [SIMPLEDEF]\n");
		break;
	case BASICTYPE_BITSTRING:  /* library type */
		PrintJsonDocBitstringDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_SEQUENCEOF:  /* list types */
	case BASICTYPE_SETOF:
		// fprintf(src, "// [BASICTYPE_SEQUENCEOF/BASICTYPE_SETOF]\n");
		PrintJsonDocSetOfDefCode (src, mods, m, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_IMPORTTYPEREF:  /* type references */

		fprintf(src, "\t\t\t\"type\" : \"%s\",\n", "alias");
		fprintf(src, "\t\t\t\"alias\" : \"%s\"\n", td->type->basicType->a.importTypeRef->typeName);

		break;
	case BASICTYPE_LOCALTYPEREF:

		fprintf(src, "\t\t\t\"type\" : \"%s\",\n", "alias");
		fprintf(src, "\t\t\t\"alias\" : \"%s\"\n", td->type->basicType->a.localTypeRef->typeName);

		break;
	case BASICTYPE_ANYDEFINEDBY:  /* ANY types */
	case BASICTYPE_ANY:
		fprintf(src, "// [BASICTYPE_ANY]\n");
		//PrintCxxAnyDefCode (src, hdr, mods, m, r, td, NULL, td->type);
		break;
	case BASICTYPE_CHOICE:
		PrintJsonDocChoiceDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_ENUMERATED:  /* library type */
		PrintJsonDocEnumDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_SET:
		fprintf(src, "// [BASICTYPE_SET]\n");
		//PrintCxxSetDefCode (src, hdr, mods, m, r, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_SEQUENCE:
		PrintJsonDocSeqDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_COMPONENTSOF:
	case BASICTYPE_SELECTION:
	case BASICTYPE_UNKNOWN:
	case BASICTYPE_MACRODEF:
	case BASICTYPE_MACROTYPE:
		fprintf(src, "// [SWITCH DO NOTHING]\n");
		/* do nothing */
		break;
	default:
		fprintf(src, "// [UNKNOWN TYPE]\n");
		/* TBD: print error? */
		break;
	}

	fprintf(src, "\t\t}");
} /* PrintCxxTypeDefCode */

int GetJsonROSEDetails(ValueDef *vd, char** ppszArgument, char** ppszResult, char** ppszError, Type** argumentType, Type** resultType, Type** errorType)
{
	int bRetval = 0;
	if (vd->value->type != NULL)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (vd->value->type->basicType->a.macroType->choiceId == MACROTYPE_ROSOPERATION)
			{
				if (vd->value->type->basicType->a.macroType->a.rosOperation->arguments &&
					((vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF &&
						vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.localTypeRef->typeName) ||
						(vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF &&
							vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.importTypeRef->typeName)
						))
				{
					//there is an argument
					if (vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF &&
						vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.localTypeRef->typeName)
					{
						*ppszArgument = vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.localTypeRef->typeName;
						if (argumentType != NULL) *argumentType = vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type;
					}
					else if (vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF &&
						vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.importTypeRef->typeName)
					{
						*ppszArgument = vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type->basicType->a.importTypeRef->typeName;
						if (argumentType != NULL) *argumentType = vd->value->type->basicType->a.macroType->a.rosOperation->arguments->type;
					}


					*ppszResult = NULL;
					*ppszError = NULL;

					if (vd->value->type->basicType->a.macroType->a.rosOperation->result)
					{
						if (vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF &&
							vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->a.localTypeRef->typeName)
						{
							*ppszResult = vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->a.localTypeRef->typeName;
							if (resultType != NULL) *resultType = vd->value->type->basicType->a.macroType->a.rosOperation->result->type;
						}
						else if (vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF &&
							vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->a.importTypeRef->typeName)
						{
							*ppszResult = vd->value->type->basicType->a.macroType->a.rosOperation->result->type->basicType->a.importTypeRef->typeName;
							if (resultType != NULL) *resultType = vd->value->type->basicType->a.macroType->a.rosOperation->result->type;
						}
					}

					if (vd->value->type->basicType->a.macroType->a.rosOperation->errors &&
						vd->value->type->basicType->a.macroType->a.rosOperation->errors->count)
					{
						TypeOrValue *first = (TypeOrValue*)FIRST_LIST_ELMT(vd->value->type->basicType->a.macroType->a.rosOperation->errors);
						if (first->choiceId == TYPEORVALUE_TYPE)
						{
							if (first->a.type->basicType->choiceId == BASICTYPE_LOCALTYPEREF &&
								first->a.type->basicType->a.localTypeRef->typeName)
							{
								//local defined
								*ppszError = first->a.type->basicType->a.localTypeRef->typeName;
								if (errorType != NULL) {
									*errorType = first->a.type->basicType->a.localTypeRef->link->type;
								}
							}
							else if (first->a.type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF &&
								first->a.type->basicType->a.importTypeRef->typeName)
							{
								//imported
								*ppszError = first->a.type->basicType->a.importTypeRef->typeName;
								if (errorType != NULL) {
									*errorType = first->a.type->basicType->a.importTypeRef->link->type;
								}
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


static int PrintJsonDocOperation(FILE *src, ValueDef *vd)
{
	char* pszArgument = NULL;
	char* pszResult = NULL;
	char* pszError = NULL;

	if (GetJsonROSEDetails(vd, &pszArgument, &pszResult, &pszError, NULL, NULL, NULL))
	{
		fprintf(src, "\t\t{\n");
		fprintf(src, "\t\t\t\"typeName\" : \"%s\",\n", vd->definedName);

		asnoperationcomment comment;
		if (GetOperationComment_UTF8(vd->definedName, &comment))
		{
			fprintf(src, "\t\t\t\"category\" : \"%s\",\n", comment.szCategory);
			if (comment.szShort)
				fprintf(src, "\t\t\t\"short\" : \"%s\",\n", comment.szShort);
			if (comment.szLong)
				fprintf(src, "\t\t\t\"long\" : \"%s\",\n", comment.szLong);
			fprintf(src, "\t\t\t\"private\" : %d,\n", comment.iPrivate);
			fprintf(src, "\t\t\t\"deprecated\" : %d,\n", comment.iDeprecated);
		}


		fprintf(src, "\t\t\t\"arg\" : \"%s\"", pszArgument);
		if (pszResult)
		{
			fprintf(src, ",\n");
			fprintf(src, "\t\t\t\"res\" : \"%s\"", pszResult);

			if (pszError) {
				fprintf(src, ",\n");
				fprintf(src, "\t\t\t\"err\" : \"%s\"\n", pszError);
			}
			else {
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
static void PrintJsonDocOperations(FILE *src, Module *m)
{
	/*
	hdr = hdr;
	vd = vd;
	bEvents = bEvents;
	m = m;
	*/
	ValueDef *vd;

	fprintf(src, "\t\"operations\": [\n");

	//Now generate the invoke messages
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (PrintJsonDocOperation(src, vd) == 0)
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

void PrintJsonDocModule(FILE *src, ModuleList *mods, Module *m)
{
	fprintf(src, "\t\"module\": {\n");

	char* szModName = MakeFileNameWithoutOutputPath(m->baseFileName, "");
	fprintf(src, "\t\t\"name\": \"%s\"", szModName);

	asnmodulecomment comment;
	if (GetModuleComment_UTF8(szModName, &comment))
	{
		fprintf(src, ",\n");
		fprintf(src, "\t\t\"short\": \"%s\",\n", comment.szShort);
		fprintf(src, "\t\t\"long\": \"%s\",\n", comment.szLong);
		fprintf(src, "\t\t\"category\" : \"%s\",\n", comment.szCategory);
		fprintf(src, "\t\t\"private\": %d,\n", comment.iPrivate);
		fprintf(src, "\t\t\"deprecated\": %d\n", comment.iDeprecated);
	}
	else
		fprintf(src, "\n");

	free(szModName);

	fprintf(src, "\t},\n");
}

void PrintJsonDocImports(FILE *src, ModuleList *mods, Module *m)
{
	Module *currMod;
	AsnListNode *currModTmp;

	fprintf(src, "\t\"imports\": [\n");

	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		if ((strcmp(m->jsFileName, currMod->jsFileName) == 0))
		{
			// Code to see the import module list AND load possible "namespace" refs.
			ImportModuleList *ModLists;
			ImportModule *impMod;

			ModLists = currMod->imports;
			currModTmp = mods->curr;    //RWC;
			FOR_EACH_LIST_ELMT(impMod, ModLists)
			{
				ImportElmt *impElmt;

				//fprintf(src, "// Imports from %s\n", impMod->modId->name);

				if (impMod->moduleRef == NULL)
					impMod->moduleRef = GetImportModuleRef(impMod->modId->name, mods);

				fprintf(src, "\t\t{\n");
				fprintf(src, "\t\t\t\"elements\": [\n");

				FOR_EACH_LIST_ELMT(impElmt, impMod->importElmts)
				{
					/*
					if (strcmp(impElmt->name, "AsnOptionalParameters") == 0
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

				char* szModName = MakeFileNameWithoutOutputPath(impMod->moduleRef->baseFileName, "");
				fprintf(src, "\t\t\t\"module\" : \"%s\"", szModName);
				free(szModName);

				fprintf(src, "\n");
				fprintf(src, "\t\t}");
				if (ModLists->curr->next)
					fprintf(src, ",");
				fprintf(src, "\n");
			}
			mods->curr = currModTmp;    // RWC;RESET loop control
		}
	}
	fprintf(src, "\t],\n");
}

void PrintJsonDocTypeDefinitions(FILE *src, ModuleList *mods, Module *m)
{
	fprintf(src, "\t\"types\": [\n");

	TypeDef *td;
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

void PrintJsonDocCode(FILE *src, ModuleList *mods, Module *m)
{
	//Top Open
	fprintf(src, "{\n");

	PrintJsonDocModule(src, mods, m);

	// Includes
	PrintJsonDocImports(src, mods, m);

	PrintJsonDocOperations(src, m);

	PrintJsonDocTypeDefinitions(src, mods, m);

	fprintf(src, "}\n");

} /* PrintJsonDocCode */

/* EOF gen-code.c (for back-ends/TS-gen) */

