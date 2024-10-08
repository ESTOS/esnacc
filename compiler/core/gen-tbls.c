/*
 * compiler/core/gen_tbls.c
 *
 * generates type tables and writes them to a file.
 *
 * MS
 * 93/02/07
 *
 * Copyright (C) 1993 Michael Sample
 *            and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/core/gen-tbls.c,v 1.1.1.1 2005/04/14 14:59:43 \ste Exp $
 * $Log: gen-tbls.c,v $
 * Revision 1.1.1.1  2005/04/14 14:59:43  \ste
 * no message
 *
 * Revision 1.8  2004/01/14 19:07:52  gronej
 * Updated Compiler to accept and process relative-oid's
 *
 * Revision 1.7  2003/07/14 21:07:43  nicholar
 * Changed how parser handles --snacc directives.  Added namespace option.
 *
 * Revision 1.6  2003/07/07 14:50:13  nicholar
 * Eliminated headers and cleaned up include references
 *
 * Revision 1.5  2002/12/05 15:16:26  nicholar
 * Fixed GenTypeTbls() to use GenBuf.
 *
 * Revision 1.4  2002/09/16 16:49:46  mcphersc
 * iFixed warnings
 *
 * Revision 1.3  2002/09/04 18:23:07  vracarl
 * got rid of c++ comments
 *
 * Revision 1.2  2000/10/24 14:54:51  rwc
 * Updated to remove high-level warnings (level 4 on MSVC++) for an easier build.
 * SOME warnings persist due to difficulty in modifying the SNACC compiler to
 * properly build clean source; also some files are built by Lex/Yacc.
 *
 * Revision 1.1.1.1  2000/08/21 20:36:00  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.5  1997/06/19 09:17:16  wan
 * Added isPdu flag to tables. Added value range checks during parsing.
 *
 * Revision 1.4  1997/05/07 15:18:34  wan
 * Added (limited) size constraints, bitstring and enumeration names to tables
 *
 * Revision 1.3  1995/07/25 19:41:28  rj
 * changed `_' to `-' in file names.
 *
 * Revision 1.2  1994/09/01  00:33:41  rj
 * snacc_config.h removed.
 *
 * Revision 1.1  1994/08/28  09:49:10  rj
 * first check-in. for a list of changes to the snacc-1.1 distribution please refer to the ChangeLog.
 *
 */

#include <string.h>
#include "../../c-lib/include/asn-incl.h"
#include "asn1module.h"
#include "tbl.h"

int FlattenLinkedOid PARAMS((o, asn1FileName, lineNo, quiet), OID* o _AND_ const char* asn1FileName _AND_ AsnInt lineNo _AND_ int quiet);

extern FILE* errFileG; // Defined in snacc.c

const char* GetDirectiveName(SnaccDirectiveEnum dirType);

/* non-exported routine protos */
void GenTypeDefIds PROTO((TBL * tbl, Module* m));
int GenTblModule PROTO((TBL * tbl, Module* m, TBLModule** newTbl));
int GenTblTypeDefs PROTO((TBL * tbl, Module* m, TBLModule* tblMod));
int GenTblTypes PROTO((TBL * tbl, Module* m, TBLModule* tblMod, TypeDef* td, TBLTypeDef* tblTd));
TBLType* GenTblTypesRec PROTO((TBL * tbl, Module* m, TBLModule* tblMod, TypeDef* td, TBLTypeDef* tblTd, Type* t));

static int abortTblTypeDefG;
static int tblTypesTotalG;
static int tblTagsTotalG;
static int tblStringsTotalG;
static int tblStringLenTotalG;

static int tableFileVersionG;

void GenTypeTbls PARAMS((mods, fileName, tableFileVersion), ModuleList* mods _AND_ char* fileName _AND_ int tableFileVersion)
{
	TBL tbl;
	TBLModule* newTblMod;
	FILE* tblFile;
	GenBuf* buf;
	ExpBuf *expBuf, *tmpBuf;
	Module* m;

	tableFileVersionG = tableFileVersion;

	tbl.modules = AsnListNew(sizeof(void*));
	tbl.totalNumModules = 0;
	tbl.totalNumTypeDefs = 0;
	tbl.totalNumTypes = 0;
	tbl.totalNumTags = 0;
	tbl.totalNumStrings = 0;
	tbl.totalLenStrings = 0;

	/*
	 * Give each type def a unique id
	 * Id is stored in TypeDef's "tmpRefCount" since
	 * it was only used in the recursion pass.
	 * Also updates tbl.totalNumModules and
	 * tbl.totalNumTypeDefs appropriately
	 */
	FOR_EACH_LIST_ELMT(m, mods)
	{
		GenTypeDefIds(&tbl, m);
	}

	/* convert each module from parse format to simpler table format */
	FOR_EACH_LIST_ELMT(m, mods)
	{
		if (!GenTblModule(&tbl, m, &newTblMod))
		{
			fprintf(errFileG, "ERROR: type table generator failed for module \"%s\", so file \"%s\" will not be written.\n", m->modId->name, fileName);
			return;
		}
	}

	/* encode the TBLModules */
	ExpBufInit(1024);
	expBuf = ExpBufAllocBufAndData();
	ExpBuftoGenBuf(expBuf, &buf);
	BEncTBL(buf, &tbl);

	if (GenBufWriteError(buf))
	{
		fprintf(errFileG, "ERROR: buffer write error during encoding of type table.\n");
		ExpBufFreeBufAndDataList(expBuf);
		GenBufFree(buf);
		return;
	}

	/* open & truncate or create as file with given filename */

#ifdef _WIN32
	if (fopen_s(&tblFile, fileName, "w") != 0 || tblFile == NULL)
	{
		fprintf(errFileG, "ERROR: Could not open file \"%s\" for the type table.\n", fileName);
		ExpBufFreeBufAndDataList(expBuf);
		GenBufFree(buf);
		return;
	}
#else  // _WIN32
	tblFile = fopen(fileName, "w");
	if (tblFile == NULL)
	{
		fprintf(errFileG, "ERROR: Could not open file \"%s\" for the type table.\n", fileName);
		ExpBufFreeBufAndDataList(expBuf);
		GenBufFree(buf);
		return;
	}
#endif // _WIN32

	/*
	 * go through buffer (s) and write encoded value
	 * to opened file
	 */
	expBuf = ExpBufListFirstBuf(expBuf);
	for (tmpBuf = expBuf; tmpBuf != NULL; tmpBuf = tmpBuf->next)
		fwrite(tmpBuf->dataStart, tmpBuf->dataEnd - tmpBuf->dataStart, 1, tblFile);

	fclose(tblFile);

	ExpBufFreeBufAndDataList(expBuf);
	GenBufFree(buf);

} /* GenTypeTbls */

/*
 * The typeDefIds start at zero.  They are used as "portable"
 * pointers.  Each TBLTypeDef has a unique typeDefId.
 * The typeDefIds in a given TBLModule will be consecutive
 * and increasing from the first typedef to the last.
 *
 * This routine gives each type def in the given module a unique
 * integer identifier.
 * This id is temporarily stored in the tmpRefCount field of the TypeDef
 * (in the big parse tree).  The typeDefId is transfered
 * to the TBL data structure after this.
 *
 * tbl.totalNumModules and tbl.totalNumTypeDefs are updated.
 *
 * ASSUMES: that tbl->totalNumModules is initialized to zero
 *          and that tbl->totalNumTypeDefs is initialized to zero
 *          on the first call to this routine.
 *          This allows subsequent calls to give out the proper ids
 *          to the types in the next module.
 *
 *  (the type ids range from 0 to tbl->totalNumTypeDefs-1 (inclusive))
 */
void GenTypeDefIds PARAMS((tbl, m), TBL* tbl _AND_ Module* m)
{
	TypeDef* td;

	tbl->totalNumModules++;
	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		td->tmpRefCount = tbl->totalNumTypeDefs;
		tbl->totalNumTypeDefs++;
	}

} /* GenTypeDefIds */

/*
 * builds a TBLModule from the given module and appends it to
 * the given TBL's module list.  Also updates the TBLs
 * totals for modules, tags, typedefs and types.
 * Returns TRUE is succeeded. FALSE is failed.
 */
int GenTblModule PARAMS((tbl, m, newTblMod), TBL* tbl _AND_ Module* m _AND_ TBLModule** newTblMod)
{
	TBLModule** mHndl;
	TBLModule* tblMod;
	int eLen;

	mHndl = AsnListAppend(tbl->modules);

	tblMod = MT(TBLModule);
	*newTblMod = *mHndl = tblMod;

	/* copy the name */
	tblMod->name.octetLen = strlen(m->modId->name);
	size_t size = tblMod->name.octetLen + 1;
	tblMod->name.octs = Malloc(size + 1);
	strcpy_s(tblMod->name.octs, size, m->modId->name);
	tbl->totalNumStrings++;
	tbl->totalLenStrings += (AsnInt)tblMod->name.octetLen;

	/* copy the OBJECT IDENTIFIER (if any) */
	if (m->modId->oid != NULL)
	{
		/* convert the (linked) OID into a (encoded) AsnOid */
		/* RWC;10/16/00; UPDATED NULL,0,0 to avoid compiler error
			after proto added (??? UNTESTED ???).*/
		if (FlattenLinkedOid(m->modId->oid, NULL, 0, 0))
		{
			eLen = EncodedOidLen(m->modId->oid);
			tblMod->id.octetLen = eLen;
			tblMod->id.octs = (char*)Malloc(eLen);
			BuildEncodedOid(m->modId->oid, &tblMod->id);
			tbl->totalNumStrings++;
			tbl->totalLenStrings += eLen;
		}
	}

	/*
	 * useful defaults to false
	 * (ie assume the it is not the usefultypes modules)
	 */
	tblMod->isUseful = FALSE;

	/* now copy each of the type defs */
	return GenTblTypeDefs(tbl, m, tblMod);

} /* GenTblModule */

/*
 * converts typeDefs in Module format to TBLModule format
 * returns TRUE for success, FALSE for failure.
 */
int GenTblTypeDefs PARAMS((tbl, m, tblMod), TBL* tbl _AND_ Module* m _AND_ TBLModule* tblMod)
{
	TypeDef* td;
	TBLTypeDef** tblTdHndl;
	TBLTypeDef* tblTd;
	SnaccDirective* pDirective;
	int isOk = TRUE; /* init to no errors */

	tblMod->typeDefs = AsnListNew(sizeof(void*));
	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		tblTd = MT(TBLTypeDef);

		/* set type def id */
		tblTd->typeDefId = td->tmpRefCount;

		/* copy type def name */
		tblTd->typeName.octetLen = strlen(td->definedName);
		size_t size = tblTd->typeName.octetLen + 1;
		tblTd->typeName.octs = Malloc(size + 1);
		strcpy_s(tblTd->typeName.octs, size, td->definedName);
		tbl->totalNumStrings++;
		tbl->totalLenStrings += (AsnInt)tblTd->typeName.octetLen;

		/*
				if (td->isPdu)
				tblTd->isPdu = MT (AsnNull);
		*/

		FOR_EACH_LIST_ELMT(pDirective, td->attrList)
		{
			switch (pDirective->type)
			{
				case IsPtr:
					/* pDirective->value.boolVal;	Nothing to set this to! */
					break;

				default:
					fprintf(errFileG, "Warning: ignoring unrecognized type def attribute '%s'\n", GetDirectiveName(pDirective->type));
			}
		}

		/* fill in type portion */
		if (!GenTblTypes(tbl, m, tblMod, td, tblTd) && !abortTblTypeDefG)
			isOk = FALSE;

		/*
		 * add TBLtypeDef to TBLModule
		 * if no weird types were found
		 * (weird types are skipped)
		 */
		if (!abortTblTypeDefG)
		{
			tblTdHndl = AsnListAppend(tblMod->typeDefs);
			*tblTdHndl = tblTd;
			tbl->totalNumTypes += tblTypesTotalG;
			tbl->totalNumTags += tblTagsTotalG;
			tbl->totalNumStrings += tblStringsTotalG;
			tbl->totalLenStrings += tblStringLenTotalG;
		}
		/* else could free it */
	}

	return isOk;
} /* GenTblTypeDefs */

/*
 * converts Module Type to a TBLModule Type.  attaches converted
 * type info to the given tblTd.
 * Returns TRUE for success, FALSE for failure.
 */
int GenTblTypes PARAMS((tbl, m, tblMod, td, tblTd), TBL* tbl _AND_ Module* m _AND_ TBLModule* tblMod _AND_ TypeDef* td _AND_ TBLTypeDef* tblTd)
{
	abortTblTypeDefG = FALSE;
	tblTypesTotalG = 0;
	tblTagsTotalG = 0;
	tblStringsTotalG = 0;
	tblStringLenTotalG = 0;

	tblTd->type = GenTblTypesRec(tbl, m, tblMod, td, tblTd, td->type);

	if (tblTd->type == NULL)
		return FALSE; /* failed */
	else
		return TRUE;

} /* GenTblTypes */

BasicValue* GetTblValue PARAMS((v), Value* v)
{
	switch (v->basicValue->choiceId)
	{
		case BASICVALUE_INTEGER:
			return v->basicValue;
		default:
			return NULL;
	}
}

enum BasicTypeChoiceId GetTblBasicType PARAMS((bt), BasicType* bt)
{
	switch (bt->choiceId)
	{
		case BASICTYPE_LOCALTYPEREF:
		case BASICTYPE_IMPORTTYPEREF:
			return GetTblBasicType(bt->a.localTypeRef->link->type->basicType);
		default:
			return bt->choiceId;
	}
}

TBLType* GenTblTypesRec PARAMS((tbl, m, tblMod, td, tblTd, t), TBL* tbl _AND_ Module* m _AND_ TBLModule* tblMod _AND_ TypeDef* td _AND_ TBLTypeDef* tblTd _AND_ Type* t)
{
	TBLType* tblT;
	NamedType* e;
	TBLType** tblTHndl;
	Tag* tag;
	TBLTag** tblTagHndl;

	tblTypesTotalG++;
	tblT = MT(TBLType);
	tblT->content = MT(TBLTypeContent);
	switch (t->basicType->choiceId)
	{
		case BASICTYPE_BOOLEAN:
			tblT->typeId = TBL_BOOLEAN;
			tblT->content->choiceId = TBLTYPECONTENT_PRIMTYPE;
			break;

		case BASICTYPE_INTEGER:
			tblT->typeId = TBL_INTEGER;
			tblT->content->choiceId = TBLTYPECONTENT_PRIMTYPE;
			break;

		case BASICTYPE_BITSTRING:
			tblT->typeId = TBL_BITSTRING;
			tblT->content->choiceId = TBLTYPECONTENT_PRIMTYPE;
			/*	  tblT->values = GenTblValues(tbl,m,tblMod,t->basicType->a.bitString); */
			break;

		case BASICTYPE_OCTETSTRING:
			tblT->typeId = TBL_OCTETSTRING;
			tblT->content->choiceId = TBLTYPECONTENT_PRIMTYPE;
			/*	  if (t->subtypes)
					  tblT->constraint = GenTblValueRange(tbl, m, tblMod,t->subtypes,1); */
			break;

		case BASICTYPE_NULL:
			tblT->typeId = TBL_NULL;
			tblT->content->choiceId = TBLTYPECONTENT_PRIMTYPE;
			break;

		case BASICTYPE_OID:
			tblT->typeId = TBL_OID;
			tblT->content->choiceId = TBLTYPECONTENT_PRIMTYPE;
			break;

		case BASICTYPE_RELATIVE_OID:
			tblT->typeId = TBL_RELATIVE_OID;
			tblT->content->choiceId = TBLTYPECONTENT_PRIMTYPE;
			break;

		case BASICTYPE_REAL:
			tblT->typeId = TBL_REAL;
			tblT->content->choiceId = TBLTYPECONTENT_PRIMTYPE;
			break;

		case BASICTYPE_ENUMERATED:
			tblT->typeId = TBL_ENUMERATED;
			tblT->content->choiceId = TBLTYPECONTENT_PRIMTYPE;
			/*	  tblT->values = GenTblValues(tbl,m,tblMod,t->basicType->a.enumerated); */
			break;

		case BASICTYPE_SEQUENCE:
			tblT->typeId = TBL_SEQUENCE;
			tblT->content->choiceId = TBLTYPECONTENT_ELMTS;
			tblT->content->a.elmts = AsnListNew(sizeof(void*));
			FOR_EACH_LIST_ELMT(e, t->basicType->a.sequence)
			{
				tblTHndl = AsnListAppend(tblT->content->a.elmts);
				*tblTHndl = GenTblTypesRec(tbl, m, tblMod, td, tblTd, e->type);

				if (*tblTHndl == NULL)
					break;

				if (e->fieldName != NULL)
				{
					(**tblTHndl).fieldName.octetLen = strlen(e->fieldName);
					size_t size = (**tblTHndl).fieldName.octetLen + 1;
					(**tblTHndl).fieldName.octs = Malloc(size + 1);
					strcpy_s((**tblTHndl).fieldName.octs, size, e->fieldName);
					tblStringsTotalG++;
					tblStringLenTotalG += (int)((**tblTHndl).fieldName.octetLen);
				}

				(**tblTHndl).optional = (unsigned char)((e->type->optional) || (e->type->defaultVal != NULL));
			}

			break;

		case BASICTYPE_SET:
			tblT->typeId = TBL_SET;
			tblT->content->choiceId = TBLTYPECONTENT_ELMTS;
			tblT->content->a.elmts = AsnListNew(sizeof(void*));
			FOR_EACH_LIST_ELMT(e, t->basicType->a.set)
			{
				tblTHndl = AsnListAppend(tblT->content->a.elmts);
				*tblTHndl = GenTblTypesRec(tbl, m, tblMod, td, tblTd, e->type);

				if (*tblTHndl == NULL)
					break;

				if (e->fieldName != NULL)
				{
					(**tblTHndl).fieldName.octetLen = strlen(e->fieldName);
					size_t size = (**tblTHndl).fieldName.octetLen + 1;
					(**tblTHndl).fieldName.octs = Malloc(size + 1);
					strcpy_s((**tblTHndl).fieldName.octs, size, e->fieldName);
					tblStringsTotalG++;
					tblStringLenTotalG += (int)((**tblTHndl).fieldName.octetLen);
				}

				(**tblTHndl).optional = (unsigned char)((e->type->optional) || (e->type->defaultVal != NULL));
			}
			break;

		case BASICTYPE_SEQUENCEOF:
			tblT->typeId = TBL_SEQUENCEOF;
			tblT->content->choiceId = TBLTYPECONTENT_ELMTS;
			tblT->content->a.elmts = AsnListNew(sizeof(void*));
			tblTHndl = AsnListAppend(tblT->content->a.elmts);
			*tblTHndl = GenTblTypesRec(tbl, m, tblMod, td, tblTd, t->basicType->a.sequenceOf);
			/*	  if (t->subtypes)
					  tblT->constraint = GenTblValueRange(tbl, m, tblMod,t->subtypes,1); */
			break;

		case BASICTYPE_SETOF:
			tblT->typeId = TBL_SETOF;
			tblT->content->choiceId = TBLTYPECONTENT_ELMTS;
			tblT->content->a.elmts = AsnListNew(sizeof(void*));
			tblTHndl = AsnListAppend(tblT->content->a.elmts);
			*tblTHndl = GenTblTypesRec(tbl, m, tblMod, td, tblTd, t->basicType->a.setOf);
			/*	  if (t->subtypes)
					  tblT->constraint = GenTblValueRange(tbl, m, tblMod,t->subtypes,1); */
			break;

		case BASICTYPE_CHOICE:
			tblT->typeId = TBL_CHOICE;
			tblT->content->choiceId = TBLTYPECONTENT_ELMTS;
			tblT->content->a.elmts = AsnListNew(sizeof(void*));
			FOR_EACH_LIST_ELMT(e, t->basicType->a.set)
			{
				tblTHndl = AsnListAppend(tblT->content->a.elmts);
				*tblTHndl = GenTblTypesRec(tbl, m, tblMod, td, tblTd, e->type);

				if (*tblTHndl == NULL)
					break;

				if (e->fieldName != NULL)
				{
					(**tblTHndl).fieldName.octetLen = strlen(e->fieldName);
					size_t size = (**tblTHndl).fieldName.octetLen + 1;
					(**tblTHndl).fieldName.octs = Malloc(size + 1);
					strcpy_s((**tblTHndl).fieldName.octs, size, e->fieldName);
					tblStringsTotalG++;
					tblStringLenTotalG += (int)((**tblTHndl).fieldName.octetLen);
				}

				(**tblTHndl).optional = (unsigned char)((e->type->optional) || (e->type->defaultVal != NULL));
			}
			break;

		case BASICTYPE_LOCALTYPEREF:
		case BASICTYPE_IMPORTTYPEREF:
			tblT->typeId = TBL_TYPEREF;
			tblT->content->choiceId = TBLTYPECONTENT_TYPEREF;
			tblT->content->a.typeRef = MT(TBLTypeRef);
			tblT->content->a.typeRef->implicit = t->implicit;
			tblT->content->a.typeRef->typeDef = t->basicType->a.localTypeRef->link->tmpRefCount;
			break;

		default:
			if (!abortTblTypeDefG) /* only print first time  */

				fprintf(errFileG, "WARNING: Type definition \"%s\" will not be included in the type table because it contains a weird type.\n", td->definedName);
			abortTblTypeDefG = TRUE;
			Free(tblT->content);
			Free(tblT);
			tblT = NULL;
			break;
	}

	/* handle constraints */
	if (t->subtypes)
	{
		switch (GetTblBasicType(t->basicType))
		{
			case BASICTYPE_INTEGER:
				/*	    tblT->constraint = GenTblValueRange(tbl,m,tblMod,t->subtypes,0); */
				break;
			case BASICTYPE_OCTETSTRING:
			case BASICTYPE_SEQUENCEOF:
				/*	    tblT->constraint = GenTblValueRange(tbl,m,tblMod,t->subtypes,1); */
				break;
			default:
				break;
		}
	}

	/* copy the tags */
	if ((tblT != NULL) && ((t->tags != NULL) && (!LIST_EMPTY(t->tags))))
	{
		tblT->tagList = AsnListNew(sizeof(void*));
		FOR_EACH_LIST_ELMT(tag, t->tags)
		{
			tblTagsTotalG++;
			tblTagHndl = AsnListAppend(tblT->tagList);
			*tblTagHndl = MT(TBLTag);
			switch (tag->tclass)
			{
				case UNIV:
					(**tblTagHndl).tclass = UNIVERSAL;
					break;
				case APPL:
					(**tblTagHndl).tclass = APPLICATION;
					break;
				case CNTX:
					(**tblTagHndl).tclass = CONTEXT;
					break;
				case PRIV:
					(**tblTagHndl).tclass = PRIVATE;
					break;
			}
			(**tblTagHndl).code = tag->code;
		}
	}

	return tblT;
} /* GenTblTypesRec */
