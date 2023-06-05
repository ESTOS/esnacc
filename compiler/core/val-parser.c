/*
 * compiler/core/val_parser.c
 *         given a string with txt ASN.1 value notation, the length of
 *         the string and the ASN.1 type the value notion defines a value
 *         for, return a Value that contains the internal version
 *
 *
 * currently limited to parsing OBJECT IDENTIFIERs.
 * should be easy to extend for other values as needed
 *
 * Copyright (C) 1991, 1992 Michael Sample
 *            and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/core/val-parser.c,v 1.1.1.1 2005/04/14 14:59:43 \ste Exp $
 * $Log: val-parser.c,v $
 * Revision 1.1.1.1  2005/04/14 14:59:43  \ste
 * no message
 *
 * Revision 1.10  2004/04/06 15:13:41  gronej
 * *** empty log message ***
 *
 * Revision 1.9  2004/01/14 19:07:53  gronej
 * Updated Compiler to accept and process relative-oid's
 *
 * Revision 1.8  2003/07/14 21:07:44  nicholar
 * Changed how parser handles --snacc directives.  Added namespace option.
 *
 * Revision 1.7  2003/07/07 14:50:14  nicholar
 * Eliminated headers and cleaned up include references
 *
 * Revision 1.6  2003/04/29 21:06:37  leonberp
 * integerated Deepak's changes for IOB support
 *
 * Revision 1.5  2002/09/16 18:01:16  mcphersc
 * fixed warning
 *
 * Revision 1.4  2002/09/16 16:50:33  mcphersc
 * Fixed warnings
 *
 * Revision 1.3  2002/09/04 18:31:38  vracarl
 * got rid of c++ comments
 *
 * Revision 1.2  2000/10/24 14:54:55  rwc
 * Updated to remove high-level warnings (level 4 on MSVC++) for an easier build.
 * SOME warnings persist due to difficulty in modifying the SNACC compiler to
 * properly build clean source; also some files are built by Lex/Yacc.
 *
 * Revision 1.1.1.1  2000/08/21 20:36:02  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.4  1997/10/10 13:43:16  wan
 * Corrected bug in generic table decoder wrt. indefinite length elements
 * Corrected compiler access to freed memory (bug reported by Markku Savela)
 * Broke asnwish.c into two pieces so that one can build ones on wish
 * Added beredit tool (based on asnwish, allowes to edit BER messages)
 *
 * Revision 1.3  1995/07/25 19:41:46  rj
 * changed `_' to `-' in file names.
 *
 * Revision 1.2  1994/09/01  00:47:33  rj
 * snacc_config.h removed; val_parser.h includet.
 *
 * Revision 1.1  1994/08/28  09:49:44  rj
 * first check-in. for a list of changes to the snacc-1.1 distribution please refer to the ChangeLog.
 *
 */

#include <string.h>
#include <ctype.h> /* for isalpha, isdigit etc macros */
#include "../../c-lib/include/asn-incl.h"
#include "asn1module.h"
#include "snacc-util.h"

int OidArcNameToNum PROTO((char* name));

/*
 * a bunch of macros for 'standard' parse routines
 */
#define P_LOCALS char* startStr

#define SAVE_POS() startStr = *vStr;

#define RESTORE_POS() *vStr = startStr;

#define AT_EOF() (*vStr == eof)

#define FAIL()                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
	{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
		if (*vStr > farthestPosG)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
			farthestPosG = *vStr;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
		RESTORE_POS();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
		return FALSE;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
	}

#define SUCCEED() return TRUE;

#define PRINT_VAL(vd) PrintValueDef(errFileG, vd);

/*
 * globals
 */
static ValueDefList* newValsG;
static int parseValuesErrG;
static unsigned long valLineNoG;
static char* farthestPosG;

/*
 * prototypes for non-exported routines
 */
char* StripComments PROTO((char* asn1Str, size_t len));
Value* ParseValue PROTO((ModuleList * mods, Module* m, ValueDef* vd, Type* t, char* valueNotation, size_t len));

Value* ParseValueInternal PROTO((ModuleList * mods, Module* m, ValueDef* vd, Type* t, char** valueNotation, char* eof));

int ParseOidValue PROTO((ModuleList * mods, Module* m, ValueDef* vd, Type* t, char** valueNotation, char* eof, Value** result));

void SkipWht PROTO((char** vStr, char* eof));
int ParseIdentifier PROTO((char** valueNotation, char* eof, char** result));
int ParseNum PROTO((char** valueNotation, char* eof, char** result));
void AddNewValueDef PROTO((ValueDefList * vdl, char* name, Value* value));

/*
 * returns 0 if no parse errors occurred
 * otherwise returns non-zero
 */
int ParseValues PARAMS((mods, m), ModuleList* mods _AND_ Module* m)
{
	ValueDef* v;
	Value* pv;

	newValsG = AsnListNew(sizeof(void*));

	FOR_EACH_LIST_ELMT(v, m->valueDefs)
	{
		if (v->value->basicValue->choiceId == BASICVALUE_VALUENOTATION)
		{
			valLineNoG = v->value->lineNo;
			pv = ParseValue(mods, m, v, v->value->type, v->value->basicValue->a.valueNotation->octs, v->value->basicValue->a.valueNotation->octetLen);

			/* replace value notation value with parsed version */
			if (pv != NULL)
			{
				pv->lineNo = v->value->lineNo;
				pv->type = v->value->type;
				Free(v->value->basicValue->a.valueNotation->octs);
				Free(v->value->basicValue->a.valueNotation);
				Free(v->value->basicValue);
				Free(v->value);
				v->value = pv;
			}
		}
	}

	/*
	 * should traverse type structures for default values etc
	 * that need parsing
	 */

	/* add any new value defs */
	m->valueDefs = AsnListConcat(m->valueDefs, newValsG);
	Free(newValsG);

	return parseValuesErrG;

} /* ParseValues */

/*
 * returns the Value that resuls from parsing the given
 * value notation string
 */
Value* ParseValue PARAMS((mods, m, vd, t, valueNotationStr, vnLen), ModuleList* mods _AND_ Module* m _AND_ ValueDef* vd _AND_ Type* t _AND_ char* valueNotationStr _AND_ size_t vnLen)
{
	char* vStr;
	char* vStrOrig;
	size_t vStrLen;
	Value* retVal;

	/* make copy of value notation with ASN.1 comments zapped */
	vStrOrig = vStr = StripComments(valueNotationStr, vnLen);
	vStrLen = strlen(vStr);

	retVal = ParseValueInternal(mods, m, vd, t, &vStr, (vStr + vStrLen));

	/* use original since parsing has changed vStr */
	// Deepak: changed from free to Free
	Free(vStrOrig);

	return retVal;
}

/*
 * vStr is a handle to a commentless ASN.1 value string,
 * eof is a char * to character after the last valid character
 * in vStr.  vStr will be advanced to the current parse location.
 */
Value* ParseValueInternal PARAMS((mods, m, vd, t, vStr, eof), ModuleList* mods _AND_ Module* m _AND_ ValueDef* vd _AND_ Type* t _AND_ char** vStr _AND_ char* eof)
{
	Type* dT;
	Value* retVal;
	int parseResult = FALSE;

	dT = ParanoidGetType(t); /* skip type refs to get defining type */

	if (dT == NULL)
		return NULL;

	retVal = NULL;

	switch (dT->basicType->choiceId)
	{
		case BASICTYPE_SEQUENCE:
		case BASICTYPE_SET:
		case BASICTYPE_CHOICE:
		case BASICTYPE_SEQUENCEOF:
		case BASICTYPE_SETOF:
			/* don't do constructed types yet */
			break;

		case BASICTYPE_SELECTION:
		case BASICTYPE_COMPONENTSOF:
		case BASICTYPE_ANYDEFINEDBY:
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_ANY:
			/* don't do weird types */
			break;

			/*
			 * The following simple types will need to be filled in
			 * when the constructed types are parsed.
			 * (ie ParseValueInternal becomes recursive)
			 * (currenly all simple types not in {}'s are parsed
			 * in the main yacc parser.)
			 */

		case BASICTYPE_BOOLEAN:
			break;

		case BASICTYPE_INTEGER:
		case BASICTYPE_ENUMERATED:
			break;

		case BASICTYPE_REAL:
			break;

		case BASICTYPE_BITSTRING:
			break;

		case BASICTYPE_NULL:
			break;

		case BASICTYPE_OCTETSTRING:
			break;

		case BASICTYPE_OCTETCONTAINING:
			break;

			/* assume all macro values in {}'s are OID values */
		case BASICTYPE_OID:
		case BASICTYPE_RELATIVE_OID:
		case BASICTYPE_MACROTYPE:
			parseResult = ParseOidValue(mods, m, vd, t, vStr, eof, &retVal);
			if (!parseResult)
			{
				parseValuesErrG = 1;
				PrintErrLoc(m->asn1SrcFileName, (long)valLineNoG);
			}
			break;

		default:
			break;
	}

	if (parseResult)
		return retVal;
	else
		return NULL;

} /* ParseValueInternal */

/*
 *  Strips ASN.1 comments from the given string.
 *  returns a null terminated malloc'd copy without the comments
 */
char* StripComments PARAMS((s, len), char* s _AND_ size_t len)
{
	char* cpy;
	size_t sIndex, cpyIndex;

	cpy = (char*)Malloc(len + 1);
	cpyIndex = 0;
	for (sIndex = 0; sIndex < len;)
	{
		if ((s[sIndex] == '-') && ((sIndex + 1) < len) && (s[sIndex + 1] == '-'))
		{
			/* eat comment body */
			for (sIndex += 2; sIndex < len;)
				if ((s[sIndex] == '-') && ((sIndex + 1) < len) && (s[sIndex + 1] == '-'))
				{
					sIndex += 2;
					break; /* exit for */
				}
				else if (s[sIndex] == '\n')
				{
					sIndex++;
					break; /* exit for */
				}
				else
					sIndex++;
		}
		else /* not in or start of comment */
			cpy[cpyIndex++] = s[sIndex++];
	}

	cpy[cpyIndex] = '\0'; /* add  NULL terminator */
	return cpy;
} /* StripComments */

/*
 * Returns TRUE if successfully parsed an OID
 * otherwise returns FALSE.  Puts the resulting OID Value in
 * result if successful.
 *  The result Value's type is BASICVALUE_LINKEDOID
 *
 * Pseudo reg expr of the expected oid format:
 *  "{"
 *    (oid_val_ref)?
 *(defined_oid_elmt_name | digit+ | int_or_enum_val_ref |name"(" digit")")*
 * "}"
 *
 * Does not attempt to link/lookup referenced values
 *
 * eg
 * for { ccitt foo (1) bar bell (bunt) 2 }
 *
 * ccitt
 *    arcnum is set to number from oid table (oid.c)
 * foo (1)
 *   - arcnum set to 1
 *   - sets up a new integer value def "foo"
 *      defined as 1 *CHANGED -see changes*
 *   - makes oid valueref a value ref to foo (doesn't link it tho)
 * bar
 *   - makes oid valueref a value ref to bar (doesn't link it tho)
 * bell (bunt)
 *   - sets up a new integer value def "bell" defined
 *      as a val ref to "bunt" *CHANGED -see changes*
 *   - makes oid valueref a value ref to bell (doesn't link it tho)
 * 2
 *  -arc num is set to 2
 *
 * CHANGES:
 *   93/05/03 - named arcs such as foo (1) or bell (bunt) handling
 *              changed. The names (foo and bell) are now ignored
 *              and *do not* define new integer values.
 *              The old way led to problems of defining some values
 *              more than once. E.g. in X.500 the { .. ds (5) }
 *              arc name is used everywhere - "ds INTEGER ::= 5"
 *              was defined multiple times as a result.
 *              Then the snacc error checker halted the compilation
 *              since the integer value "ds" was mulitply defined.
 *
 */
int ParseOidValue PARAMS((mods, m, vd, t, vStr, eof, result), ModuleList* mods _AND_ Module* m _AND_ ValueDef* vd _AND_ Type* t _AND_ char** vStr _AND_ char* eof _AND_ Value** result)
{
	Value* newVal;
	Value* oidVal;
	OID* parsedOid;
	OID** nextOid;
	char* id;
	char* id2;
	char* id3;
	char* num;
	int arcNum;
	P_LOCALS;

	SAVE_POS();

	if (AT_EOF())
	{
		fprintf(errFileG, "ERROR - expecting more data in OBJECT IDENTIFER value\n");
		PrintErrLoc((char*)m, (long)vd->value->lineNo);
		FAIL();
	}

	SkipWht(vStr, eof);

	if (**vStr != '{')
	{
		PrintErrLoc((char*)m, (long)vd->value->lineNo);
		fprintf(errFileG, "ERROR - OBJECT IDENTIFER values must begin with an \"{\".\n");
		FAIL();
	}
	else
		(*vStr)++; /* skip opening { */

	SkipWht(vStr, eof);

	parsedOid = NULL;
	nextOid = &parsedOid;

	while (**vStr != '}')
	{
		if (ParseIdentifier(vStr, eof, &id))
		{
			/*
			 * check for named number ident (num) or ident (valref)
			 * make a new value def with the name ident if is name
			 * and number form
			 */
			SkipWht(vStr, eof);
			if (**vStr == '(')
			{

				(*vStr)++; /* skip opening ( */
				SkipWht(vStr, eof);

				arcNum = NULL_OID_ARCNUM;
				/*
				 * ident (num)/ident (valref) yields a new value definition
				 * ident.  The oid then refences this new value def.
				 */

				/*
				 * first case check if of form
				 * { ... ident (valref) ... }
				 */
				if (ParseIdentifier(vStr, eof, &id2))
				{
					id3 = NULL;
					/* check if modname.val format */
					if (**vStr == '.')
					{
						(*vStr)++;
						if (!ParseIdentifier(vStr, eof, &id3))
						{
							PrintErrLoc((char*)m, (long)vd->value->lineNo);
							fprintf(errFileG, "ERROR - missing a module name after the \"%s.\" value reference", id2);
							FAIL();
						}
					}

					/* grab closing ) */
					SkipWht(vStr, eof);
					if (**vStr == ')')
						(*vStr)++;
					else
					{
						PrintErrLoc((char*)m, (long)vd->value->lineNo);
						fprintf(errFileG, "ERROR - missing a closing \")\", after the \"%s\" value reference.\n", id2);
						FAIL();
					}

					if (id3 != NULL) /* modname.val format */
					{
						SetupValue(&newVal, BASICVALUE_IMPORTVALUEREF, valLineNoG);
						newVal->basicValue->a.importValueRef = (ValueRef*)Malloc(sizeof(ValueRef));
						newVal->basicValue->a.importValueRef->valueName = id2;
						newVal->basicValue->a.importValueRef->moduleName = id3;

						AddPrivateImportElmt(m, id2, id3, valLineNoG);
					}
					else
					{
						SetupValue(&newVal, BASICVALUE_LOCALVALUEREF, valLineNoG);
						newVal->basicValue->a.localValueRef = (ValueRef*)Malloc(sizeof(ValueRef));
						newVal->basicValue->a.localValueRef->valueName = id2;
					}
				}
				/* check this form { ... ident (2)...}*/
				else if (ParseNum(vStr, eof, &num))
				{
					/* grab closing ) */
					SkipWht(vStr, eof);
					if (**vStr == ')')
						(*vStr)++;
					else
					{
						PrintErrLoc((char*)m, (long)vd->value->lineNo);
						fprintf(errFileG, "ERROR - missing a closing \")\" after the \"%s (%s\".\n", id2, num);
						Free(num);
						FAIL();
					}
					arcNum = atoi(num);
					Free(num);
					newVal = NULL;
				}
				else /* neither an ident or num after the "(" */
				{
					PrintErrLoc((char*)m, (long)vd->value->lineNo);
					fprintf(errFileG, "ERROR - expecting either a value reference or number after the \"(\".\n");
					FAIL();
				}

				*nextOid = (OID*)Malloc(sizeof(OID));
				(*nextOid)->valueRef = newVal;
				(*nextOid)->arcNum = arcNum;
				nextOid = &(*nextOid)->next;

			} /* end of ident (num) and ident (ident) form */

			else /* value ref: { ... ident .... } */
			{
				*nextOid = (OID*)Malloc(sizeof(OID));
				(*nextOid)->arcNum = NULL_OID_ARCNUM;

				/*
				 * check if special defined oid elmt name
				 * like joint-iso-ccitt, iso, standard etc.
				 */

				arcNum = OidArcNameToNum(id);
				if (arcNum != -1)
				{
					(*nextOid)->arcNum = arcNum;
				}
				else /* value reference */
				{
					SetupValue(&newVal, BASICVALUE_LOCALVALUEREF, valLineNoG);
					newVal->basicValue->a.localValueRef = (ValueRef*)Malloc(sizeof(ValueRef));
					newVal->basicValue->a.localValueRef->valueName = id;

					(*nextOid)->valueRef = newVal;
				}
				nextOid = &(*nextOid)->next;
			}
		}
		else if (ParseNum(vStr, eof, &num)) /* { .. 2 .. } */
		{
			*nextOid = (OID*)Malloc(sizeof(OID));
			(*nextOid)->arcNum = atoi(num);
			nextOid = &(*nextOid)->next;
			Free(num);
		}
		else
		{
			PrintErrLoc((char*)m, (long)vd->value->lineNo);
			fprintf(errFileG, "ERROR - bady formed arc number\n");
			FAIL();
		}

		SkipWht(vStr, eof);
	}

	(*vStr)++; /* move over closing } */

	SetupValue(&oidVal, BASICVALUE_LINKEDOID, valLineNoG);
	oidVal->basicValue->a.linkedOid = parsedOid;
	*result = oidVal;
	SUCCEED();
}

void SkipWht PARAMS((vStr, eof), char** vStr _AND_ char* eof)
{
	while (!AT_EOF())
		switch (**vStr)
		{
			case '\n': /* newline */
			case '\f': /* form feed  ?*/
			case '\v': /* vertical tab ?*/
			case '\r':
				valLineNoG++; /* carriage return */
			case '\t':		  /* tab */
			case ' ':		  /* space */
			case '\007':	  /* bell? */
			case '\b':		  /* back spc */
				(*vStr)++;
				break;

			default:
				return;
		}
}

/*
 * advances the vStr over ASN.1 identifier, returns a copy
 * in result, and returns TRUE. otherwise returns FALSE
 *
 * ASN.1 identifier is: lowercase letter followed by a
 * string of letters (upper and lower case allowed), digtits, or single
 * hyphens. last char cannot be a hyphen.
 */
int ParseIdentifier PARAMS((vStr, eof, result), char** vStr _AND_ char* eof _AND_ char** result)
{
	char* start;
	size_t len;
	P_LOCALS;

	SAVE_POS();

	if (AT_EOF())
		FAIL();

	start = *vStr;
	if (!islower(**vStr))
		FAIL();

	(*vStr)++;

	while (!AT_EOF())
	{
		/* allow letters, digits  and single hyphens */
		if ((isalpha(**vStr)) || isdigit(**vStr) || ((**vStr == '-') && !(*(*vStr - 1) == '-')))
			(*vStr)++;
		else
			break; /* exit for loop */
	}

	/* don't allow hyphens on the end */
	if (*(*vStr - 1) == '-')
		(*vStr)--;

	len = *vStr - start;
	*result = Malloc(len + 1);
	strncpy_s(*result, len + 1, start, len);
	(*result)[len] = '\0'; /* null terminate */

	SUCCEED();
} /* ParseIdentifier */

/*
 * advances the vStr over ASN.1 number, returns a
 * null terminated ascii copy of the number
 * in result, and returns TRUE. otherwise returns FALSE
 */
int ParseNum PARAMS((vStr, eof, result), char** vStr _AND_ char* eof _AND_ char** result)
{
	P_LOCALS;
	char* start;
	size_t len;

	SAVE_POS();

	if (AT_EOF())
		FAIL();

	start = *vStr;

	while (!AT_EOF())
		if (isdigit(**vStr))
			(*vStr)++;
		else
			break; /* exit for loop */

	len = *vStr - start;

	if (len == 0)
		FAIL();

	*result = Malloc(len + 1);
	strncpy_s(*result, len + 1, start, len);
	(*result)[len] = '\0'; /* null terminate */

	SUCCEED();
} /* ParseNum */

/*
 * adds a new value def to the vdl.  Used
 * when parsing oid's that defined arc values
 * eg { 1 2 foo (3) } --> defined foo INTEGER ::= 3
 * (should be  foo INTEGER (0..MAX) ::= 3)
 */
void AddNewValueDef PARAMS((vdl, name, value), ValueDefList* vdl _AND_ char* name _AND_ Value* value)
{
	ValueDef* vd;
	ValueDef** tmpVd;

	vd = (ValueDef*)Malloc(sizeof(ValueDef));
	vd->definedName = name;
	vd->value = value;
	tmpVd = (ValueDef**)AsnListAppend(vdl);
	*tmpVd = vd;
} /* AddNewValueDef */
