/*
 * compiler/core/err_chk.c - Check for semantic errors an ASN.1 module
 *
 * The following are checked:
 *
 * - Components of CHOICE and SET types must have distinct tags. x
 *
 * - CHOICE, ANY, and ANY DEFINED BY types cannot be implicitly tagged. x
 *
 * - Type and value names within the same scope must be unique. x
 *
 * - Field names in a SET, SEQUENCE or CHOICE must be distinct.  If
 *   a CHOICE with no field name is embedded in a SET, SEQUENCE or CHOICE,
 *   then the embedded CHOICE's field names must be distinct from its
 *   parents to avoid ambiguity in value notation. x
 *
 * - An APPLICATION tag can only be used once per module. x (done in asn1.yacc)
 *
 * - Each value in a named bit (BIT STRINGs) or named number x
 *   (INTEGERs and ENUMERATED) list must be different.
 *
 * - Each identifier in a named bit or named number list must be different. x
 *
 * - The tags on a series of one or more consecutive OPTIONAL or DEFAULT
 *   SEQUENCE elements and the following element must be distinct. x
 *
 * link_types.c does the following three checks
 * A COMPONENTS OF type in a SET must reference a SET
 * A COMPONENTS OF type in a SEQUENCE must reference a SEQUENCE
 * SELECTION types must reference a field of a CHOICE type.
 *
 * - gives a warning if an ANY DEFINED BY type appears in a SET or
 * if and ANY DEFINED BY appears in a SEQUENCE before its identifier.
 * these cases make decoding difficult.
 *
 *  ******* following are not done yet - need improved value proc. first*****
 *
 * - Each identifier in a BIT STRING value must from that BIT
 *   STRING's named bit list.
 *
 * - SET or SEQUENCE values can be empty {} only if the SET or
 *   SEQUENCE type was defined as empty or all of its elements are marked
 *   as OPTIONAL or DEFAULT.
 *
 * Mike Sample
 * 92/07/13
 *
 * Copyright (C) 1991, 1992 Michael Sample
 *            and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/core/err-chk.c,v 1.1.1.1 2005/04/14 14:59:43 \ste Exp $
 * $Log: err-chk.c,v $
 * Revision 1.1.1.1  2005/04/14 14:59:43  \ste
 * no message
 *
 * Revision 1.11  2004/04/06 15:13:41  gronej
 * *** empty log message ***
 *
 * Revision 1.10  2004/03/31 20:03:19  leonberp
 * resolved many gcc compile warnings
 *
 * Revision 1.9  2004/01/14 19:07:52  gronej
 * Updated Compiler to accept and process relative-oid's
 *
 * Revision 1.8  2003/07/28 11:11:23  colestor
 * Changes to complete handing of the "--snacc namespace" compiler directive.
 * Also, updates to handle ASN.1 constant integer tag designations for C++/C.
 *
 *
 * Revision 1.7  2003/07/07 14:50:13  nicholar
 * Eliminated headers and cleaned up include references
 *
 * Revision 1.6  2003/04/29 21:09:12  leonberp
 * integerated Deepak's changes for IOB support
 *
 * Revision 1.5  2002/09/16 16:49:22  mcphersc
 * Fixed warnings
 *
 * Revision 1.4  2002/09/04 18:58:39  leonberp
 * Enhanced ANY DEFINED BY check to handle int, oid, enum, or choice of them.
 *
 * Revision 1.3  2002/09/04 18:23:07  vracarl
 * got rid of c++ comments
 *
 * Revision 1.2  2000/10/24 14:54:51  rwc
 * Updated to remove high-level warnings (level 4 on MSVC++) for an easier build.
 * SOME warnings persist due to difficulty in modifying the SNACC compiler to
 * properly build clean source; also some files are built by Lex/Yacc.
 *
 * Revision 1.1.1.1  2000/08/21 20:35:59  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.3.1.1  1997/08/20 23:14:40  povey
 *
 *
 * Revision 1.3  1995/07/25 19:41:25  rj
 * changed `_' to `-' in file names.
 *
 * Revision 1.2  1994/09/01  00:33:02  rj
 * snacc_config.h removed; err_chk.h includet.
 *
 * Revision 1.1  1994/08/28  09:49:05  rj
 * first check-in. for a list of changes to the snacc-1.1 distribution please refer to the ChangeLog.
 *
 */

#include <string.h>
#include <ctype.h>
#include "../../c-lib/include/asn-incl.h"
#include "asn1module.h"
#include "snacc-util.h"
#include "define.h"

typedef struct DefinedTag
{
	Tag* tag;
	struct DefinedTag* next;
} DefinedTag;

typedef struct DefinedName
{
	char* name;
	struct DefinedName* next;
} DefinedName;

static NamedType* badNamedType;
// static DefinedName *fieldNames = NULL;

int CountTags(Type* t);
enum BasicTypeChoiceId ParanoidGetBuiltinType(Type* t);

void ErrChkTypeDef(Module* m, TypeDef* td);
void ErrChkType(Module* m, TypeDef* td, Type* parent, NamedType* nt, Type* t);
void ErrChkElmtTypes(Module* m, TypeDef* td, Type* parent, NamedTypeList* e);
void ErrChkBasicType(Module* m, TypeDef* td, Type* parent, NamedType* nt, Type* type);
void ErrChkValueDef(Module* m, ValueDef* vd);
void ErrChkValue(Module* m, ValueDef* vd, Value* v);
int HasDistinctTags(NamedTypeList* elmts);
int AddFirstTag(DefinedObj** definedTags, Type* t);
void ChkFieldNames(Module* m, TypeDef* td, Type* parent, NamedTypeList* elmts);
void ChkNamedNumbers(Module* m, Type* t, NamedNumberList* n);
void ChkNamedBits(Module* m, Type* t, NamedNumberList* n);
void ChkSeqTags(Module* m, TypeDef* td, Type* t);
char* DetermineCode(Tag* tag, int* ptagLen, int bJustIntegerFlag);

extern FILE* errFileG; /* Pointer to file for reporting errors */

/* return TRUE if the Tag *t1 and t2 are the same in class and code */
int TagObjCmp(void* vt1, void* vt2)
{
	Tag* t1 = vt1;
	Tag* t2 = vt2;
	int iResult = 0;
	if (t1->valueRef == NULL && t2->valueRef == NULL)
	{
		iResult = (t1->tclass == t2->tclass && t1->code == t2->code);
	}
	else
	{ // RWC;THEN we need to check further, may be indirectly referenced.
	  // THIS logic assumes similar types, only Integer value returned...
		char* p1 = DetermineCode(t1, NULL, 1);
		if (p1)
		{
			char* p1Tmp = _strdup(p1);
			// "static" memory, so 1 needs to be copied.
			char* p2 = DetermineCode(t2, NULL, 1);
			if (p2)
				if (strcmp(p1Tmp, p2) == 0)
					iResult = 1;
			free(p1Tmp);
		} // END IF p1
	}
	return (iResult);
}

/*
 * Checks for errors listed above.
 * sets module status to MOD_ERROR if any errors occured
 */
void ErrChkModule PARAMS((m), Module* m)
{
	TypeDef* td;
	ValueDef* vd = NULL;
	DefinedObj* typeNames;
	DefinedObj* valueNames;
	ImportModule* impList;
	ImportElmt* impElmt;

	/*
	 * go through each type in typeList
	 */
	typeNames = NewObjList();
	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		/* first check for name conflicts */
		if (ObjIsDefined(typeNames, td->definedName, StrObjCmp))
		{
			PrintErrLoc(m->asn1SrcFileName, (long)td->type->lineNo);
			fprintf(errFileG, "ERROR - type \"%s\" is multiply defined.\n", td->definedName);
			m->status = MOD_ERROR;
		}
		else
			DefineObj(&typeNames, td->definedName);

		/* now check type def internals */
		ErrChkTypeDef(m, td);
	}

	/*  now check for name conflicts with imported types */
	FOR_EACH_LIST_ELMT(impList, m->imports)
	{
		FOR_EACH_LIST_ELMT(impElmt, impList->importElmts)
		{
			if ((!impElmt->privateScope) && (isupper(impElmt->name[0])))
			{
				if (ObjIsDefined(typeNames, impElmt->name, StrObjCmp))
				{
					PrintErrLoc(m->asn1SrcFileName, (long)impElmt->lineNo);
					fprintf(errFileG, "ERROR - type \"%s\" is multiply defined.\n", impElmt->name);
					m->status = MOD_ERROR;
				}
				else
					DefineObj(&typeNames, impElmt->name);
			}
		}
	}
	FreeDefinedObjs(&typeNames);

	/*
	 *  go through each value for types
	 */
	valueNames = NewObjList();
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		/* check for name conflict */
		if (ObjIsDefined(valueNames, vd->definedName, StrObjCmp))
		{
			PrintErrLoc(m->asn1SrcFileName, (long)vd->value->lineNo);
			fprintf(errFileG, "ERROR - value \"%s\" is multiply defined.\n", vd->definedName);
			m->status = MOD_ERROR;
		}
		else
			DefineObj(&valueNames, vd->definedName);

		/* check value internal info */
		ErrChkValueDef(m, vd);
	}
	/*  now check for name conflicts with imported values */
	FOR_EACH_LIST_ELMT(impList, m->imports)
	{
		FOR_EACH_LIST_ELMT(impElmt, impList->importElmts)
		{
			if ((!impElmt->privateScope) && (islower(impElmt->name[0])))
			{
				if (ObjIsDefined(valueNames, impElmt->name, StrObjCmp))
				{
					PrintErrLoc(m->asn1SrcFileName, (long)impElmt->lineNo);
					fprintf(errFileG, "ERROR - value \"%s\" is multiply defined.\n", impElmt->name);
					m->status = MOD_ERROR;
				}
				else
					DefineObj(&valueNames, impElmt->name);
			}
		}
	}

	FreeDefinedObjs(&valueNames);

} /* ErrChkModule */

void ErrChkTypeDef PARAMS((m, td), Module* m _AND_ TypeDef* td)
{
	if (td == NULL)
		return;

	ErrChkType(m, td, NULL, NULL, td->type);

} /* ErrChkTypeDef */

void ErrChkType PARAMS((m, td, parent, nt, t), Module* m _AND_ TypeDef* td _AND_ Type* parent _AND_ NamedType* nt _AND_ Type* t)
{
	if (t == NULL)
		return;

	ErrChkBasicType(m, td, parent, nt, t);

} /* ErrChkType */

void ErrChkElmtTypes PARAMS((m, td, parent, e), Module* m _AND_ TypeDef* td _AND_ Type* parent _AND_ NamedTypeList* e)
{
	NamedType* nt;

	/*
	 * if starting new type aggregate type,
	 * check that the field names are distinct
	 * (goes 'through' un-named elements that are CHOICEs)
	 */
	if (td->type == parent)
		ChkFieldNames(m, td, parent, e);

	FOR_EACH_LIST_ELMT(nt, e)
	{
		ErrChkType(m, td, parent, nt, nt->type);
	}
} /* ErrChkElmtTypes */

void ErrChkBasicType PARAMS((m, td, parent, tnt, type), Module* m _AND_ TypeDef* td _AND_ Type* parent _AND_ NamedType* tnt _AND_ Type* type)
{
	NamedType* nt = NULL;
	Type* refdType;
	enum BasicTypeChoiceId refdTypeId;

	if ((type == NULL) || (type->basicType == NULL))
		return;

	switch (type->basicType->choiceId)
	{
		case BASICTYPE_LOCALTYPEREF:
		case BASICTYPE_IMPORTTYPEREF:
			/*
			 * make sure that untagged CHOICE and ANY types
			 * are not implicitly tagged
			 */
			refdTypeId = ParanoidGetBuiltinType(type);
			if ((type->implicit) && ((refdTypeId == BASICTYPE_CHOICE) || (refdTypeId == BASICTYPE_ANY) || (refdTypeId == BASICTYPE_ANYDEFINEDBY)) && (CountTags(type->basicType->a.localTypeRef->link->type) == 0))
			{
				m->status = MOD_ERROR;
				PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
				fprintf(errFileG, "ERROR - IMPLICITLY tagged CHOICE, ANY or ANY DEFINED BY type.\n");
			}

			if ((parent != NULL) && ((refdTypeId == BASICTYPE_ANY) || (refdTypeId == BASICTYPE_ANYDEFINEDBY)))
			{

				/*
				 * give a warning.  It is stupid to have an ANY DEFINED
				 * BY type in a SET since they are not ordered and hence
				 * the ANY DEFINED BY type may need to be decoded before
				 * its identifer which is very difficult
				 */
				if ((refdTypeId == BASICTYPE_ANYDEFINEDBY) && (parent->basicType->choiceId == BASICTYPE_SET))
				{
					PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
					fprintf(errFileG, "WARNING - ANY DEFINED BY in a SET needs to be decoded before its identifier. This is not guaranteed since SETs are not ordered.  Use a SEQUENCE instead, if possible.\n");
				}

				/*
				 * give a warning.  It is stupid to have an ANY DEFINED
				 * BY type in a SEQUENCE before its identifier.
				 * The ANY DEFINED BY type will need to be decoded before
				 * its identifer which is very difficult.
				 * tnt is the NamedType holding "type"
				 */
				if ((refdTypeId == BASICTYPE_ANYDEFINEDBY) && (tnt != NULL) && (parent->basicType->choiceId == BASICTYPE_SEQUENCE) && (GetAsnListElmtIndex(tnt, parent->basicType->a.sequence) < GetAsnListElmtIndex(type->basicType->a.anyDefinedBy->link, parent->basicType->a.sequence)))
				{
					PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
					fprintf(errFileG, "WARNING - ANY DEFINED BY in SEQUENCE should appear before its identifier since the identifier must be decoded before the ANY DEFINED BY type.\n");
				}

				if (parent->basicType->choiceId == BASICTYPE_SEQUENCE)
					nt = LAST_LIST_ELMT(parent->basicType->a.sequence);

				/*
				 * untagged, optional ANYs are strange and will cause faulty
				 * decoding code to be generated unless they are the last
				 * elmt in a SEQUENCE.
				 * (if they are the last elmt it is easy to check
				 *  for the presence of the ANY if definite lengths are used)
				 * (must peek ahead for EOC otherwise)
				 */
				if (nt && !((parent->basicType->choiceId == BASICTYPE_SEQUENCE) && (type == nt->type)) && (type->optional) && (CountTags(type) == 0))
				{
					PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
					fprintf(errFileG, "WARNING - untagged optional ANY encountered, the produced code will be wrong.\n");
				}
			}

			break;

		case BASICTYPE_INTEGER:
		case BASICTYPE_ENUMERATED:
			ChkNamedNumbers(m, type, type->basicType->a.integer);
			break;

		case BASICTYPE_BITSTRING:
			ChkNamedBits(m, type, type->basicType->a.bitString);
			break;

		case BASICTYPE_SEQUENCEOF:
		case BASICTYPE_SETOF:
			ErrChkType(m, td, type, NULL, type->basicType->a.setOf);
			break;

		case BASICTYPE_SEQUENCE:
			ErrChkElmtTypes(m, td, type, type->basicType->a.sequence);

			/*
			 * check that tags on one or more consecutive optional elmts
			 * and following (if any) non-optional elmt are distinct
			 */
			ChkSeqTags(m, td, type);
			break;

		case BASICTYPE_OBJECTCLASS: // Deepak: 14/Mar/2003
			ErrChkElmtTypes(m, td, type, type->basicType->a.objectclass->classdef);

			/*
			 * check that tags on one or more consecutive optional elmts
			 * and following (if any) non-optional elmt are distinct
			 */
			ChkSeqTags(m, td, type); // Deepak: ????? chk for Class Tags???
			break;

		case BASICTYPE_CHOICE:
			/* CHOICE elements must have distinct tags */
			if (!HasDistinctTags(type->basicType->a.choice))
			{
				PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
				fprintf(errFileG, "ERROR - tag conflict among the CHOICE elements.\n");
				m->status = MOD_ERROR;
			}

			/*
			 * untagged choices cannot be implicitily tagged
			 * (this would make it impossible/difficult to figure out which
			 * elmt of the choice was present when decoding)
			 */
			if (((type->tags == NULL) || LIST_EMPTY(type->tags)) && (type->implicit))
			{
				PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
				fprintf(errFileG, "ERROR - IMPLICITLy tagged CHOICE type.\n");
				m->status = MOD_ERROR;
			}

			/* Check out each of the components */
			ErrChkElmtTypes(m, td, type, type->basicType->a.choice);

			break;

		case BASICTYPE_ANYDEFINEDBY:
			/* for ANY DEFINED BY make sure id field is int or oid */
			refdType = ResolveImportedType(type->basicType->a.anyDefinedBy->link->type);
			if ((refdType->basicType->choiceId != BASICTYPE_INTEGER) && (refdType->basicType->choiceId != BASICTYPE_ENUMERATED) && (refdType->basicType->choiceId != BASICTYPE_OID) && (refdType->basicType->choiceId != BASICTYPE_RELATIVE_OID))
			{
				if (refdType->basicType->choiceId == BASICTYPE_CHOICE)
				{
					NamedType* nt2;

					FOR_EACH_LIST_ELMT(nt2, refdType->basicType->a.choice)
					{
						enum BasicTypeChoiceId choiceId = nt2->type->basicType->choiceId;

						if (choiceId != BASICTYPE_INTEGER && choiceId != BASICTYPE_ENUMERATED && choiceId != BASICTYPE_OID && choiceId != BASICTYPE_RELATIVE_OID)
						{
							PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
							fprintf(errFileG, "ERROR - Field referenced by ANY DEFINED BY type must be of INTEGER or OBJECT IDENTIFIER type.\n");
							m->status = MOD_ERROR;
						}
					}
				}
				else
				{

					PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
					fprintf(errFileG, "ERROR - Field referenced by ANY DEFINED BY type must be of INTEGER or OBJECT IDENTIFIER type.\n");
					m->status = MOD_ERROR;
				}
			}
			/* make sure id field is not optional */
			if (type->basicType->a.anyDefinedBy->link->type->optional)
			{
				PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
				fprintf(errFileG, "ERROR - Field referenced by ANY DEFINED BY cannot be optional.\n");
				m->status = MOD_ERROR;
			}

			/*
			 * give a warning.  It is stupid to have an ANY DEFINED
			 * BY type in a SET since they are not ordered and hence
			 * the ANY DEFINED BY type may need to be decoded before
			 * its identifer which is very difficult
			 */
			if ((parent != NULL) && (parent->basicType->choiceId == BASICTYPE_SET))
			{
				PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
				fprintf(errFileG, "WARNING - ANY DEFINED BY in a SET needs to be decoded before its identifier. This is not guaranteed since SETs are not ordered.  Use a SEQUENCE instead, if possible.\n");
			}

			/*
			 * give a warning.  It is stupid to have an ANY DEFINED
			 * BY type in a SEQUENCE before its identifier.
			 * The ANY DEFINED BY type will need to be decoded before
			 * its identifer which is very difficult.
			 * tnt is the NamedType holding "type"
			 */
			if ((parent != NULL) && (tnt != NULL) && (parent->basicType->choiceId == BASICTYPE_SEQUENCE) && (GetAsnListElmtIndex(tnt, parent->basicType->a.sequence) < GetAsnListElmtIndex(type->basicType->a.anyDefinedBy->link, parent->basicType->a.sequence)))
			{
				PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
				fprintf(errFileG, "WARNING - ANY DEFINED BY in SEQUENCE should appear before its identifier since the identifier must be decoded before the ANY DEFINED BY type.\n");
			}

			/* fall through - arrrrrg! */

		case BASICTYPE_ANY:
			/* ANY cannot be implicitily tagged */
			if (((type->tags == NULL) || LIST_EMPTY(type->tags)) && (type->implicit))
			{
				PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
				fprintf(errFileG, "ERROR - IMPLICITLy tagged ANY type.\n");
				m->status = MOD_ERROR;
			}

			if (parent != NULL)
			{
				if (parent->basicType->choiceId == BASICTYPE_SEQUENCE)
					nt = LAST_LIST_ELMT(parent->basicType->a.sequence);

				/*
				 * untagged, optional ANYs are strange and will cause faulty
				 * decoding code to be generated unless they are the last
				 * elmt in a SEQUENCE
				 */
				if (!((parent->basicType->choiceId == BASICTYPE_SEQUENCE) && (type == nt->type)) && (type->optional) && (CountTags(type) == 0))
				{
					PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
					fprintf(errFileG, "WARNING - untagged optional ANY encountered, the produced code will be wrong.\n");
				}

				/*
				 *  if parent is SET or CHOICE then ANY or ANY DEFINED BY
				 *  should be tagged to help determine its presence
				 *
				 * NOTE: there are also probs with untagged ANYs in SEQs
				 * where the ANY is preceeded by optional elmts
				 * (err msg written in produced code)
				 */
				if (((parent->basicType->choiceId == BASICTYPE_SET) || (parent->basicType->choiceId == BASICTYPE_CHOICE)) && (CountTags(type) == 0))
				{
					PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
					fprintf(errFileG, "WARNING - untagged ANY in a SET or CHOICE, the produced code will be wrong.\n");
				}
			}

			break;

		case BASICTYPE_SET:
			/* SET elements must have distinct tags */
			if (!HasDistinctTags(type->basicType->a.set))
			{
				PrintErrLoc(m->asn1SrcFileName, (long)type->lineNo);
				fprintf(errFileG, "ERROR - tag conflict among the SET elements.\n");
				m->status = MOD_ERROR;
			}

			/* Check out each of the components */
			ErrChkElmtTypes(m, td, type, type->basicType->a.set);
			break;

		default:
			/* the rest do not need checking */
			break;
	}
} /* ErrChkBasicType */

void ErrChkValueDef PARAMS((m, vd), Module* m _AND_ ValueDef* vd)
{
	ErrChkValue(m, vd, vd->value);
}

void ErrChkValue PARAMS((m, vd, v), Module* m _AND_ ValueDef* vd _AND_ Value* v)
{
}

/*
 * returns non-zero if the first tags on the elements
 * are all different.  Otherwise 0 is returned
 *
 *  algorithm: add each tag to a list, adding only if
 *             not already in list. if there, free list
 *             and return FALSE. if finished adding tags
 *             and no duplicates occurred then return TRUE;
 */
int HasDistinctTags PARAMS((elmts), NamedTypeList* elmts)
{
	DefinedObj* tL;
	NamedType* e;

	tL = NewObjList();
	FOR_EACH_LIST_ELMT(e, elmts)
	{
		if (!AddFirstTag(&tL, e->type))
		{
			FreeDefinedObjs(&tL);
			badNamedType = e;
			return FALSE;
		}
	}
	FreeDefinedObjs(&tL);
	badNamedType = NULL;
	return TRUE;
} /* HasDistinctTags */

/*
 * puts first tag of the given type into the defined tags list
 * returns FALSE if the tag was already in the defined tags list.
 * return TRUE otherwise
 */
int AddFirstTag PARAMS((definedTags, t), DefinedObj** definedTags _AND_ Type* t)
{
	Tag* tag;
	TagList* tl;
	int implicitRef;
	NamedType* e;

	tl = t->tags;
	if (tl != NULL)
		AsnListFirst(tl);

	implicitRef = FALSE;

	for (;;)
	{
		/*
		 * get first tag from tag list local to this type if any
		 */

		if ((tl != NULL) && (CURR_LIST_NODE(tl) != NULL) && (CURR_LIST_ELMT(tl) != NULL))
		{
			tag = (Tag*)CURR_LIST_ELMT(tl);

			if (ObjIsDefined(*definedTags, tag, TagObjCmp))
				return FALSE;
			else
			{
				DefineObj(definedTags, tag);
				return TRUE;
			}
		}

		/*
		 * follow tags of referenced types if no tags on this type
		 */

		if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF) || (t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF))
		{
			if (!implicitRef)
				implicitRef = t->implicit;

			if (t->basicType->a.localTypeRef->link == NULL)
			{
				/* this should be found in the type link stage */
				fprintf(errFileG, "ERROR - unresolved type ref, cannot get tags for decoding\n");
				break;
			}
			t = t->basicType->a.localTypeRef->link->type;
			tl = t->tags;

			if (tl != NULL)
			{
				AsnListFirst(tl); /* set curr ptr to first node */
				if ((!LIST_EMPTY(tl)) && implicitRef)
				{
					AsnListNext(tl);
					implicitRef = FALSE;
				}
			}
		}

		/*
		 * if untagged choice and no tags found yet
		 */
		else if (t->basicType->choiceId == BASICTYPE_CHOICE)
		{
			/*
			 * add top level tags from each choice elmt
			 */
			if (implicitRef)
				fprintf(errFileG, "ERROR - IMPLICITLY Tagged CHOICE\n");

			FOR_EACH_LIST_ELMT(e, t->basicType->a.choice)
			{
				if (!AddFirstTag(definedTags, e->type))
					return FALSE;
			}

			return TRUE;
		}

		else /* could be ANY type - assume correct tagging */
			return TRUE;
	}
	return TRUE;
} /* AddFirstTag */

/*
 *  Prints Errors if the field names of the elements are
 *  not distinct.
 *  currently an endless recursion problem here
 *  for recursive types involving CHOICEs - Fixed MS
 */
void ChkFieldNamesRec PARAMS((m, td, parent, elmts, fieldNames, followedTypeRefs), Module* m _AND_ TypeDef* td _AND_ Type* parent _AND_ NamedTypeList* elmts _AND_ DefinedObj** fieldNames _AND_ DefinedObj** followedTypeRefs)
{
	NamedType* e;
	const Type* definingType;

	FOR_EACH_LIST_ELMT(e, elmts)
	{
		definingType = ParanoidGetType(e->type);
		if (e->fieldName != NULL)
		{
			if (ObjIsDefined(*fieldNames, e->fieldName, StrObjCmp))
			{
				if (parent->basicType->a.choice == elmts)
				{
					PrintErrLoc(m->asn1SrcFileName, (long)e->type->lineNo);
					fprintf(errFileG, "WARNING - field name \"%s\" is used more than once in same value notation scope.\n", e->fieldName);
				}
				else
				{
					PrintErrLoc(m->asn1SrcFileName, (long)parent->lineNo);
					fprintf(errFileG, "WARNING - field name \"%s\" in embedded CHOICE conflicts with field name in type \"%s\".", e->fieldName, td->definedName);
					fprintf(errFileG, " This may lead to ambiguous value notation.\n");
				}
				/* m->status = MOD_ERROR; */
			}
			else
				DefineObj(fieldNames, e->fieldName);
		}

		/*
		 * must include embedded CHOICE's field names
		 * if it has no field name (this case is a reference to
		 * a CHOICE) (fieldName is NULL)
		 */
		else if (((e->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF) || (e->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)) && (definingType->basicType->choiceId == BASICTYPE_CHOICE))
		{
			/* stop if this is a recursive ref we have already checked */
			if (!ObjIsDefined(*followedTypeRefs, e->type->basicType->a.localTypeRef->typeName, StrObjCmp))
			{
				/* push this type name so we don't go through it again */
				DefineObj(followedTypeRefs, e->type->basicType->a.localTypeRef->typeName);
				/* pass in field type not defining type as parent for line no*/
				ChkFieldNamesRec(m, td, e->type, definingType->basicType->a.choice, fieldNames, followedTypeRefs);

				/* pop this type name since we're done checking it */
				UndefineObj(followedTypeRefs, e->type->basicType->a.localTypeRef->typeName, StrObjCmp);
			}
		}

		/* this is an embedded CHOICE definition (fieldName is NULL) */
		else if (e->type->basicType->choiceId == BASICTYPE_CHOICE)
		{
			ChkFieldNamesRec(m, td, e->type, /* pass in field type for line */
							 definingType->basicType->a.choice, fieldNames, followedTypeRefs);
		}
	}
} /* ChkFieldNamesRec */

/*
 * wrapper for ChkFieldNamesRec
 * Checks that the field names of an aggregate type (CHOICE/SET/SEQ)
 * are distinct.  Violations are printed to errFileG.
 */
void ChkFieldNames PARAMS((m, td, parent, elmts), Module* m _AND_ TypeDef* td _AND_ Type* parent _AND_ NamedTypeList* elmts)
{
	DefinedObj* fieldNames;
	DefinedObj* followedTypeRefs;

	fieldNames = NewObjList();
	followedTypeRefs = NewObjList();

	/*
	 * first define the type itself as followed to prevent
	 * infinintely checking it
	 */
	DefineObj(&followedTypeRefs, td->definedName);

	ChkFieldNamesRec(m, td, parent, elmts, &fieldNames, &followedTypeRefs);

	FreeDefinedObjs(&fieldNames);
	FreeDefinedObjs(&followedTypeRefs);

} /* ChkFieldNames */

/*
 * make sure that the identifiers of the named numbers are unique
 * among themselves.
 *
 * also check that the values of the named numbers are unique
 * among themselves.
 */
void ChkNamedNumbers PARAMS((m, t, n), Module* m _AND_ Type* t _AND_ NamedNumberList* n)
{
	DefinedObj* ids;
	DefinedObj* nums;
	ValueDef* nn;
	Value* baseVal;

	if (n == NULL)
		return;

	ids = NewObjList();
	nums = NewObjList();
	FOR_EACH_LIST_ELMT(nn, n)
	{
		if (ObjIsDefined(ids, nn->definedName, StrObjCmp))
		{
			PrintErrLoc(m->asn1SrcFileName, (long)t->lineNo);
			fprintf(errFileG, "ERROR - named numbers (%s) must have unique identifiers.\n", nn->definedName);
		}
		else
			DefineObj(&ids, nn->definedName);

		baseVal = GetValue(nn->value);
		if (baseVal->basicValue->choiceId != BASICVALUE_INTEGER)
		{
			PrintErrLoc(m->asn1SrcFileName, (long)t->lineNo);
			fprintf(errFileG, "ERROR - value format problem (%s)- named numbers must be integers.\n", nn->definedName);
		}
		else if (ObjIsDefined(nums, &baseVal->basicValue->a.integer, IntObjCmp))
		{
			PrintErrLoc(m->asn1SrcFileName, (long)t->lineNo);
			fprintf(errFileG, "ERROR - named numbers (%s) must have unique values.\n", nn->definedName);
		}
		else
			DefineObj(&nums, &baseVal->basicValue->a.integer);
	}

	FreeDefinedObjs(&ids);
	FreeDefinedObjs(&nums);

} /* ChkNamedNumbers */

/*
 * The same as ChkNamedNumbers except that the elmt values must be
 * > 0 (needed for BIT STRINGs)
 */
void ChkNamedBits PARAMS((m, t, n), Module* m _AND_ Type* t _AND_ NamedNumberList* n)
{
	ValueDef* vd;
	Value* baseVal;

	ChkNamedNumbers(m, t, n);

	FOR_EACH_LIST_ELMT(vd, n)
	{
		baseVal = GetValue(vd->value);
		if ((baseVal->basicValue->choiceId == BASICVALUE_INTEGER) && (baseVal->basicValue->a.integer < 0))
		{
			PrintErrLoc(m->asn1SrcFileName, (long)t->lineNo);
			fprintf(errFileG, "ERROR - named bits (%s) must have positive values.\n", vd->definedName);
		}
	}

} /* ChkNamedBits */

/*
 * check that tags on one or more consecutive optional elmts
 * and following (if any) non-optional elmt are distinct
 */
void ChkSeqTags PARAMS((m, td, t), Module* m _AND_ TypeDef* td _AND_ Type* t)
{
	DefinedObj* dO;
	NamedType* e;

	if (t->basicType->choiceId != BASICTYPE_SEQUENCE)
		return;

	dO = NewObjList();
	FOR_EACH_LIST_ELMT(e, t->basicType->a.sequence)
	{
		/* if optional add tag */
		if (e->type->optional || (e->type->defaultVal != NULL))
		{
			if (!AddFirstTag(&dO, e->type))
			{
				PrintErrLoc(m->asn1SrcFileName, (long)e->type->lineNo);
				fprintf(errFileG, "ERROR - one or more consecutive optional SEQUENCE elmements and the the following non-optional elmt (if any) must have distinct tags.\n");
				m->status = MOD_ERROR;
			}
		}
		else if (dO != NULL) /* first non-opt after opt elmts */
		{
			if (!AddFirstTag(&dO, e->type))
			{
				PrintErrLoc(m->asn1SrcFileName, (long)e->type->lineNo);
				fprintf(errFileG, "ERROR - one or more consecutive optional SEQUENCE elmements and the the following non-optional elmt (if any) must have distinct tags.\n");
				m->status = MOD_ERROR;
			}
			FreeDefinedObjs(&dO);
			dO = NewObjList();
		}
	}
	FreeDefinedObjs(&dO);
} /* ChkSeqTags */
