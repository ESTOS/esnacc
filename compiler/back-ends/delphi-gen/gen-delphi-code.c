/*
*   compiler/back_ends/Delphi_gen/gen_delphi_code.c - routines for printing Delphi code from type trees
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
* 2016 estos/chs
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
*/

// TODO: SET, SET OF, ANY, BASICTYPE_EXTENSION is missing. BASICTYPE_OCTETCONTAINING probably partial. AsnOptionalParameters not working 

#include "gen-delphi-code.h"
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


static void PrintDelphiNativeType(FILE *hdr, int basicTypeChoiseId) 
{
	switch (basicTypeChoiseId) {
	case BASICTYPE_BOOLEAN:
		fprintf(hdr, "Boolean");
		break;
	case BASICTYPE_INTEGER:
		fprintf(hdr, "Integer");
		break;
	case BASICTYPE_OCTETSTRING:
	case BASICTYPE_OCTETCONTAINING:
		fprintf(hdr, "RawByteString");
		break;
	//case BASICTYPE_ENUMERATED:
	//	fprintf(hdr, "Integer"); //FIXME
	//	break;
	case BASICTYPE_REAL:
		fprintf(hdr, "double");
		break;
	case BASICTYPE_UTF8_STR:
		fprintf(hdr, "string");
		break;
	case BASICTYPE_UTCTIME:
		fprintf(hdr, "TDateTime");
		break;
	case BASICTYPE_UNKNOWN:
	case BASICTYPE_NULL:
		fprintf(hdr, "TObject"); //really?
		break;
	default:
		exit(1);
		break;
	}
}


static int DelphiIsEnumeratedType(Type *t)
{
	return (t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED) ||
		(t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && t->basicType->a.importTypeRef->link != NULL && t->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED);

	
}

static int DelphiIsFinalType(Type* t)
{
	switch (t->basicType->choiceId) {
	case BASICTYPE_BOOLEAN:
	case BASICTYPE_INTEGER:
	case BASICTYPE_OCTETSTRING:
	case BASICTYPE_OCTETCONTAINING:
	case BASICTYPE_REAL:
	case BASICTYPE_UTF8_STR:
	case BASICTYPE_UTCTIME:
		return TRUE;
	case BASICTYPE_ENUMERATED:
	case BASICTYPE_UNKNOWN:
	case BASICTYPE_NULL:
	case BASICTYPE_SEQUENCEOF:
	case BASICTYPE_EXTENSION:
	case BASICTYPE_IMPORTTYPEREF:
	case BASICTYPE_LOCALTYPEREF:
		return FALSE;
	default:
		return FALSE;
	}

}

// the name of the enum itself. not the class name
static void PrintDelphiEnumType(FILE *hdr, ModuleList *mods, Module *mmodule, TypeDef *td, Type *parent, Type *t)
{
	if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED) ||
		(t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && t->basicType->a.importTypeRef->link != NULL && t->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED))
	{
		fprintf(hdr, "T%s.%sEnum", t->cxxTypeRefInfo->className, t->cxxTypeRefInfo->className);
	}
}

static void PrintDelphiType(FILE *hdr, ModuleList *mods, Module *mmodule, TypeDef *td, Type *parent, Type *t)
{
	// fprintf(hdr, "{type: '");
	enum BasicTypeChoiceId basictype = BASICTYPE_UNKNOWN;
	if (t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL)
	{
		basictype = t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId;
	}
	else if (t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && t->basicType->a.importTypeRef->link != NULL)
	{
		basictype = t->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId;
	}

	// fprintf(hdr, "[PrintDelphiType] ");
	if (basictype == BASICTYPE_SEQUENCEOF)
	{
		//fprintf(hdr, "[SEQUENCE OF] ");
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
			//fprintf(hdr, "[Listtype]");
			// fprintf(hdr, "', // type: %s", t->cxxTypeRefInfo->className);
		}		
		fprintf(hdr, " T%s", t->cxxTypeRefInfo->className); // Prefix for class names
	}
	else if (basictype == BASICTYPE_ENUMERATED)
	{
		fprintf(hdr, "T%s", t->cxxTypeRefInfo->className); // Prefix for class names
	}
	else if (basictype == BASICTYPE_CHOICE)
	{
		// fprintf(hdr, "[BASICTYPE_CHOICE] %s", t->cxxTypeRefInfo->className);
		fprintf(hdr, "T%s", t->cxxTypeRefInfo->className); // Prefix for class names
	}	
	else
	{
		// fprintf(hdr, "[BASIC TYPE] ");
		switch (t->basicType->choiceId) {
		case BASICTYPE_BOOLEAN:
			fprintf(hdr, "AsnBool");
			break;
		case BASICTYPE_INTEGER:
			fprintf(hdr, "AsnInteger");
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(hdr, "AsnOctet");
			break;
		case BASICTYPE_ENUMERATED:
			fprintf(hdr, "[BASICTYPE_ENUMERATED xx]");
			break;
		case BASICTYPE_REAL:
			fprintf(hdr, "AsnReal");
			break;
		case BASICTYPE_UTF8_STR:
			fprintf(hdr, "AsnUtf8String");
			break;
		case BASICTYPE_UTCTIME:
			fprintf(hdr, "AsnTime");
			break;
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
			fprintf(hdr, "AsnNull");
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
				fprintf(hdr, "AsnTime"); // AsnSystemTime ist im Asn1-file als REAL definiert, wird aber im JS als String übermittelt.
			}
			else if (strcmp(t->cxxTypeRefInfo->className, "AsnContactID") == 0) 
			{
				fprintf(hdr, "AsnUtf8String");
			}
			else
			{
				// custom asn.1 object
				fprintf(hdr, "T%s", t->cxxTypeRefInfo->className);  // Prefix for class names
			}
			break;
		default:
			fprintf(hdr, "[UNKNOWN BASIC TYPE] %s", t->cxxTypeRefInfo->className);
			break;
		}
	}
} /* PrintDelphiType */


static void PrintDelphiBitstringDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *enumerated)
{
	//	NamedType *e;
	//	enum BasicTypeChoiceId tmpTypeId;
	CNamedElmt *n;
	fprintf(src, "// [PrintDelphiBitstringDefCode] %s\n", td->definedName);

	fprintf(src, "var %s = {\n", td->definedName);
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
} /* PrintDelphiBitstringDefCode */

static void PrintDelphiEnumDefCode(FILE *src, ModuleList *mods, Module *m,
	TypeDef *td, Type *parent, Type *enumerated)
{
	//	NamedType *e;
	//	enum BasicTypeChoiceId tmpTypeId;
	CNamedElmt *n;
	fprintf(src, "// [PrintDelphiEnumDefCode] %s\n", td->definedName);



	/* class definition */
	fprintf(src, "  T%s = class(TObject)\n", td->definedName);// Prefix for class names
	fprintf(src, "  public type\n");

	fprintf(src, "    %sEnum = (\n", td->definedName);
	if (HasNamedElmts(td->type) != 0) {
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			fprintf(src, "      %s = %d", n->name, n->value);
			if (((td->type->cxxTypeRefInfo->namedElmts)->curr->next && ((td->type->cxxTypeRefInfo->namedElmts)->curr->next->data) != NULL))
				fprintf(src, ",");
			fprintf(src, "\n");
		}
	}

	/* close enum definition */
	fprintf(src, "    );\n");

	fprintf(src, "  public\n");
	fprintf(src, "    value: %sEnum;\n", td->definedName);
	fprintf(src, "    procedure JEnc(var json : TJSONValue);\n");
	fprintf(src, "    function JDec(json: TJSONValue) : Boolean;\n");

	/* close class definition */
	fprintf(src, "  end;\n\n");
} /* PrintDelphiEnumDefCode */

static void PrintDelphiChoiceDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *choice)
{
	NamedType *e;

	fprintf(src, "// [PrintDelphiChoiceDefCode] %s\n", td->definedName);

	fprintf(src, "  T%s = class (TObject)\n", td->definedName); // Prefix for class names

	fprintf(src, "  public type\n");
	//
	// internal enum
	//
	fprintf(src, "    ChoiceIdEnum =\n");
	fprintf(src, "    (\n");
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.choice)
	{
		fprintf(src, "      %s = %d", e->type->cxxTypeRefInfo->choiceIdSymbol, e->type->cxxTypeRefInfo->choiceIdValue);
		if (e != (NamedType*)LAST_LIST_ELMT(choice->basicType->a.choice))
			fprintf(src, ",\n");
		else
			fprintf(src, "\n");
	}
	fprintf(src, "    );\n"); // close internal enum

	//
	// internal case-record / union
	//
	fprintf(src, "  private type\n");
	fprintf(src, "    T%sInternal = record\n", td->definedName);
	fprintf(src, "    case byte of\n");
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.choice)
	{
		fprintf(src, "      %d: (%s: ", e->type->cxxTypeRefInfo->choiceIdValue, e->type->cxxTypeRefInfo->fieldName);
		PrintDelphiType(src, mods, m, td, choice, e->type);
		fprintf(src, ");\n");
	}
	fprintf(src, "    end;\n"); // close internal record

	//
	// members, functions etc.
	//
	fprintf(src, "  private\n"); 
	fprintf(src, "    FchoiceInternal: T%sInternal;\n", td->definedName);

	fprintf(src, "  public\n");
	fprintf(src, "    ChoiceId: ChoiceIdEnum;\n");
	fprintf(src, "    constructor Create();\n");
	fprintf(src, "    destructor Destroy(); override;\n");
	fprintf(src, "    procedure Clear();\n");

	fprintf(src, "    procedure JEnc(var json: TJSONValue);\n");
	fprintf(src, "    function JDec(json: TJSONValue): Boolean;\n");


	//
	// properties
	//
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.sequence)
	{
		fprintf(src, "    property %s: ", e->fieldName);
		PrintDelphiType(src, mods, m, td, choice, e->type);
		fprintf(src, " read FchoiceInternal.%s write FchoiceInternal.%s;\n", e->fieldName, e->fieldName);
	}

	/* close class definition */
	fprintf(src, "  end;\n\n");
} /* PrintDelphiChoiceDefCode */

static void PrintDelphiSeqDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *seq)
{
	NamedType *e;

	// DEFINE PER encode/decode tmp vars.
	//NamedType **pSeqElementNamedType = NULL;
	//	int collectionCounter = 0;

	fprintf(src, "// [PrintDelphiSeqDefCode] %s\n", td->definedName);


	fprintf(src, "  T%s = class (TAsnBase)\n", td->definedName); // Prefix for class names
																//	fprintf(src, "  private\n");


	fprintf(src, "  public\n");
	fprintf(src, "    constructor Create(); override;\n");
	fprintf(src, "    destructor Destroy(); override;\n");

	fprintf(src, "    procedure Clear();\n"); 
	fprintf(src, "    procedure JEnc(var json: TJSONValue); override;\n");
	fprintf(src, "    function JDec(json: TJSONValue; suppressErrMissing: Boolean = false; suppressErrDecode: Boolean = false): Boolean; override;\n");


	/* add fields */

	int iOptionalCount = 0;
	// print public fields for optional types
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		if (!e->type->optional)
			continue;
		//// OptionalParams ...
		//if (e->type->basicType->choiceId == BASICTYPE_EXTENSION) {
		//	fprintf(src, "    //skipping BASICTYPE_EXTENSION field %s", e->fieldName);
		//	fprintf(src, "\n");
		//	continue;
		//}
		if (iOptionalCount == 0)
		{
			fprintf(src, "  public\n");
			fprintf(src, "  // optional parameters - must be created on use\n");
		}
		

		fprintf(src, "    F%s: ", e->fieldName);
		PrintDelphiType(src, mods, m, td, seq, e->type);
		fprintf(src, ";\n");
		iOptionalCount++;
	}

	fprintf(src, "  private\n");
	// and private types for mandantory
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		// OptionalParams ...
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION) {
			fprintf(src, "    //skipping BASICTYPE_EXTENSION field %s", e->fieldName);
			fprintf(src, "\n");
			continue;
		}
		if (e->type->optional)
			continue;

		fprintf(src, "    F%s: ", e->fieldName);
		PrintDelphiType(src, mods, m, td, seq, e->type);
		fprintf(src, ";\n");
	}

	/* add getters/setters */
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		// OptionalParams ...
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION) {
			continue;
		}
		if (e->type->optional)
		{
			continue;
		}

		if (DelphiIsFinalType(e->type))
		{
			fprintf(src, "    function Get%s: ", e->fieldName);
			PrintDelphiNativeType(src, e->type->basicType->choiceId);
			fprintf(src, ";\n");
			fprintf(src, "    procedure Set%s(const Value: ", e->fieldName);
			PrintDelphiNativeType(src, e->type->basicType->choiceId);
			fprintf(src, ");\n");
		}
		else if (DelphiIsEnumeratedType(e->type))
		{
			fprintf(src, "    function Get%s: ", e->fieldName);
			PrintDelphiEnumType(src, mods, m, td, seq, e->type);
			fprintf(src, ";\n");
			fprintf(src, "    procedure Set%s(const Value: ", e->fieldName);
			PrintDelphiEnumType(src, mods, m, td, seq, e->type);
			fprintf(src, ");\n"); 

		}
	}



	/* add properties */
	fprintf(src, "  public\n");
	// fprintf(src, "\ttype: '%s',\n", td->definedName);
	
	/* Write out properties */
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		// OptionalParams ...
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION) {
			continue;
		}
		if (e->type->optional)
		{
			continue;
		}


		fprintf(src, "    property %s: ", e->fieldName);
		//PrintDelphiDefaultValue(src, mods, m, td, seq, e->type);

		if (DelphiIsFinalType(e->type))
		{
			PrintDelphiNativeType(src, e->type->basicType->choiceId);
			// "final type" - use getters and setters
			fprintf(src, " read Get%s write Set%s;\n", e->fieldName, e->fieldName);
		}
		//else if (e->type->basicType->choiceId == BASICTYPE_ENUMERATED)
		else if (DelphiIsEnumeratedType(e->type))
		{
			PrintDelphiEnumType(src, mods, m, td, seq, e->type);
			fprintf(src, " read Get%s write Set%s;\n", e->fieldName, e->fieldName);
		}
		else
		{
			PrintDelphiType(src, mods, m, td, seq, e->type);
			// another asn object. only add getter to access object
			fprintf(src, " read F%s;\n", e->fieldName);
		}
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

	//if (pSeqElementNamedType)
	//	free(pSeqElementNamedType);

	/* close class definition */
	fprintf(src, "  end;\n\n");
} /* PrintDelphiSeqDefCode */

static void PrintDelphiListClass(FILE *src, TypeDef *td, Type *lst, Module* m, ModuleList *mods)
{
	fprintf(src, "// [PrintDelphiListClass] %s\n", td->definedName);
	struct NamedType p_etemp;
	NamedType* p_e;

	p_e = &p_etemp;
	p_e->type = lst->basicType->a.setOf;

	switch (lst->basicType->a.setOf->basicType->choiceId) {
		//case BASICTYPE_BOOLEAN:
		//case BASICTYPE_INTEGER:
		//case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
		//case BASICTYPE_ENUMERATED:
		//case BASICTYPE_REAL:
		//case BASICTYPE_UTF8_STR:
		//case BASICTYPE_UTCTIME:
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

	fprintf(src, "  T%s = class(TAsnSeqOf<", td->cxxTypeDefInfo->className);
	PrintDelphiType(src, mods, m, td, lst, lst->basicType->a.sequenceOf);
	fprintf(src, ">)\n");

	fprintf(src, "    procedure JEnc(var json : TJSONValue);\n");
	fprintf(src, "    function JDec(json: TJSONValue; suppressErrMissing: Boolean = false; suppressErrDecode: Boolean = false) : Boolean;\n");


	fprintf(src, "  end;\n");
}

static void PrintDelphiSetOfDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *setOf)
{
	if (strcmp(td->cxxTypeDefInfo->className, "AsnOptionalParameters") == 0) // ESTOS special 'native Object' AsnOptionalParamaters
		return;

	/* do class */
	PrintDelphiListClass(src, td, setOf, m, mods);

} /* PrintDelphiSetOfDefCode */

static void PrintDelphiTypeDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td)
{
	fprintf(src, "// [PrintDelphiTypeDefCode] %s\n", td->definedName);


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
		//PrintDelphiSimpleDef (hdr, src, m, r, td);
		fprintf(src, "// [SIMPLEDEF]\n");
		break;
	case BASICTYPE_BITSTRING:  /* library type */
		PrintDelphiBitstringDefCode(src, mods, m, td, NULL, td->type);
		break;
	case BASICTYPE_SEQUENCEOF:  /* list types */
	case BASICTYPE_SETOF:
		// fprintf(src, "// [BASICTYPE_SEQUENCEOF/BASICTYPE_SETOF]\n");
		PrintDelphiSetOfDefCode(src, mods, m, td, NULL, td->type);
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
		//PrintDelphiSimpleDef (hdr, src, m, r, td);
		break;
	case BASICTYPE_ANYDEFINEDBY:  /* ANY types */
	case BASICTYPE_ANY:
		fprintf(src, "// [BASICTYPE_ANY]\n");
		//PrintDelphiAnyDefCode (src, hdr, mods, m, r, td, NULL, td->type);
		break;
	case BASICTYPE_CHOICE:
		PrintDelphiChoiceDefCode(src, mods, m, td, NULL, td->type);
		break;
	case BASICTYPE_ENUMERATED:  /* library type */
		PrintDelphiEnumDefCode(src, mods, m, td, NULL, td->type);
		break;
	case BASICTYPE_SET:
		fprintf(src, "// [BASICTYPE_SET]\n");
		//PrintDelphiSetDefCode (src, hdr, mods, m, r, td, NULL, td->type);
		break;
	case BASICTYPE_SEQUENCE:
		fprintf(src, "// [BASICTYPE_SEQUENCE]\n");
		PrintDelphiSeqDefCode(src, mods, m, td, NULL, td->type);
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
	
} /* PrintDelphiTypeDefCode */

void PrintDelphiSeqConstructor(FILE *src, TypeDef *td, Type *seq)
{
	NamedType *e;
	char *varName;
	CxxTRI *cxxtri = NULL;

	fprintf(src, "constructor T%s.Create();\n", td->definedName); // Prefix for class names
	fprintf(src, "begin\n");
	fprintf(src, "  inherited Create();\n");
	fprintf(src, "\n");


	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		cxxtri = e->type->cxxTypeRefInfo;
		varName = cxxtri->fieldName;

		fprintf(src, "  F%s := ", varName);
		if (e->type->optional)
		{
			fprintf(src, "nil; // optional\n");
		}
		else
		{
			PrintDelphiType(src, NULL, NULL, td, seq, e->type); // the both NULLs are not used in PrintDelphiType
			fprintf(src, ".Create();\n" );
		}
	}

	fprintf(src, "end;\n");
	fprintf(src, "\n");
} /* PrintDelphiSeqConstructor */

void PrintDelphiDestructor(FILE *src, TypeDef *td, Type *seq)
{
	NamedType *e;
	char *varName;
	CxxTRI *cxxtri = NULL;

	fprintf(src, "destructor T%s.Destroy();\n", td->definedName); // Prefix for class names
	fprintf(src, "begin\n");


	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		cxxtri = e->type->cxxTypeRefInfo;
		varName = cxxtri->fieldName;

		fprintf(src, "  FreeAndNil(F%s);\n", varName);
	}
	fprintf(src, "\n");
	fprintf(src, "  inherited;\n");

	fprintf(src, "end;\n");
	fprintf(src, "\n");
} /* PrintDelphiDestructor */

void PrintDelphiSeqDefClear(FILE *src, TypeDef *td, Type *seq)
{
	NamedType *e;
	char *varName;
	CxxTRI *cxxtri = NULL;

	fprintf(src, "procedure T%s.Clear();\n", td->definedName); // Prefix for class names
	fprintf(src, "begin\n");

	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		cxxtri = e->type->cxxTypeRefInfo;
		varName = cxxtri->fieldName;
		if (e->type->optional)
			fprintf(src, "  FreeAndNil(F%s);\n", varName);
		else if (!DelphiIsFinalType(e->type) && !DelphiIsEnumeratedType(e->type) && (e->type->basicType->choiceId != BASICTYPE_NULL) && (e->type->basicType->choiceId != BASICTYPE_UNKNOWN))
			fprintf(src, "  F%s.Clear();\n", varName);
	}

	fprintf(src, "end;\n");
	fprintf(src, "\n");
} /* PrintDelphiSeqDefClear */

void PrintDelphiChoiceConstructor(FILE *src, TypeDef *td, Type *choice)
{
	NamedType *e;
	//char *varName;
	//CxxTRI *cxxtri = NULL;

	fprintf(src, "constructor T%s.Create();\n", td->definedName); // Prefix for class names
	fprintf(src, "begin\n");
	fprintf(src, "  inherited Create();\n");
	fprintf(src, "\n");

	// I need only the first element

	FOR_EACH_LIST_ELMT_NOITERATE(e, choice->basicType->a.choice)
	{
		fprintf(src, "  ChoiceId := ChoiceIdEnum.%s;\n", e->type->cxxTypeRefInfo->choiceIdSymbol);
		fprintf(src, "  FchoiceInternal.%s := nil;\n", e->fieldName);
		break;
	}

	fprintf(src, "end;\n");
	fprintf(src, "\n");
} /* PrintDelphiChoiceConstructor */

void PrintDelphiChoiceDestructor(FILE *src, TypeDef *td, Type *choice)
{
//	NamedType *e;

	fprintf(src, "destructor T%s.Destroy();\n", td->definedName); // Prefix for class names
	fprintf(src, "begin\n");
	fprintf(src, "  Clear();\n");
	fprintf(src, "  inherited;\n");

	fprintf(src, "end;\n");
	fprintf(src, "\n");
} /* PrintDelphiChoiceDestructor */

void PrintDelphiChoiceCodeClear(FILE *src, TypeDef *td, Type *choice)
{
	NamedType *e;

	fprintf(src, "procedure T%s.Clear();\n", td->definedName); // Prefix for class names
	fprintf(src, "begin\n");
	fprintf(src, "  case choiceId of\n");
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.choice)
	{
		fprintf(src, "    ChoiceIdEnum.%s:\t\t", e->type->cxxTypeRefInfo->choiceIdSymbol);
		fprintf(src, "FreeAndNil(FchoiceInternal.%s);\n", e->fieldName);
	}
	fprintf(src, "  end; // end case\n"); // end case
	fprintf(src, "end;\n");
	fprintf(src, "\n");
} /* PrintDelphiChoiceCodeClear */

void PrintDelphiEnumDefCodeJsonEnc(FILE *src, TypeDef *td/*, Type *seq*/)
{
	fprintf(src, "procedure T%s.JEnc(var json: TJSONValue);\n", td->definedName);// Prefix for class names
	fprintf(src, "begin\n");
	fprintf(src, "  json := TJSONNumber.Create(ord(value));\n");
	fprintf(src, "end;\n");
	fprintf(src, "\n");
	fprintf(src, "function T%s.JDec(json: TJSONValue): Boolean;\n", td->definedName);// Prefix for class names
	fprintf(src, "var\n");
	fprintf(src, "  tmp: Integer;\n");
	fprintf(src, "begin\n");
	fprintf(src, "  result := false;\n");
	fprintf(src, "  if json is TJsonNumber then\n");
	fprintf(src, "  begin\n");
	fprintf(src, "    tmp := TJSONNumber(json).AsInt;\n");
	fprintf(src, "    if (tmp >= ord(low(%sEnum))) and (tmp <= ord(high(%sEnum))) then\n", td->definedName, td->definedName);
	fprintf(src, "    begin\n");
	fprintf(src, "      self.value := %sEnum(tmp);\n", td->definedName);
	fprintf(src, "      result := true;\n");
	fprintf(src, "    end;\n");
	fprintf(src, "  end;\n");

	fprintf(src, "end;\n");
	fprintf(src, "\n");
}

void PrintDelphiSeqDefCodeJsonEnc(FILE *src, TypeDef *td, Type *seq)
{
	NamedType *e;
	char *varName;
	CxxTRI *cxxtri = NULL;

	fprintf(src, "procedure T%s.JEnc(var json: TJSONValue);\n", td->definedName);// Prefix for class names
	fprintf(src, "var\n");
	fprintf(src, "  tmp: TJsonValue;\n");
	fprintf(src, "begin\n");
	fprintf(src, "  json := TJSONObject.Create;\n");



	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		cxxtri = e->type->cxxTypeRefInfo;
		varName = cxxtri->fieldName;
		fprintf(src, "\n");
		if (e->type->optional)
		{
			fprintf(src, "  if Assigned(F%s) then\n", varName);
			fprintf(src, "  begin\n");
		}
		fprintf(src, "   F%s.JEnc(tmp);\n", varName);
		fprintf(src, "   TJSONObject(json).AddPair('%s', tmp);\n", varName);
		if (e->type->optional)
		{
			fprintf(src, "  end;\n");
		}
	}

	fprintf(src, "end;\n");
	fprintf(src, "\n");
	fprintf(src, "function T%s.JDec(json: TJSONValue; suppressErrMissing: Boolean; suppressErrDecode: Boolean): Boolean;\n", td->definedName);// Prefix for class names
	fprintf(src, "var\n");
	fprintf(src, "  jo:  TJsonObject;\n");
	fprintf(src, "  jv: TJsonValue;\n");
	fprintf(src, "begin\n");

	fprintf(src, "  Clear();\n");
	fprintf(src, "  Result := false;\n");
	fprintf(src, "  if json is TJsonObject then\n");
	fprintf(src, "  begin;\n");
	fprintf(src, "    jo := TJsonObject(json);\n");
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		cxxtri = e->type->cxxTypeRefInfo;
		varName = cxxtri->fieldName;
//		fprintf(src, "		F%s.JDec(jo.Get('%s').JsonValue);\n", varName, varName);
		fprintf(src, "\n");
		fprintf(src, "    if GetJsonValue(jo, '%s', jv) then\n", varName);
		fprintf(src, "    begin\n");
		if (e->type->optional)
		{
			fprintf(src, "      F%s := ", varName);
			PrintDelphiType(src, NULL, NULL, td, seq, e->type); // the both NULLs are not used in PrintDelphiType
			fprintf(src, ".Create();\n");
		}

		enum BasicTypeChoiceId basictype = BASICTYPE_UNKNOWN;
		if (e->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF && e->type->basicType->a.localTypeRef->link != NULL)
		{
			basictype = e->type->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId;
		}
		else if (e->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && e->type->basicType->a.importTypeRef->link != NULL)
		{
			basictype = e->type->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId;
		}
		
		if ((basictype == BASICTYPE_SEQUENCE) ||
			(basictype == BASICTYPE_SEQUENCEOF))
			fprintf(src, "      if (not F%s.JDec(jv, suppressErrMissing, suppressErrDecode)) and (not suppressErrDecode) then\n", varName);
		else
			fprintf(src, "      if (not F%s.JDec(jv)) and (not suppressErrDecode) then\n", varName);
		fprintf(src, "        raise EAsnTagDecodeException.Create('%s', '%s');\n", td->definedName, varName);
		fprintf(src, "    end");
		if (e->type->optional)
		{
			fprintf(src, ";\n");
		}
		else
		{
			fprintf(src, "\n");
			fprintf(src, "    else if not suppressErrMissing then\n");			
			fprintf(src, "      raise EAsnTagMissingException.Create('%s', '%s');\n", td->definedName, varName);
		}
	}
	fprintf(src, "\n");
	fprintf(src, "    Result := true;\n");
	fprintf(src, "  end;\n");

	fprintf(src, "end;\n");
	fprintf(src, "\n");
}

void PrintDelphiSetOfDefCodeJsonEnc(FILE *src, TypeDef *td, Type *lst)
{
	fprintf(src, "procedure T%s.JEnc(var json: TJSONValue);\n", td->definedName);// Prefix for class names
	fprintf(src, "var\n");
	fprintf(src, "  I: Integer;\n");
	fprintf(src, "  tmp: TJsonValue;\n");
	fprintf(src, "begin\n");
	fprintf(src, "  json := TJSONArray.Create;\n");
	fprintf(src, "  for I := 0 to Count -1 do\n");
	fprintf(src, "  begin\n");
	fprintf(src, "    self[I].JEnc(tmp);\n");
	fprintf(src, "    TJSONArray(json).AddElement(tmp);\n");
	fprintf(src, "  end;\n");

	fprintf(src, "end;\n");
	fprintf(src, "\n");
}

void PrintDelphiSetOfDefCodeJsonDec(FILE *src, TypeDef *td, Type *lst)
{
	fprintf(src, "function T%s.JDec(json: TJSONValue; suppressErrMissing: Boolean; suppressErrDecode: Boolean): Boolean;\n", td->definedName);// Prefix for class names
	fprintf(src, "var\n");
	//fprintf(src, "  jo:  TJsonObject;\n");
	fprintf(src, "  jv: TJsonValue;\n");
	fprintf(src, "  tmpObj: ");
	PrintDelphiType(src, NULL, NULL, td, lst, lst->basicType->a.sequenceOf);
	fprintf(src, ";\n");
	fprintf(src, "begin\n");

	fprintf(src, "  Clear();\n");
	fprintf(src, "  Result := false;\n");
	fprintf(src, "  if json is TJSONArray then\n");
	fprintf(src, "  begin\n");

	fprintf(src, "    for jv in TJsonArray(json) do\n");
	fprintf(src, "    begin\n");
	fprintf(src, "      tmpObj := "); 
	PrintDelphiType(src, NULL, NULL, td, lst, lst->basicType->a.sequenceOf);
	fprintf(src, ".Create(); \n");

	enum BasicTypeChoiceId basictype = BASICTYPE_UNKNOWN;
	if (lst->basicType->a.sequenceOf->basicType->choiceId == BASICTYPE_LOCALTYPEREF && lst->basicType->a.sequenceOf->basicType->a.localTypeRef->link != NULL)
	{
		basictype = lst->basicType->a.sequenceOf->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId;
	}
	else if (lst->basicType->a.sequenceOf->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && lst->basicType->a.sequenceOf->basicType->a.importTypeRef->link != NULL)
	{
		basictype = lst->basicType->a.sequenceOf->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId;
	}

	if ((basictype == BASICTYPE_SEQUENCE) ||
		(basictype == BASICTYPE_SEQUENCEOF))	
		fprintf(src, "      if tmpObj.JDec(jv, suppressErrMissing, suppressErrDecode) then\n");
	else
		fprintf(src, "      if tmpObj.JDec(jv) then\n");
	fprintf(src, "        self.Add(tmpObj)\n");
	fprintf(src, "      else if (not suppressErrDecode) then\n");
	fprintf(src, "        raise EAsnTagDecodeException.Create('%s', '');\n", td->definedName);
	fprintf(src, "    end;\n");
	fprintf(src, "    Result := true;\n");

	fprintf(src, "  end;\n");
	fprintf(src, "end;\n");
	fprintf(src, "\n");
}


void PrintDelphiChoiceDefCodeJsonEnc(FILE *src, TypeDef *td, Type *choice)
{
	NamedType *e;

	fprintf(src, "procedure T%s.JEnc(var json: TJSONValue);\n", td->definedName);// Prefix for class names
	fprintf(src, "var\n");
	fprintf(src, "  tmp: TJsonValue;\n");
	fprintf(src, "begin\n");
	fprintf(src, "  json := TJSONObject.Create;\n");

	fprintf(src, "  case choiceId of\n");
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.choice)
	{
		fprintf(src, "    ChoiceIdEnum.%s:\n", e->type->cxxTypeRefInfo->choiceIdSymbol);
		fprintf(src, "    begin\n");
		fprintf(src, "      if not assigned(FchoiceInternal.%s) then raise EAsnException.Create('Null pointer in choice: %s.%s');\n", e->fieldName, td->definedName, e->fieldName);
		fprintf(src, "      FchoiceInternal.%s.JEnc(tmp);\n", e->fieldName);
		fprintf(src, "      TJSONObject(json).AddPair('%s', tmp);\n", e->fieldName);
		fprintf(src, "    end;\n");
	}
	fprintf(src, "    else\n");
	fprintf(src, "      raise EAsnException.Create('choice %s is empty');\n", td->definedName);

	fprintf(src, "  end; // end case\n"); // end case

	fprintf(src, "end;\n");
	fprintf(src, "\n");
}

void PrintDelphiChoiceDefCodeJsonDec(FILE *src, TypeDef *td, Type *choice)
{
	NamedType *e;
	char *varName;
	CxxTRI *cxxtri = NULL;

	fprintf(src, "function T%s.JDec(json: TJSONValue): Boolean;\n", td->definedName);// Prefix for class names
	fprintf(src, "var\n");
	fprintf(src, "  jo: TJsonObject;\n");
	fprintf(src, "  jv: TJsonValue;\n");
	fprintf(src, "begin\n");
	fprintf(src, "  Result := false;\n");
	fprintf(src, "  Clear();\n");
	fprintf(src, "  if json is TJsonObject then\n");
	fprintf(src, "  begin;\n");
	fprintf(src, "    jo := TJsonObject(json);\n");
	fprintf(src, "\n");

	fprintf(src, "   "); // indent of 1st if
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.choice)
	{
		cxxtri = e->type->cxxTypeRefInfo;
		varName = cxxtri->fieldName;
		//		fprintf(src, "		F%s.JDec(jo.Get('%s').JsonValue);\n", varName, varName);
		fprintf(src, " if GetJsonValue(jo, '%s', jv) then\n", varName);
		fprintf(src, "    begin\n");
		fprintf(src, "      choiceId := ChoiceIdEnum.%s;\n", cxxtri->choiceIdSymbol);
		// delete in C++ (freeAndNil in delphi) not required - already freen by Clear();
		fprintf(src, "      FchoiceInternal.%s := ", varName);
		PrintDelphiType(src, NULL, NULL, td, choice, e->type); // the both NULLs are not used in PrintDelphiType
		fprintf(src, ".Create();\n");
		fprintf(src, "      if not FchoiceInternal.%s.JDec(jv) then\n", varName);
		fprintf(src, "        raise EAsnTagDecodeException.Create('%s', '%s');\n", td->definedName, varName);
		fprintf(src, "    end\n");
		fprintf(src, "    else");
	}
	fprintf(src, "\n");
	fprintf(src, "      raise EAsnException.Create('%s: No valid choice member');\n", td->definedName);
	fprintf(src, "\n");
	fprintf(src, "    Result := true;\n");
	fprintf(src, "  end;\n");

	fprintf(src, "end;\n");
	fprintf(src, "\n");

}
static void PrintDelphiPropertyImplementation(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *seq)
{
	NamedType *e;
	/* add getters/setters */
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		// OptionalParams ...
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION) {
			continue;
		}
		if (e->type->optional)
		{
			continue;
		}

		if (DelphiIsFinalType(e->type) || (DelphiIsEnumeratedType(e->type)))
		{
			fprintf(src, "function T%s.Get%s: ", td->definedName, e->fieldName);// Prefix for class names
			if (DelphiIsFinalType(e->type))
				PrintDelphiNativeType(src, e->type->basicType->choiceId);
			else
			{
				PrintDelphiEnumType(src, mods, m, td, seq, e->type);
			}
			fprintf(src, ";\n");
			fprintf(src, "begin\n");
			fprintf(src, "  Result := F%s.value;\n", e->fieldName);
			fprintf(src, "end;\n");
			fprintf(src, "procedure T%s.Set%s(const Value: ", td->definedName, e->fieldName);// Prefix for class names

			if (DelphiIsFinalType(e->type))
				PrintDelphiNativeType(src, e->type->basicType->choiceId);
			else
			{
				PrintDelphiEnumType(src, mods, m, td, seq, e->type);
			}

			fprintf(src, ");\n");
			fprintf(src, "begin\n");
			fprintf(src, "  F%s.value := Value;\n", e->fieldName);
			fprintf(src, "end;\n");
			fprintf(src, "\n");
		}
	}

}

static void PrintDelphiImplementationCode(FILE *src, Module *m, TypeDef *td)
{

	// class name comment: { ClassName }
	fprintf(src, "{ %s }\n", td->definedName);
	fprintf(src, "\n"); 
	switch (td->type->basicType->choiceId)
	{
		case BASICTYPE_ENUMERATED:  /* library type */
			PrintDelphiEnumDefCodeJsonEnc(src, td/*, td->type*/);
			break;
		case BASICTYPE_SEQUENCE:

			PrintDelphiSeqConstructor(src, td, td->type);
			PrintDelphiDestructor(src, td, td->type);
			PrintDelphiSeqDefClear(src, td, td->type);

			PrintDelphiPropertyImplementation(src, NULL, m, td, td->type);


			PrintDelphiSeqDefCodeJsonEnc(src, td, td->type);
			break;
		case BASICTYPE_CHOICE:
			PrintDelphiChoiceConstructor(src, td, td->type);
			PrintDelphiChoiceDestructor(src, td, td->type);
			PrintDelphiChoiceCodeClear(src, td, td->type);

			PrintDelphiChoiceDefCodeJsonEnc(src, td, td->type);
			PrintDelphiChoiceDefCodeJsonDec(src, td, td->type);
			break;
		case BASICTYPE_SEQUENCEOF:  /* list types */
		case BASICTYPE_SETOF:

			PrintDelphiSetOfDefCodeJsonEnc(src, td, td->type);
			PrintDelphiSetOfDefCodeJsonDec(src, td, td->type);
			break;
		default:
			break;
	}
	//switch (td->type->basicType->choiceId)
	//{
	//case BASICTYPE_BITSTRING:  /* library type */
	//case BASICTYPE_CHOICE:
	//case BASICTYPE_ENUMERATED:  /* library type */
	//case BASICTYPE_SEQUENCE:
	//	
	//	// Datentypen, die in JS nicht als eigenes HumanModel abgebildet werden.
	//	if (strcmp(td->definedName, "UTF8StringList") == 0 
	//		|| strcmp(td->definedName, "SEQInteger") == 0 
	//		|| strcmp(td->definedName, "AsnContactIDs") == 0
	//		|| strcmp(td->definedName, "AsnContactID") == 0
	//		|| strcmp(td->definedName, "AsnOptionalParameters") == 0) {
	//		break;
	//	}

	//	fprintf(src, "\texports.%s= %s;\n", td->definedName, td->definedName);

	//	//if (td != (TypeDef *)LAST_LIST_ELMT(m->typeDefs))
	//	//	fprintf(src, ",\n");
	//	//else
	//	//	fprintf(src, "\n");

	//	break;
	//case BASICTYPE_SEQUENCEOF:  /* list types */
	//case BASICTYPE_SETOF:
	//	
	//	break;
	//default:
	//	break;
	//}
} /* PrintDelphiImplementationCode */


/*
* prints PrintROSEInvoke
*/
//static void PrintDelphiROSEInvoke(FILE *hdr, Module *m, int bEvents, ValueDef *vd)
//{
//	hdr = hdr;
//	vd = vd;
//	bEvents = bEvents;
//	m = m;
//
//} /* PrintROSEInvoke */

static void PrintDelphiTypeDecl(FILE *f, TypeDef *td)
{
	//fprintf(f, "// [PrintDelphiTypeDecl] %s\n", td->cxxTypeDefInfo->className);

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
			fprintf(f, " T%s = class;\n", td->cxxTypeDefInfo->className); // Prefix for class names
	}

}

void PrintDelphiImports(FILE *src, ModuleList *mods, Module *m)
{
	Module *currMod;
	AsnListNode *currModTmp;

	fprintf(src, "// Global imports\n");
	fprintf(src, "uses\n");
	fprintf(src, "  System.Sysutils,\n");

	fprintf(src, "  // starting with Delphi XE6 the TJSON* classes reside in System.JSON\n");
	fprintf(src, "  {$IF CompilerVersion >= 27}\n");
	fprintf(src, "  System.JSON,\n");
	fprintf(src, "  {$ELSE}\n");
	fprintf(src, "  Data.DBXJson,\n");
	fprintf(src, "  {$IFEND}\n");

	fprintf(src, "  DelphiAsn1Types");

	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		if ((strcmp(m->delphiFileName, currMod->delphiFileName) == 0))
		{
			// Code to see the import module list AND load possible "namespace" refs.
			ImportModuleList *ModLists;
			ImportModule *impMod;
	
			ModLists = currMod->imports;
			currModTmp = mods->curr;    //RWC;
			FOR_EACH_LIST_ELMT(impMod, ModLists)
			{
				ImportElmt *impElmt;

				//fprintf(src, "  // Imports from %s\n", impMod->modId->name);

				if (impMod->moduleRef == NULL)
					impMod->moduleRef = GetImportModuleRef(impMod->modId->name, mods);
				int importRequired = FALSE;
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
					else 
					{
						importRequired = TRUE;
						break;
					}
				}
				if (importRequired)
				{
					fprintf(src, ",\n");
					fprintf(src, "  // Imports from %s\n", impMod->modId->name);
					fprintf(src, "  %s", impMod->moduleRef->moduleName);
				}

			}
			mods->curr = currModTmp;    // RWC;RESET loop control
		}
	}
	fprintf(src, ";\n");
	fprintf(src, "\n");
}

void PrintDelphiComments(FILE *src, Module *m) {
	fprintf(src, "{\n");
	fprintf(src, " * %s\n", RemovePath(m->delphiFileName));
	fprintf(src, " * \"%s\" ASN.1 stubs.\n", m->modId->name);
	write_snacc_header(src, " * ");
	fprintf(src, " * }\n\n");
}

//void PrintDelphiROSECode(FILE *src, ModuleList *mods, Module *m)
//{
//	src = src;
//	m = m;
//	mods = mods;
//}


void PrintDelphiCode(FILE *src, ModuleList *mods, Module *m)
{
	TypeDef *td;

	fprintf(src, "// [PrintDelphiCode]\n");
	
	fprintf(src, "unit %s;\n", m->moduleName);

	// Comments
	PrintDelphiComments(src, m);

	fprintf(src, "\n\n");
	fprintf(src, "interface\n");

	// Uses/Includes
	PrintDelphiImports(src, mods, m);
	
	fprintf(src, "{$SCOPEDENUMS ON}\n"); // required to prevent collisions between enumerations
	//fprintf(src, "//------------------------------------------------------------------------------\n");
	fprintf(src, "type\n");
	fprintf(src, "// class forward declarations:\n");
	FOR_EACH_LIST_ELMT(td, m->typeDefs)
		PrintDelphiTypeDecl(src, td);
	fprintf(src, "\n");

	fprintf(src, "// class declarations:\n");
	FOR_EACH_LIST_ELMT (td, m->typeDefs)
		PrintDelphiTypeDefCode (src, mods, m, td);


	fprintf(src, "\n");
	fprintf(src, "implementation\n");
	fprintf(src, "// [PrintDelphiImplementationCode]\n");

	FOR_EACH_LIST_ELMT(td, m->typeDefs)
		PrintDelphiImplementationCode(src, m, td);

	fprintf(src, "end.\n");
} /* PrintDelphiCode */

/* EOF gen-code.c (for back-ends/delphi-gen) */

