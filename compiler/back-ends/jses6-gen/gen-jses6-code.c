/*
*   compiler/back_ends/JSES6_gen/gen_jses6_code.c - routines for printing JavaScript ES6 code from type trees
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
* 2021 ESTOS/stm
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
*/

#include "gen-jses6-code.h"
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


static void PrintJSES6NativeType(FILE *hdr, int basicTypeChoiseId) {
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
		fprintf(hdr, "object");
		break;
	default:
		exit(1);
		break;
	}
}


static void PrintJSES6DefaultValue(FILE *hdr, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *t)
{
	// fprintf(hdr, "/*[PrintJSES6DefaultValue]*/");

	if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_SEQUENCEOF) ||
		(t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && t->basicType->a.importTypeRef->link != NULL && t->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_SEQUENCEOF))
	{
		if (strcmp(t->cxxTypeRefInfo->className, "AsnOptionalParameters") == 0) // ESTOS special 'native Object' AsnOptionalParamaters
			fprintf(hdr, "{}"); // AsnOptionalParameters are objects not arrays
		else
			fprintf(hdr, "[]");
	}
	else if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED) ||
			 (t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && t->basicType->a.importTypeRef->link != NULL && t->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED))
	{
		//fprintf(hdr, "/*[if(2)]*/");
		// fprintf(hdr, "[ENUMERATED] ");
		CNamedElmt *n;
		Type *enumType = t->basicType->a.localTypeRef->link->type;
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
	else if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_CHOICE) ||
			 (t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && t->basicType->a.importTypeRef->link != NULL && t->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_CHOICE))
	{
		// fprintf(hdr, "[BASICTYPE_CHOICE] %s", t->cxxTypeRefInfo->className);
		fprintf(hdr, "{}");
	}
	else
	{
		//fprintf(hdr, "/*[if(BASIC TYPE)]*/");
		// fprintf(hdr, "[BASIC TYPE] ");
		switch (t->basicType->choiceId)
		{
		case BASICTYPE_BOOLEAN:
			fprintf(hdr, "false");
			break;
		case BASICTYPE_INTEGER:
			fprintf(hdr, "0");
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(hdr, "''");
			break;
		case BASICTYPE_ENUMERATED:
			fprintf(hdr, "0"); // FIXME
			break;
		case BASICTYPE_SEQUENCEOF: // Array
			// Use default from HumenModel which is a new Array instance
			//fprintf(hdr, "[]");//  , t->cxxTypeRefInfo->className);
			break;
		case BASICTYPE_REAL:
			fprintf(hdr, "0.0");
			break;
		case BASICTYPE_UTF8_STR:
			fprintf(hdr, "''");
			break;
		case BASICTYPE_UTCTIME:
			fprintf(hdr, "''");
			break;
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
				fprintf(hdr, "null");
			break;
		case BASICTYPE_IMPORTTYPEREF:
		case BASICTYPE_LOCALTYPEREF:
			if (strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
			{
				fprintf(hdr, "''"); // AsnSystemTime ist im Asn1-file als REAL definiert, wird aber im JS als String übermittelt.
			}
			else if (strcmp(t->cxxTypeRefInfo->className, "AsnContactID") == 0) 
			{
				fprintf(hdr, "''");
			}
			else 
			{
				fprintf(hdr, "{}");
			}
			break;
		default:
			fprintf(hdr, "[UNKNOWN BASIC TYPE] {}"); //  , t->cxxTypeRefInfo->className);
			break;
		}
	}
}

static void PrintJSES6Type(FILE *hdr, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *t)
{
	// fprintf(hdr, "{type: '");


	// fprintf(hdr, "[PrintJSES6Type] ");
	if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_SEQUENCEOF) ||
		(t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && t->basicType->a.importTypeRef->link != NULL && t->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_SEQUENCEOF))
	{
		// fprintf(hdr, "[SEQUENCE OF] ");
		if (strcmp(t->cxxTypeRefInfo->className, "AsnOptionalParameters") == 0) // ESTOS special 'native Object' AsnOptionalParamaters
		{
		}
		else if (strcmp(t->cxxTypeRefInfo->className, "UTF8StringList") == 0 || strcmp(t->cxxTypeRefInfo->className, "SEQInteger") == 0 || strcmp(t->cxxTypeRefInfo->className, "AsnContactIDs") == 0)
		{
		}
		else
		{
			fprintf(hdr, "[Listtype]");
		}		
		fprintf(hdr, " type: %s", t->cxxTypeRefInfo->className);
	}
	else if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED) ||
			(t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && t->basicType->a.importTypeRef->link != NULL && t->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED))
	{
		fprintf(hdr, "[ENUMERATED] ");
		// fprintf(hdr, "number");
	}
	else if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_CHOICE) ||
			(t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && t->basicType->a.importTypeRef->link != NULL && t->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_CHOICE))
	{
		// fprintf(hdr, "[BASICTYPE_CHOICE] %s", t->cxxTypeRefInfo->className);
		fprintf(hdr, " type: %s", t->cxxTypeRefInfo->className);
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
			PrintJSES6NativeType(hdr, t->basicType->choiceId);
			break;
		case BASICTYPE_SEQUENCEOF:
			fprintf(hdr, "%s[]", t->cxxTypeRefInfo->className);
			break;
		case BASICTYPE_EXTENSION:
			fprintf(hdr, "object");
			break;
		case BASICTYPE_IMPORTTYPEREF:
		case BASICTYPE_LOCALTYPEREF:
			if (strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
			{
				fprintf(hdr, "string"); // AsnSystemTime ist im Asn1-file als REAL definiert, wird aber im JS als String übermittelt.
			}
			else if (strcmp(t->cxxTypeRefInfo->className, "AsnContactID") == 0) 
			{
				fprintf(hdr, "string");
			}
			else
			{
				fprintf(hdr, " type: %s", t->cxxTypeRefInfo->className);
			}
			break;
		default:
			fprintf(hdr, "[UNKNOWN BASIC TYPE] %s", t->cxxTypeRefInfo->className);
			break;
		}
	}
} /* PrintCxxType */


static void PrintJSES6BitstringDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *enumerated, int novolatilefuncs)
{
	//	NamedType *e;
	//	enum BasicTypeChoiceId tmpTypeId;
	CNamedElmt *n;
	fprintf(src, "// [PrintJSES6BitstringDefCode] %s\n", td->definedName);

	
	#ifndef EXPORTS	
		fprintf(src, "const %s = {\n", td->definedName);
	#else
		fprintf(src, "export const %s = {\n", td->definedName);
	#endif
	
	if (HasNamedElmts(td->type) != 0) {
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			fprintf(src, "  %s: %d", n->name, 0x00000001 << n->value);
			if (((td->type->cxxTypeRefInfo->namedElmts)->curr->next && ((td->type->cxxTypeRefInfo->namedElmts)->curr->next->data) != NULL))
				fprintf(src, ",");
			fprintf(src, "\n");
		}
	}

	/* close class definition */
	fprintf(src, "};\n\n\n");
} /* PrintJSES6BitstringDefCode */

static void PrintJSES6EnumDefCode(FILE *src, ModuleList *mods, Module *m,
	TypeDef *td, Type *parent, Type *enumerated, int novolatilefuncs)
{
	//	NamedType *e;
	//	enum BasicTypeChoiceId tmpTypeId;
	CNamedElmt *n;
	fprintf(src, "// [PrintJSES6EnumDefCode] %s\n", td->definedName);

	#ifndef EXPORTS	
		fprintf(src, "const %s = Object.freeze({\n", td->definedName);
	#else
		fprintf(src, "export const %s = Object.freeze({\n", td->definedName);
	#endif
	
	if (HasNamedElmts(td->type) != 0) {
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			fprintf(src, "\t%s: %d", n->name, n->value);
			if (((td->type->cxxTypeRefInfo->namedElmts)->curr->next && ((td->type->cxxTypeRefInfo->namedElmts)->curr->next->data) != NULL))
				fprintf(src, ",");
			fprintf(src, "\n");
		}
	}

	/* close class definition */
	fprintf(src, "});\n\n\n");
} /* PrintJSES6EnumDefCode */

static void PrintJSES6ChoiceDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *choice, int novolatilefuncs)
{
	NamedType *e;
	//	CxxTRI *cxxtri=NULL;
	//	int inTailOptElmts;
	//	enum BasicTypeChoiceId tmpTypeId;
	//	int allOpt;

	// DEFINE PER encode/decode tmp vars.
	NamedType **pSeqElementNamedType = NULL;
	int propertyCounter = 0;
//	int collectionCounter = 0;

	fprintf(src, "// [PrintJSES6ChoiceDefCode] %s\n", td->definedName);

	/* put class spec in hdr file */
	#ifndef EXPORTS
		fprintf(src, "class %s {\n", td->definedName);
	#else
		fprintf(src, "export class %s {\n", td->definedName);
	#endif
	
	// fprintf(src, "\ttype: '%s',\n", td->definedName);

	/* Write out properties */
	// fprintf(src, "\tprops: {\n");
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.sequence)
	{
		if (propertyCounter > 0) {
			// fprintf(src, ",");
			fprintf(src, "\n");
		}

		fprintf(src, "\t\t");
		fprintf(src, "static %s = ", e->fieldName);
		PrintJSES6DefaultValue(src, mods, m, td, choice, e->type);
		fprintf(src, " // BLUBBER");
		PrintJSES6Type(src, mods, m, td, choice, e->type);

		
		fprintf(src, "}");

		propertyCounter++;
	}
	// fprintf(src, "\n\t}");

	fprintf(src, "\n");

	if (pSeqElementNamedType)
		free(pSeqElementNamedType);

	/* close class definition */
	fprintf(src, "}\n\n");
} /* PrintJSES6ChoiceDefCode */

static void PrintJSES6SeqDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *seq, int novolatilefuncs)
{
	NamedType *e;
	//	CxxTRI *cxxtri=NULL;
	//	int inTailOptElmts;
	//	enum BasicTypeChoiceId tmpTypeId;
	//	int allOpt;

	// DEFINE PER encode/decode tmp vars.
	NamedType **pSeqElementNamedType = NULL;
	int propertyCounter = 0;
	
	fprintf(src, "// [PrintJSES6SeqDefCode] %s\n", td->definedName);

	/* put class spec in hdr file */
	// fprintf(src, "function %s() {\n", td->definedName);
	#ifndef EXPORTS
		fprintf(src, "class %s {\n", td->definedName);
	#else
		fprintf(src, "export class %s {\n", td->definedName);
	#endif
	
	fprintf(src, "\t_type = '%s';\n", td->definedName);
	
	/* Write out properties */
	// fprintf(src, "\tprops: {\n");
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		// OptionalParams ...
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION) {
			continue;
		}

		if (propertyCounter > 0) {
			fprintf(src, ",");
			fprintf(src, "\n");
		}

		fprintf(src, "\t");
		fprintf(src, "%s = ", e->fieldName);
		PrintJSES6DefaultValue(src, mods, m, td, seq, e->type);
		fprintf(src, "; // ");
		PrintJSES6Type(src, mods, m, td, seq, e->type);

		fprintf(src, "}");

		propertyCounter++;
	}
	// fprintf(src, "\n\t}");

	/* Write out collections */
	//FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	//{
	//	if (collectionCounter == 0) {
	//		fprintf(src, ",\n\tcollections: {\n");
	//	}
	//	else {
	//		fprintf(src, ",");
	//		fprintf(src, "\n");
	//	}

	//	// vars
	//	fprintf(src, "\t\t");
	//	fprintf(src, "%s: %s", e->fieldName, e->type->cxxTypeRefInfo->className);
	//			
	//	collectionCounter++;
	//}
	//if (collectionCounter > 0) {
	//	fprintf(src, "\n\t}");
	//}

	fprintf(src, "\n");

	if (pSeqElementNamedType)
		free(pSeqElementNamedType);

	/* close class definition */
	fprintf(src, "}\n\n");
} /* PrintCxxSeqDefCode */

static void PrintJSES6ListClass(FILE *src, TypeDef *td, Type *lst, Module* m, ModuleList *mods)
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
			fprintf(src, "//  %s [No collections of primitive Types %s]\n", td->cxxTypeDefInfo->className, p_e->type->cxxTypeRefInfo->className);
			return;
		default:
			break;
	}
	if (strcmp(p_e->type->cxxTypeRefInfo->className, "AsnContactID") == 0)
	{
		fprintf(src, "//  %s [No collections of primitive Types %s]\n", td->cxxTypeDefInfo->className, p_e->type->cxxTypeRefInfo->className);
		return;
	}

	fprintf(src, "// const %s = []; // List of %s\n\n", td->cxxTypeDefInfo->className, p_e->type->cxxTypeRefInfo->className);
	//fprintf(src, "var %s = [{\n", td->cxxTypeDefInfo->className);
	////fprintf(src, "%s = {\n", td->cxxTypeDefInfo->className);
	////fprintf(src, "\t_abstractType: 'Backbone.Collection',\n");
	//fprintf(src, "\ttype: '%s',\n", td->cxxTypeDefInfo->className);
	//fprintf(src, "\tmodel: %s\n", );
	//fprintf(src, "}],\n\n");
}

static void PrintJSES6SetOfDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *setOf, int novolatilefuncs)
{
	if (strcmp(td->cxxTypeDefInfo->className, "AsnOptionalParameters") == 0) // ESTOS special 'native Object' AsnOptionalParamaters
		return;

	/* do class */
	PrintJSES6ListClass(src, td, setOf, m, mods);

} /* PrintJSES6SetOfDefCode */

static void PrintJSES6TypeDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, int novolatilefuncs)
{
	fprintf(src, "// [PrintJSES6TypeDefCode] %s\n", td->definedName);

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
		//PrintCxxSimpleDef (hdr, src, m, r, td);
		fprintf(src, "// [SIMPLEDEF]\n");
		break;
	case BASICTYPE_BITSTRING:  /* library type */
		PrintJSES6BitstringDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_SEQUENCEOF:  /* list types */
	case BASICTYPE_SETOF:
		// fprintf(src, "// [BASICTYPE_SEQUENCEOF/BASICTYPE_SETOF]\n");
		PrintJSES6SetOfDefCode (src, mods, m, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_IMPORTTYPEREF:  /* type references */
		fprintf(src, "// [BASICTYPE_IMPORTTYPEREF]\n");
		break;
	case BASICTYPE_LOCALTYPEREF:
		fprintf(src, "// [BASICTYPE_LOCALTYPEREF]\n");
		/*
		* if this type has been re-tagged then
		* must create new class instead of using a typedef
		*/
		//PrintCxxSimpleDef (hdr, src, m, r, td);
		break;
	case BASICTYPE_ANYDEFINEDBY:  /* ANY types */
	case BASICTYPE_ANY:
		fprintf(src, "// [BASICTYPE_ANY]\n");
		//PrintCxxAnyDefCode (src, hdr, mods, m, r, td, NULL, td->type);
		break;
	case BASICTYPE_CHOICE:
		PrintJSES6ChoiceDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_ENUMERATED:  /* library type */
		PrintJSES6EnumDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_SET:
		fprintf(src, "// [BASICTYPE_SET]\n");
		//PrintCxxSetDefCode (src, hdr, mods, m, r, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_SEQUENCE:
		PrintJSES6SeqDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
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
} /* PrintCxxTypeDefCode */


static void PrintJSES6ExportCode(FILE *src, Module *m, TypeDef *td)
{
	//fprintf(src, "// [PrintHJSExportCode] %s\n", td->definedName);
	switch (td->type->basicType->choiceId)
	{
	case BASICTYPE_BITSTRING:  /* library type */
	case BASICTYPE_CHOICE:
	case BASICTYPE_ENUMERATED:  /* library type */
	case BASICTYPE_SEQUENCE:

		// Datentypen, die in JS nicht als eigenes HumanModel abgebildet werden.
		if (strcmp(td->definedName, "UTF8StringList") == 0
			|| strcmp(td->definedName, "SEQInteger") == 0
			|| strcmp(td->definedName, "AsnContactIDs") == 0
			|| strcmp(td->definedName, "AsnContactID") == 0
			|| strcmp(td->definedName, "AsnOptionalParameters") == 0) {
			break;
		}

		fprintf(src, "\t %s: %s", td->definedName, td->definedName);

		if (td != (TypeDef *)LAST_LIST_ELMT(m->typeDefs))
			fprintf(src, ",\n");
		else
			fprintf(src, "\n");

		break;
	case BASICTYPE_SEQUENCEOF:  /* list types */
	case BASICTYPE_SETOF:
	default:
		break;
	}
} /* PrintJSES6ExportCode */

void PrintJSES6Imports(FILE *src, ModuleList *mods, Module *m)
{
	Module *currMod;
	AsnListNode *currModTmp;

	fprintf(src, "// Global imports\n");
	//fprintf(src, "var Backbone = require('backbone');\n");
	//fprintf(src, "var HumanModel = require('../lib/human-model');\n");

	fprintf(src, "\n");

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

				fprintf(src, "// Imports from %s\n", impMod->modId->name);

				if (impMod->moduleRef == NULL)
					impMod->moduleRef = GetImportModuleRef(impMod->modId->name, mods);

				FOR_EACH_LIST_ELMT(impElmt, impMod->importElmts)
				{
					if (strcmp(impElmt->name, "AsnOptionalParameters") == 0
						|| strcmp(impElmt->name, "UTF8StringList") == 0
						|| strcmp(impElmt->name, "SEQInteger") == 0
						|| strcmp(impElmt->name, "AsnContactIDs") == 0
						|| strcmp(impElmt->name, "AsnSystemTime") == 0)
					{
						continue;
					}
					else {
						fprintf(src, "const %s = require('./%s').%s;\n", impElmt->name, impMod->moduleRef->jsFileName, impElmt->name);
					}
				}

			}
			mods->curr = currModTmp;    // RWC;RESET loop control
		}
	}
}

void PrintJSES6Comments(FILE *src, Module *m) {
	fprintf(src, "/*\n");
	fprintf(src, " * %s\n", RemovePath(m->jsFileName));
	fprintf(src, " * \"%s\" ASN.1 stubs.\n", m->modId->name);
	write_snacc_header(src, " * ");
	fprintf(src, " */\n\n");
}

void PrintJSES6ROSECode(FILE *src, ModuleList *mods, Module *m)
{
}

void PrintJSES6Code(FILE *src, ModuleList *mods, Module *m, long longJmpVal, int printTypes, int printValues, int printEncoders, int printDecoders, int PrintJSES6ONEncDec, int novolatilefuncs)
{
	TypeDef *td;

	fprintf(src, "// [PrintJSES6Code]\n");

	//	ValueDef *vd;

	// Comments
	PrintJSES6Comments(src, m);

	// Includes
	PrintJSES6Imports(src, mods, m);

	fprintf(src, "\n\n");

	//fprintf(src, "//------------------------------------------------------------------------------\n");
	//fprintf(src, "// class declarations:\n\n");
	//FOR_EACH_LIST_ELMT (td, m->typeDefs)
	//	PrintJSES6TypeDecl(src, td);
	//fprintf(src, "\n");


	FOR_EACH_LIST_ELMT (td, m->typeDefs)
		PrintJSES6TypeDefCode (src, mods, m, td, novolatilefuncs);


	fprintf(src, "\n");
	fprintf(src, "// [PrintJSES6ExportCode]\n");

	// "export
#ifndef EXPORTS	
	fprintf(src, "module.exports = {\n");

	FOR_EACH_LIST_ELMT(td, m->typeDefs)
		PrintJSES6ExportCode(src, m, td);

	fprintf(src, "};\n");
#endif
	
	// PrintConditionalIncludeClose (hdr, m->cxxHdrFileName);
} /* PrintJSES6Code */

/* EOF gen-code.c (for back-ends/JS-gen) */

