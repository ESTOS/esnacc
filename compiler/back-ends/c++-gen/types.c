/*
 * compiler/back-ends/c++-gen/types.c  - fills in c++ type information
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
 * INSERT_VDA_COMMENTS
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/back-ends/c++-gen/types.c,v 1.1.1.1 2005/04/14 14:59:42 \ste Exp $
 * $Log: types.c,v $
 * Revision 1.1.1.1  2005/04/14 14:59:42  \ste
 * no message
 *
 * Revision 1.12  2004/03/25 19:20:16  gronej
 * fixed some linux warnings
 *
 * Revision 1.11  2004/01/14 19:07:53  gronej
 * Updated Compiler to accept and process relative-oid's
 *
 * Revision 1.10  2003/12/17 19:05:04  gronej
 * SNACC baseline merged with PER v1_7 tag
 *
 * Revision 1.9.2.1  2003/12/04 16:11:32  gronej
 * Fixed bitsDecoded to return correct value
 *
 * Revision 1.9  2003/07/07 14:52:35  nicholar
 * Eliminated headers and cleaned up include references
 *
 * Revision 1.8  2002/10/28 19:57:03  leonberp
 * Added BITSTRING CONTAINING support and fixed CONTAINED ANY DEFINED BY
 *
 * Revision 1.7  2002/10/24 21:07:21  leonberp
 * latest fixing for OCTET CONTAINING
 *
 * Revision 1.6  2002/09/16 17:48:19  mcphersc
 * Fixed warnings
 * Z
 *
 * Revision 1.5  2002/09/04 17:53:24  vracarl
 * got rid of c++ comments
 *
 * Revision 1.4  2002/05/15 14:53:12  leonberp
 * added support for new basicTypes to compiler
 *
 * Revision 1.3  2001/01/02 14:33:24  rwc
 * Updated to remove last print on VDA DER Rules when compiling.  Cleaner builds by application running compiler.
 *
 * Revision 1.2  2000/10/24 14:54:46  rwc
 * Updated to remove high-level warnings (level 4 on MSVC++) for an easier build.
 * SOME warnings persist due to difficulty in modifying the SNACC compiler to
 * properly build clean source; also some files are built by Lex/Yacc.
 *
 * Revision 1.1.1.1  2000/08/21 20:36:06  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.4  1995/07/25 18:25:11  rj
 * file name has been shortened for redundant part: c++-gen/c++-types -> c++-gen/types.
 *
 * changed `_' to `-' in file names.
 *
 * Revision 1.3  1994/10/08  03:47:51  rj
 * since i was still irritated by cpp standing for c++ and not the C preprocessor, i renamed them to cxx (which is one known suffix for C++ source files). since the standard #define is __cplusplus, cplusplus would have been the more obvious choice, but it is a little too long.
 *
 * Revision 1.2  1994/09/01  01:06:02  rj
 * snacc_config.h removed.
 *
 * Revision 1.1  1994/08/28  09:47:56  rj
 * first check-in. for a list of changes to the snacc-1.1 distribution please refer to the ChangeLog.
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

void FillCxxTypeDefInfo PROTO((CxxRules * r, Module* m, TypeDef* td));

static void FillCxxFieldNames PROTO((CxxRules * r, NamedTypeList* firstSibling));

static void FillCxxTypeRefInfo PROTO((CxxRules * r, Module* m, TypeDef* head, Type* parent, Type* t));

static void FillCxxStructElmts PROTO((CxxRules * r, Module* m, TypeDef* head, Type* parent, NamedTypeList* t));

static void FillCxxChoiceElmts PROTO((CxxRules * r, Module* m, TypeDef* head, Type* parent, NamedTypeList* first));

static int IsCxxPtr PROTO((CxxRules * r, TypeDef* td, Type* parent, Type* t));

void FillCxxTDIDefaults PROTO((CxxRules * r, CxxTDI* ctdi, TypeDef* td));

/*
 *  allocates and fills all the cxxTypeInfos
 *  in the type trees for every module in the list
 */
void FillCxxTypeInfo(CxxRules* r, ModuleList* modList)
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
		FillCxxTypeDefInfo(r, m, td);
	}

	/*
	 * now that type def info is filled in
	 * set up set/seq/list/choice elements that ref
	 * those definitions
	 */
	FOR_EACH_LIST_ELMT(m, modList)
	{
		FOR_EACH_LIST_ELMT(td, m->typeDefs)
		FillCxxTypeRefInfo(r, m, td, NULL, td->type);
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

} /* FillCxxTypeInfo */

/*
 *  allocates and fills structure holding C type definition information
 *  fo the given ASN.1 type definition.  Does not fill CTRI for contained
 *  types etc.
 */
void FillCxxTypeDefInfo PARAMS((r, m, td), CxxRules* r _AND_ Module* m _AND_ TypeDef* td)
{
	char* tmpName;
	CxxTDI* cxxtdi;

	/*
	 * if CxxTDI is present this type def has already been 'filled'
	 */
	if (td->cxxTypeDefInfo != NULL)
		return;

	cxxtdi = MT(CxxTDI);
	td->cxxTypeDefInfo = cxxtdi;

	/* get default type def attributes from table for type on rhs of ::= */

	FillCxxTDIDefaults(r, cxxtdi, td);

	/*
	 * if defined by a ref to another type definition fill in that type
	 * def's CxxTDI so can inherit (actully completly replace default
	 * attributes) from it
	 */
	if ((td->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF) || (td->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF))
	{
		/*
		 * Fill in CxxTDI for defining type if nec.
		 * this works for importTypeRef as well since both a.localTypeRef
		 * and a.importTypeRef are of type TypeRef
		 */
		FillCxxTypeDefInfo(r, td->type->basicType->a.localTypeRef->module, td->type->basicType->a.localTypeRef->link);

		tmpName = cxxtdi->className; /* save className */
		/* copy all type def info and restore name related stuff - hack*/
		*cxxtdi = *td->type->basicType->a.localTypeRef->link->cxxTypeDefInfo;
		cxxtdi->className = tmpName; /* restore className */
	}

	/*
	 * check for any "--snacc" attributes that overide the current
	 * cxxtdi fields
	 * UNDEFINED FOR C++
	ParseTypeDefAttribs (cxxtdi, td->attrList);
	*/

} /* FillCxxTypeDefInfo */

static void FillCxxTypeRefInfo PARAMS((r, m, head, parent, t), CxxRules* r _AND_ Module* m _AND_ TypeDef* head _AND_ Type* parent _AND_ Type* t)
{
	CxxTRI* cxxtri;
	CxxTDI* tmpCxxtdi;
	ValueDef* namedElmt;
	CNamedElmt* cne;
	CNamedElmt** cneHndl;
	char* elmtName;
	size_t len;
	enum BasicTypeChoiceId basicTypeId;

	/*
	 * you must check for cycles yourself before calling this
	 */
	if (t->cxxTypeRefInfo == NULL)
	{
		cxxtri = MT(CxxTRI);
		t->cxxTypeRefInfo = cxxtri;
	}
	else
		cxxtri = t->cxxTypeRefInfo;

	basicTypeId = t->basicType->choiceId;

	tmpCxxtdi = &r->typeConvTbl[basicTypeId];

	/* get base type def info from the conversion table in the rules */
	cxxtri->isEnc = tmpCxxtdi->isEnc;

	// ste use OCTETCONTAINING as OCTET STRING -- (basicTypeId == BASICTYPE_OCTETCONTAINING) ||
	if (basicTypeId == BASICTYPE_BITCONTAINING)
	{
		// Check for Containing Binary
		cxxtri->className = r->typeConvTbl[t->basicType->a.stringContaining->basicType->choiceId].className;
	}
	else
		cxxtri->className = tmpCxxtdi->className;
	cxxtri->optTestRoutineName = tmpCxxtdi->optTestRoutineName;

	/*
	 * convert named elmts to C++ names.
	 * check for name conflict with other defined Types/Names/Values
	 */
	if (((basicTypeId == BASICTYPE_INTEGER) || (basicTypeId == BASICTYPE_ENUMERATED) || (basicTypeId == BASICTYPE_BITSTRING)) && !(LIST_EMPTY(t->basicType->a.integer)))
	{
		cxxtri->namedElmts = AsnListNew(sizeof(void*));
		FOR_EACH_LIST_ELMT(namedElmt, t->basicType->a.integer)
		{
			cneHndl = (CNamedElmt**)AsnListAppend(cxxtri->namedElmts);
			cne = *cneHndl = MT(CNamedElmt);
			elmtName = Asn1ValueName2CValueName(namedElmt->definedName);
			len = strlen(elmtName);
			size_t size = len + 1 + r->maxDigitsToAppend;
			cne->name = Malloc(size);
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
			/*  not nec since each class hides the enum scope
			  DefineObj (&definedNamesG, cne->name); */
		}
	}

	/* fill in rest of type info depending on the type */
	switch (basicTypeId)
	{
		case BASICTYPE_BOOLEAN: /* library types */
		case BASICTYPE_INTEGER:
		case BASICTYPE_BITSTRING:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
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
			FillCxxTypeRefInfo(r, m, head, t, t->basicType->a.setOf);
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
				tmpCxxtdi = t->basicType->a.localTypeRef->link->cxxTypeDefInfo;
				cxxtri->className = tmpCxxtdi->className;
				cxxtri->isEnc = tmpCxxtdi->isEnc;
				cxxtri->optTestRoutineName = tmpCxxtdi->optTestRoutineName;
			}

			break;

		case BASICTYPE_ANYDEFINEDBY: /* ANY types */
			break;					 /* these are handled now */

		case BASICTYPE_ANY:
			/* Enhanced ANY processing always on */
			break;

		case BASICTYPE_CHOICE:
			/*
			 * must fill field names BEFORE filling choice elmts
			 * (allows better naming for choice ids)
			 */
			FillCxxFieldNames(r, t->basicType->a.choice);
			FillCxxChoiceElmts(r, m, head, t, t->basicType->a.choice);
			break;

		case BASICTYPE_SET:
		case BASICTYPE_SEQUENCE:
			FillCxxStructElmts(r, m, head, t, t->basicType->a.set);
			FillCxxFieldNames(r, t->basicType->a.set);
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
	cxxtri->isPtr = (unsigned char)IsCxxPtr(r, head, parent, t);

	/* let user overide any defaults with the --snacc attributes */
	/* undefined for C++ ParseTypeRefAttribs (ctri, t->attrList); */

} /* FillCxxTypeRefInfo */

static void FillCxxStructElmts PARAMS((r, m, head, parent, elmts), CxxRules* r _AND_ Module* m _AND_ TypeDef* head _AND_ Type* parent _AND_ NamedTypeList* elmts)
{
	NamedType* et;

	FOR_EACH_LIST_ELMT(et, elmts)
	{
		FillCxxTypeRefInfo(r, m, head, parent, et->type);
	}

} /* FillCxxStructElmts */

/*
 *  Figures out non-conflicting enum names for the
 *  choice id's
 */
static void FillCxxChoiceElmts PARAMS((r, m, head, parent, elmts), CxxRules* r _AND_ Module* m _AND_ TypeDef* head _AND_ Type* parent _AND_ NamedTypeList* elmts)
{
	NamedType* et;
	int idCount = 0;
	CxxTRI* cxxtri;
	size_t len;

	/*
	 * fill in type info for elmt types first
	 */
	FOR_EACH_LIST_ELMT(et, elmts)
	FillCxxTypeRefInfo(r, m, head, parent, et->type);

	/*
	 * set choiceId Symbol & value
	 * eg
	 *  Car ::= CHOICE {          class Car {
	 *     chev ChevCar,   ->         enum ChoiceIdEnum {
	 *     ford FordCar                  chevCid,
	 *     toyota ToyotaCar              fordCid,
	 *     }                             toyotaCid } choiceId;
	 *                                union CarChoiceUnion {
	 *                                      ChevCar *chev;
	 *                                      FordCar *ford;
	 *                                      ToyotaCar *toyota; };
	 *                                  ...
	 *                               }
	 * NOTE that the union is anonymous
	 */
	FOR_EACH_LIST_ELMT(et, elmts)
	{
		cxxtri = et->type->cxxTypeRefInfo;

		if (cxxtri == NULL)
			continue; /* wierd type */

		cxxtri->choiceIdValue = idCount++;

		len = strlen(cxxtri->fieldName);
		cxxtri->choiceIdSymbol = Malloc(len + 4);
		strcpy_s(cxxtri->choiceIdSymbol, len + 4, cxxtri->fieldName);
		strcat_s(cxxtri->choiceIdSymbol, len + 4, "Cid");

		if (r->capitalizeNamedElmts)
			Str2UCase(cxxtri->choiceIdSymbol, len);
	}

} /* FillCxxChoiceElmts */

/*
 * takes a list of "sibling" (eg same level in a structure)
 * ElmtTypes and fills sets up the c field names in
 * the CxxTRI struct
 */
static void FillCxxFieldNames PARAMS((r, elmts), CxxRules* r _AND_ NamedTypeList* elmts)
{
	NamedType* et;
	CxxTRI* cxxtri;
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
		cxxtri = et->type->cxxTypeRefInfo;
		if (cxxtri == NULL)
		{
			cxxtri = MT(CxxTRI);
			et->type->cxxTypeRefInfo = cxxtri;
		}
		if (et->fieldName != NULL)
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
			cxxtri->fieldName = Malloc(size);
			strcpy_s(cxxtri->fieldName, size, tmpName);
			Free(tmpName);

			/*   old    cxxtri->fieldName = Asn1FieldName2CFieldName (asn1FieldName); */

			MakeCxxStrUnique(fieldNames, cxxtri->fieldName, size, r->maxDigitsToAppend, 1);
			DefineObj(&fieldNames, cxxtri->fieldName);
		}
	}

	FOR_EACH_LIST_ELMT(et, elmts)
	{
		cxxtri = et->type->cxxTypeRefInfo;

		/*
		 * generate field names for those without them
		 */
		if (cxxtri->fieldName == NULL)
		{
			size_t size = 0;
			if ((et->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF) || (et->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF))
			{
				/*
				 * take ref'd type name as field name
				 * convert first let to lower case
				 */
				tmpName = et->type->basicType->a.localTypeRef->link->cxxTypeDefInfo->className;
				tmpName = Asn1TypeName2CTypeName(tmpName);
				size = strlen(tmpName) + r->maxDigitsToAppend + 1;
				cFieldName = Malloc(size);
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
				cFieldName = Malloc(size);
				strcpy_s(cFieldName, size, tmpName);

				if (isupper(cFieldName[0]))
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
			cxxtri->fieldName = cFieldName;
		}
	}
	FreeDefinedObjs(&fieldNames);
} /* FillCxxFieldNames */

/*
 * returns true if this c type for this type should be
 * be ref'd as a ptr
 */
static int IsCxxPtr PARAMS((r, td, parent, t), CxxRules* r _AND_ TypeDef* td _AND_ Type* parent _AND_ Type* t)
{
	CxxTDI* cxxtdi;
	int retVal = FALSE;

	/*
	 * inherit ptr attriubutes from ref'd type if any
	 * otherwise grab lib c type def from the CxxRules
	 */
	if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF) || (t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF))
		cxxtdi = t->basicType->a.localTypeRef->link->cxxTypeDefInfo;
	else
		cxxtdi = &r->typeConvTbl[GetBuiltinType(t)];

	/* no parent means t is the root of a typedef */
	if ((parent == NULL) && (cxxtdi->isPtrForTypeDef))
		retVal = TRUE;

	else if ((parent != NULL) && ((parent->basicType->choiceId == BASICTYPE_SET) || (parent->basicType->choiceId == BASICTYPE_SEQUENCE)) && (cxxtdi->isPtrInSetAndSeq))
		retVal = TRUE;

	else if ((parent != NULL) && ((parent->basicType->choiceId == BASICTYPE_SETOF) || (parent->basicType->choiceId == BASICTYPE_SEQUENCEOF)) && (cxxtdi->isPtrInList))
		retVal = TRUE;

	else if ((parent != NULL) && (parent->basicType->choiceId == BASICTYPE_CHOICE) && (cxxtdi->isPtrInChoice))
		retVal = TRUE;

	else if (((t->optional) || (t->defaultVal != NULL)) && (cxxtdi->isPtrForOpt))
		retVal = TRUE;

	return retVal;
} /* IsCxxPtr */

/* fill given cxxtdi with defaults from table for given typedef */
void FillCxxTDIDefaults PARAMS((r, cxxtdi, td), CxxRules* r _AND_ CxxTDI* cxxtdi _AND_ TypeDef* td)
{
	CxxTDI* tblCxxtdi;
	int typeIndex;
	char* tmpName;

	typeIndex = GetBuiltinType(td->type);

	if (typeIndex < 0)
		return;

	tblCxxtdi = &r->typeConvTbl[typeIndex];

	memcpy(cxxtdi, tblCxxtdi, sizeof(CxxTDI));

	/* make sure class name is unique wrt to previously defined classes */
	tmpName = Asn1TypeName2CTypeName(td->definedName);
	size_t size = strlen(tmpName) + r->maxDigitsToAppend + 1;
	cxxtdi->className = Malloc(size);
	strcpy_s(cxxtdi->className, size, tmpName);
	Free(tmpName);

	MakeCxxStrUnique(definedNamesG, cxxtdi->className, size, r->maxDigitsToAppend, 1);
	DefineObj(&definedNamesG, cxxtdi->className);

} /* FillCxxTDIDefaults */
