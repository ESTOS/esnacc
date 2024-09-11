/*
 * compiler/back_ends/idl_gen/types.c  - fills in IDL type information
 *
 * MS 91/92
 * Copyright (C) 1991, 1992 Michael Sample
 *           and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/back-ends/idl-gen/types.c,v 1.1.1.1 2005/04/14 14:59:43 \ste Exp $
 * $Log: types.c,v $
 * Revision 1.1.1.1  2005/04/14 14:59:43  \ste
 * no message
 *
 * Revision 1.7  2004/03/25 19:20:16  gronej
 * fixed some linux warnings
 *
 * Revision 1.6  2004/01/14 19:07:53  gronej
 * Updated Compiler to accept and process relative-oid's
 *
 * Revision 1.5  2003/07/07 14:51:57  nicholar
 * Eliminated headers and cleaned up include references
 *
 * Revision 1.4  2002/09/16 17:53:53  mcphersc
 * Fixed warnings
 *
 * Revision 1.3  2002/09/04 18:02:27  vracarl
 * got rid of c++ comments
 *
 * Revision 1.2  2000/10/24 14:54:49  rwc
 * Updated to remove high-level warnings (level 4 on MSVC++) for an easier build.
 * SOME warnings persist due to difficulty in modifying the SNACC compiler to
 * properly build clean source; also some files are built by Lex/Yacc.
 *
 * Revision 1.1.1.1  2000/08/21 20:36:04  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.1  1997/01/01 20:25:40  rj
 * first draft
 *
 */

#include <ctype.h>
#include <string.h>
#include "../../../c-lib/include/asn-incl.h"
#include "../../core/asn1module.h"
#include "../../core/snacc-util.h"
#include "../str-util.h"
#include "rules.h"

static DefinedObj* definedNamesG;

/* unexported prototypes */

void FillIDLTypeDefInfo PROTO((IDLRules * r, Module* m, TypeDef* td));

static void FillIDLFieldNames PROTO((IDLRules * r, NamedTypeList* firstSibling));

static void FillIDLTypeRefInfo PROTO((IDLRules * r, Module* m, TypeDef* td, Type* parent, Type* t));

static void FillIDLStructElmts PROTO((IDLRules * r, Module* m, TypeDef* td, Type* parent, NamedTypeList* t));

static void FillIDLChoiceElmts PROTO((IDLRules * r, Module* m, TypeDef* td, Type* parent, NamedTypeList* first));

static int IsIDLPtr PROTO((IDLRules * r, TypeDef* td, Type* parent, Type* t));

void FillIDLTDIDefaults PROTO((IDLRules * r, IDLTDI* ctdi, TypeDef* td));

/*
 *  allocates and fills all the idlTypeInfos
 *  in the type trees for every module in the list
 */
void FillIDLTypeInfo PARAMS((r, modList), IDLRules* r _AND_ ModuleList* modList)
{
	TypeDef* td;
	Module* m;

	/*
	 * go through each module's type defs and fill
	 * in the C type and enc/dec routines etc
	 */
	definedNamesG = NULL;

	FOR_EACH_LIST_ELMT(m, modList)
	{
		FOR_EACH_LIST_ELMT(td, m->typeDefs)
		FillIDLTypeDefInfo(r, m, td);
	}

	/*
	 * now that type def info is filled in
	 * set up set/seq/list/choice elements that ref
	 * those definitions
	 */
	FOR_EACH_LIST_ELMT(m, modList)
	{
		FOR_EACH_LIST_ELMT(td, m->typeDefs)
		FillIDLTypeRefInfo(r, m, td, NULL, td->type);
	}

	/*
	 * modules compiled together (ie one call to snacc with
	 * multiple args) likely to be C compiled together so
	 * need a unique routines/types/defines/enum values
	 * since assuming they share same name space.
	 *  All Typedefs, union, struct & enum Tags, and defined values
	 * (enum consts), #define names
	 *  are assumed to share the same name space
	 */

	/* done with checking for name conflicts */
	FreeDefinedObjs(&definedNamesG);

} /* FillIDLTypeInfo */

/*
 *  allocates and fills structure holding C type definition information
 *  fo the given ASN.1 type definition.  Does not fill CTRI for contained
 *  types etc.
 */
void FillIDLTypeDefInfo PARAMS((r, m, td), IDLRules* r _AND_ Module* m _AND_ TypeDef* td)
{
	char* tmpName;
	IDLTDI* idltdi;

	/*
	 * if IDLTDI is present this type def has already been 'filled'
	 */
	if (td->idlTypeDefInfo != NULL)
		return;

	idltdi = MT(IDLTDI);
	td->idlTypeDefInfo = idltdi;

	/* get default type def attributes from table for type on rhs of ::= */

	FillIDLTDIDefaults(r, idltdi, td);

	/*
	 * if defined by a ref to another type definition fill in that type
	 * def's IDLTDI so can inherit (actully completly replace default
	 * attributes) from it
	 */
	if ((td->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF) || (td->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF))
	{
		/*
		 * Fill in IDLTDI for defining type if nec.
		 * this works for importTypeRef as well since both a.localTypeRef
		 * and a.importTypeRef are of type TypeRef
		 */
		FillIDLTypeDefInfo(r, td->type->basicType->a.localTypeRef->module, td->type->basicType->a.localTypeRef->link);

		tmpName = idltdi->typeName; /* save typeName */
		/* copy all type def info and restore name related stuff - hack*/
		*idltdi = *td->type->basicType->a.localTypeRef->link->idlTypeDefInfo;
		idltdi->typeName = tmpName; /* restore typeName */
	}

	/*
	 * check for any "--snacc" attributes that overide the current
	 * idltdi fields
	 * UNDEFINED FOR C++
	ParseTypeDefAttribs (idltdi, td->attrList);
	 */
} /* FillIDLTypeDefInfo */

static void FillIDLTypeRefInfo PARAMS((r, m, td, parent, t), IDLRules* r _AND_ Module* m _AND_ TypeDef* td _AND_ Type* parent _AND_ Type* t)
{
	IDLTRI* idltri;
	IDLTDI* tmpidltdi;
	ValueDef* namedElmt;
	CNamedElmt* cne;
	CNamedElmt** cneHndl;
	char* elmtName;
	size_t len;
	enum BasicTypeChoiceId basicTypeId;

	/*
	 * you must check for cycles yourself before calling this
	 */
	if (t->idlTypeRefInfo == NULL)
	{
		idltri = MT(IDLTRI);
		t->idlTypeRefInfo = idltri;
	}
	else
		idltri = t->idlTypeRefInfo;

	basicTypeId = t->basicType->choiceId;

	tmpidltdi = &r->typeConvTbl[basicTypeId];

	/* get base type def info from the conversion table in the rules */
	idltri->isEnc = tmpidltdi->isEnc;
	idltri->typeName = tmpidltdi->typeName;
	idltri->optTestRoutineName = tmpidltdi->optTestRoutineName;

	/*
	 * convert named elmts to IDL names.
	 * check for name conflict with other defined Types/Names/Values
	 */
	if ((basicTypeId == BASICTYPE_INTEGER || basicTypeId == BASICTYPE_ENUMERATED || basicTypeId == BASICTYPE_BITSTRING) && !(LIST_EMPTY(t->basicType->a.integer)))
	{
		idltri->namedElmts = AsnListNew(sizeof(void*));
		FOR_EACH_LIST_ELMT(namedElmt, t->basicType->a.integer)
		{
			cneHndl = (CNamedElmt**)AsnListAppend(idltri->namedElmts);
			cne = *cneHndl = MT(CNamedElmt);
			elmtName = Asn1ValueName2CValueName(namedElmt->definedName);
			len = strlen(elmtName);
			size_t size = len + 1 + r->maxDigitsToAppend;
			cne->name = Malloc(size + 1);
			strcpy_s(cne->name, size, elmtName);
			Free(elmtName); /* not very efficient */

			if (namedElmt->value->basicValue->choiceId == BASICVALUE_INTEGER)
				cne->value = namedElmt->value->basicValue->a.integer;
			else
			{
				fprintf(errFileG, "Warning: unlinked defined value. Using -9999999\n");
				cne->value = -9999999;
			}

			if (r->capitalizeNamedElmts)
				Str2UCase(cne->name, len);

			/*
			 * append digits if enum value name is a keyword
			 */
			MakeCxxStrUnique(definedNamesG, cne->name, size, r->maxDigitsToAppend, 1);
			DefineObj(&definedNamesG, cne->name);
		}
	}

	/* fill in rest of type info depending on the type */
	switch (basicTypeId)
	{
		case BASICTYPE_BOOLEAN: /* library types */
		case BASICTYPE_INTEGER:
		case BASICTYPE_BITSTRING:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_NULL:
		case BASICTYPE_OID:
		case BASICTYPE_RELATIVE_OID:
		case BASICTYPE_REAL:
		case BASICTYPE_ENUMERATED:
			/* don't need to do anything else */
			break;

		case BASICTYPE_SEQUENCEOF: /* list types */
		case BASICTYPE_SETOF:
			/* fill in component type */
			FillIDLTypeRefInfo(r, m, td, t, t->basicType->a.setOf);
			break;

		case BASICTYPE_IMPORTTYPEREF: /* type references */
		case BASICTYPE_LOCALTYPEREF:
			/*
			 * grab class name from link (link is the def of the
			 * the ref'd type)
			 */
			if (t->basicType->a.localTypeRef->link != NULL)
			{
				/* inherit attributes from referenced type */
				tmpidltdi = t->basicType->a.localTypeRef->link->idlTypeDefInfo;
				idltri->typeName = tmpidltdi->typeName;
				idltri->isEnc = tmpidltdi->isEnc;
				idltri->optTestRoutineName = tmpidltdi->optTestRoutineName;
			}

			break;

		case BASICTYPE_ANYDEFINEDBY: /* ANY types */
			break;					 /* these are handled now */

		case BASICTYPE_ANY:
#if 0
		PrintErrLoc(m->asn1SrcFileName, t->lineNo);
		fprintf(errFileG, "Warning - generated code for the \"ANY\" type in type \"%s\" will need modification by YOU.", td->definedName);
		fprintf(errFileG, "  The source files will have a \"/* ANY - Fix Me! */\" comment before related code.\n\n");
#endif

			break;

		case BASICTYPE_CHOICE:
			/*
			 * must fill field names BEFORE filling choice elmts
			 * (allows better naming for choice ids)
			 */
			FillIDLFieldNames(r, t->basicType->a.choice);
			FillIDLChoiceElmts(r, m, td, t, t->basicType->a.choice);
			break;

		case BASICTYPE_SET:
		case BASICTYPE_SEQUENCE:
			FillIDLStructElmts(r, m, td, t, t->basicType->a.set);
			FillIDLFieldNames(r, t->basicType->a.set);
			break;

		case BASICTYPE_COMPONENTSOF:
		case BASICTYPE_SELECTION:
			fprintf(errFileG, "Compiler error - COMPONENTS OF or SELECTION type slipped through normalizing phase.\n");
			break;

		case BASICTYPE_UNKNOWN:
		case BASICTYPE_MACRODEF:
		case BASICTYPE_MACROTYPE:
			/* do nothing */
			break;

		default:
			break;
	}

	/*
	 * figure out whether this is a ptr based on the enclosing
	 * type (if any) and optionality/default
	 */
	idltri->isPtr = (unsigned char)IsIDLPtr(r, td, parent, t);

	/* let user overide any defaults with the --snacc attributes */
	/* undefined for C++ ParseTypeRefAttribs (ctri, t->attrList); */

} /* FillIDLTypeRefInfo */

static void FillIDLStructElmts PARAMS((r, m, td, parent, elmts), IDLRules* r _AND_ Module* m _AND_ TypeDef* td _AND_ Type* parent _AND_ NamedTypeList* elmts)
{
	NamedType* et;

	FOR_EACH_LIST_ELMT(et, elmts)
	{
		FillIDLTypeRefInfo(r, m, td, parent, et->type);
	}

} /* FillIDLStructElmts */

/*
 *  Figures out non-conflicting enum names for the
 *  choice id's
 */
static void FillIDLChoiceElmts PARAMS((r, m, td, parent, elmts), IDLRules* r _AND_ Module* m _AND_ TypeDef* td _AND_ Type* parent _AND_ NamedTypeList* elmts)
{
	NamedType* et;
	int idCount = 0;
	IDLTRI* idltri;
	size_t len;

	/*
	 * fill in type info for elmt types first
	 */
	FOR_EACH_LIST_ELMT(et, elmts)
	FillIDLTypeRefInfo(r, m, td, parent, et->type);

	/*
	 * set choiceId Symbol & value
	 * eg
	 *  Car ::= CHOICE {          enum CarChoice {
	 *     chev ChevCar,              carChoice_chev,
	 *     ford FordCar,              carChoice_ford,
	 *     toyota ToyotaCar           carChoice_toyota
	 *  }                         };
	 *                            union Car switch (CarChoice) {
	 *                                ChevCar *chev;
	 *                                FordCar *ford;
	 *                                ToyotaCar *toyota; };
	 *                            };
	 * NOTE that the union is anonymous
	 */
	FOR_EACH_LIST_ELMT(et, elmts)
	{
		idltri = et->type->idlTypeRefInfo;

		if (idltri == NULL)
			continue; /* wierd type */

		idltri->choiceIdValue = idCount++;

		len = strlen(td->idlTypeDefInfo->typeName) + strlen(idltri->fieldName);
		size_t size = len + 6 + 1;
		idltri->choiceIdSymbol = Malloc(size + 1);
		strcpy_s(idltri->choiceIdSymbol, size, td->idlTypeDefInfo->typeName);
		strcat_s(idltri->choiceIdSymbol, size, "Choice_");
		strcat_s(idltri->choiceIdSymbol, size, idltri->fieldName);

		if (r->capitalizeNamedElmts)
			Str2UCase(idltri->choiceIdSymbol, len);

		Str2LCase(idltri->choiceIdSymbol, 1);
	}

} /* FillIDLChoiceElmts */

/*
 * takes a list of "sibling" (eg same level in a structure)
 * ElmtTypes and fills sets up the c field names in
 * the IDLTRI struct
 */
static void FillIDLFieldNames PARAMS((r, elmts), IDLRules* r _AND_ NamedTypeList* elmts)
{
	NamedType* et;
	IDLTRI* idltri;
	DefinedObj* fieldNames;
	char* tmpName;
	char* asn1FieldName;
	char* cFieldName;

	/*
	 * Initialize fieldname data
	 * allocate (if nec) and fill in CTRI fieldname if poss
	 * from asn1 field name.  leave blank otherwise
	 */
	fieldNames = NewObjList();
	FOR_EACH_LIST_ELMT(et, elmts)
	{
		idltri = et->type->idlTypeRefInfo;
		if (idltri == NULL)
		{
			idltri = MT(IDLTRI);
			et->type->idlTypeRefInfo = idltri;
		}
		if (idltri && et->fieldName != NULL)
		{
			/*
			 * can assume that the field names are
			 * distinct because they have passed the
			 * error checking step.
			 * However, still call MakeCxxStrUnique
			 * to change any field names that
			 * conflict with C++ keywords
			 */
			asn1FieldName = et->fieldName;
			tmpName = Asn1FieldName2CFieldName(asn1FieldName);
			size_t size = strlen(tmpName) + 1 + r->maxDigitsToAppend;
			idltri->fieldName = Malloc(size + 1);
			strcpy_s(idltri->fieldName, size, tmpName);
			Free(tmpName);

			/*   old    idltri->fieldName = Asn1FieldName2CFieldName (asn1FieldName); */

			MakeCxxStrUnique(fieldNames, idltri->fieldName, size, r->maxDigitsToAppend, 1);
			DefineObj(&fieldNames, idltri->fieldName);
		}
	}

	FOR_EACH_LIST_ELMT(et, elmts)
	{
		idltri = et->type->idlTypeRefInfo;

		/*
		 * generate field names for those without them
		 */
		if (idltri && idltri->fieldName == NULL)
		{
			size_t size = 0;
			if ((et->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF) || (et->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF))
			{
				/*
				 * take ref'd type name as field name
				 * convert first let to lower case
				 */
				tmpName = et->type->basicType->a.localTypeRef->link->idlTypeDefInfo->typeName;
				tmpName = Asn1TypeName2CTypeName(tmpName);
				size = strlen(tmpName) + r->maxDigitsToAppend + 1;
				cFieldName = Malloc(size + 1);
				strcpy_s(cFieldName, size, tmpName);
				Free(tmpName);
				if (isupper(cFieldName[0]))
					cFieldName[0] = (char)tolower(cFieldName[0]);
			}
			else
			{
				/*
				 * get default field name for this type
				 */
				tmpName = r->typeConvTbl[et->type->basicType->choiceId].defaultFieldName;
				size = strlen(tmpName) + r->maxDigitsToAppend + 1;
				cFieldName = Malloc(size + 1);
				strcpy_s(cFieldName, size, tmpName);

				if (cFieldName && isupper(cFieldName[0]))
					cFieldName[0] = (char)tolower(cFieldName[0]);
			}

			/*
			 * try to use just the type name (with lower case first char).
			 * if that is already used in this type or a C++ keyword,
			 * append ascii digits to field name until unique
			 * in this type
			 */
			MakeCxxStrUnique(fieldNames, cFieldName, size, r->maxDigitsToAppend, 1);
			DefineObj(&fieldNames, cFieldName);
			idltri->fieldName = cFieldName;
		}
	}
	FreeDefinedObjs(&fieldNames);
} /* FillIDLFieldNames */

/*
 * returns true if this c type for this type should be
 * be ref'd as a ptr
 */
static int IsIDLPtr PARAMS((r, td, parent, t), IDLRules* r _AND_ TypeDef* td _AND_ Type* parent _AND_ Type* t)
{
	IDLTDI* idltdi;
	int retVal = FALSE;

	/*
	 * inherit ptr attriubutes from ref'd type if any
	 * otherwise grab lib c type def from the IDLRules
	 */
	if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF) || (t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF))
		idltdi = t->basicType->a.localTypeRef->link->idlTypeDefInfo;
	else
		idltdi = &r->typeConvTbl[GetBuiltinType(t)];

	/* no parent means t is the root of a typedef */
	if ((parent == NULL) && (idltdi->isPtrForTypeDef))
		retVal = TRUE;

	else if ((parent != NULL) && ((parent->basicType->choiceId == BASICTYPE_SET) || (parent->basicType->choiceId == BASICTYPE_SEQUENCE)) && (idltdi->isPtrInSetAndSeq))
		retVal = TRUE;

	else if ((parent != NULL) && ((parent->basicType->choiceId == BASICTYPE_SETOF) || (parent->basicType->choiceId == BASICTYPE_SEQUENCEOF)) && (idltdi->isPtrInList))
		retVal = TRUE;

	else if ((parent != NULL) && (parent->basicType->choiceId == BASICTYPE_CHOICE) && (idltdi->isPtrInChoice))
		retVal = TRUE;

	else if (((t->optional) || (t->defaultVal != NULL)) && (idltdi->isPtrForOpt))
		retVal = TRUE;

	return retVal;
} /* IsIDLPtr */

/* fill given idltdi with defaults from table for given typedef */
void FillIDLTDIDefaults PARAMS((r, idltdi, td), IDLRules* r _AND_ IDLTDI* idltdi _AND_ TypeDef* td)
{
	IDLTDI* tblidltdi;
	int typeIndex;
	char* tmpName;

	typeIndex = GetBuiltinType(td->type);

	if (typeIndex < 0)
		return;

	tblidltdi = &r->typeConvTbl[typeIndex];

	memcpy(idltdi, tblidltdi, sizeof(IDLTDI));

	/* make sure class name is unique wrt to previously defined classes */
	tmpName = Asn1TypeName2CTypeName(td->definedName);
	size_t size = strlen(tmpName) + 2 + r->maxDigitsToAppend + 1;
	idltdi->typeName = Malloc(size + 1);
	strcpy_s(idltdi->typeName, size, tmpName);
	if (tblidltdi->asn1TypeId != BASICTYPE_CHOICE)
		strcat_s(idltdi->typeName, size, "_T");
	Free(tmpName);

	MakeCxxStrUnique(definedNamesG, idltdi->typeName, size, r->maxDigitsToAppend, 1);
	DefineObj(&definedNamesG, idltdi->typeName);

} /* FillIDLTDIDefaults */
