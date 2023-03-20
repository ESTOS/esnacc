/*
*   compiler/back_ends/c++_gen/gen_code.c - routines for printing C++ code from type trees
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
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* $Header: /develop30/common/esnacc1.7/SNACC/compiler/back-ends/c++-gen/gen-code.c,v 1.15 2008/04/15 07:29:30 \ste Exp $
*
*/

#include "gen-code.h"

#include "../../../c-lib/include/print.h"
#include "../../core/print.h"
#include "../tag-util.h"  /* get GetTags/FreeTags/CountTags/TagByteLen */
#include "../structure-util.h"
#include "cxxconstraints.h"
#include "cxxmultipleconstraints.h"
#include "../../core/asn_comments.h"

#if META
#include "meta.h"
#endif

static void PrintCxxSeqSetPrintFunction(FILE* src, FILE* hdr,
										MyString className,
										BasicType *pBasicType);


										/* PER specific methods*/
static void PrintCxxDefCode_SetSeqPEREncode (FILE *src, FILE *hdr, CxxRules *r, TypeDef *td,
    NamedType **pSetElementNamedType,
    int iElementCount);      /* IN, ELEMENT Count to process in arrays */

static void PrintCxxDefCode_SetSeqPERDecode (FILE *src, FILE *hdr, CxxRules *r, TypeDef *td,
    NamedType **pSetElementNamedType,
    int iElementCount);      /* IN, ELEMENT Count to process in arrays */

static void PrintCxxDefCode_PERSort (
    NamedType ***pppElementNamedType, /* OUT, array of sorted NameType(s) */
    int **ppElementTag,      /* OUT, actual tag for sorted. */
    AsnList *pElementList);  /* IN, actual eSNACC defs for NameType(s). */

/* flag to see if constraints were present */
int constraints_flag;
long lconstraintvar=0;

int genCodeCPPPrintStdAfxInclude = 0;

long longJmpValG = -100;
int printTypesG = 0;
int printEncodersG = 0;
int printDecodersG = 0;
int printJSONEncDecG = 0;
int printPrintersG = 0;
int printPrintersXMLG = 0;
int printFreeG = 0;


// normalizeValue
//
// strip whitespace and { } from valueNation values.
//
void normalizeValue(char **normalized, char *input)
{
	size_t i;
	while(*input == ' ' || *input == '{' )
		input++;

	*normalized = _strdup(input);

	i = strlen(*normalized) - 1;
	while ( (*normalized)[i] == ' ' || (*normalized)[i] == '}' )
	{
		(*normalized)[i] = 0;
		i--;
	}
}

static char *GetImportForwardDeclFileName (char *Impname, ModuleList *mods)
{
	Module *currMod;
	char *fileName = NULL;
	FOR_EACH_LIST_ELMT (currMod, mods)
	{
		/* Find the import Module in the Modules and
		* return the header file name
		*/
		if ((strcmp(Impname, currMod->modId->name) == 0))
		{
			/* Set the file name and break */
			fileName = currMod->ROSEHdrForwardDeclFileName;
			break;
		}
	}
	return fileName;
}

/* RWC; added 7/25/03 */
static Module *GetImportModuleRef (char *Impname, ModuleList *mods)
{
	Module *currMod=NULL;
	FOR_EACH_LIST_ELMT (currMod, mods)
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

/* PIERCE added 8-22-2001 */
void PrintCopyConstructor(FILE *hdr, FILE *src, char *className)
{
	fprintf(hdr, "   %s(const %s& that);\n", className, className);
	fprintf(src,"%s::%s(const %s &that)\n{\n",className, className,className);
	/*fprintf(src,"   Clear();\n");*/
	fprintf(src,"   Init();\n");
	fprintf(src,"   *this = that;\n}\n");
}

void PrintSimpleMeta(FILE *hdr, char *className,int exportMember)
{
	char *ptr="";   /* NOT DLL Exported, or ignored on Unix. */

	if (bVDAGlobalDLLExport != NULL && exportMember == 1)
		ptr = bVDAGlobalDLLExport;

	fprintf(hdr, "public:\n  %s const char* typeName() const\t{ return \"%s\"; }\n",
		ptr, className);
}

void PrintSimpleCheck(FILE *hdr, FILE* src, char *className,int exportMember)
{

	char *ptr="";   /* NOT DLL Exported, or ignored on Unix. */

	if (bVDAGlobalDLLExport != NULL && exportMember == 1)
		ptr = bVDAGlobalDLLExport;

	fprintf(hdr, "\t%s virtual int checkConstraints(ConstraintFailList* pConstraintFails) const;\n",
		ptr);
	fprintf(src, "int %s::checkConstraints(ConstraintFailList* pConstraintFails) const\n{\n",
		className);
	fprintf(src, "\treturn checkListConstraints(pConstraintFails);\n");
	fprintf(src, "}\n");
}

static void PrintHdrComment(FILE *hdr, Module *m)
{
	// time_t now = time (NULL);

	fprintf (hdr, "//   NOTE: this is a machine generated file--editing not recommended\n");
	fprintf (hdr, "//\n");
	fprintf (hdr, "// %s - class definitions for ASN.1 module %s\n", RemovePath(m->cxxHdrFileName), m->modId->name);
	fprintf (hdr, "//\n");
	// char szBuffer[512] = { 0 };
	// ctime_s(szBuffer, 512, &now);
	// fprintf (hdr, "//   This file was generated by snacc on %s", szBuffer);
	fprintf (hdr, "//   This file was generated by esnacc\n");
	fprintf (hdr, "//   esnacc by Jan Fellner - estos GmbH\n");
	fprintf (hdr, "\n");

} /* PrintHdrComment */

static void PrintSrcComment(FILE *src, Module *m)
{
	// time_t now = time (NULL);

	fprintf (src, "//   NOTE: this is a machine generated file--editing not recommended\n");
	fprintf (src, "//\n");
	fprintf (src, "// %s - class member functions for ASN.1 module %s\n", RemovePath(m->cxxSrcFileName), m->modId->name);
	fprintf (src, "//\n");
	// char szBuffer[512] = { 0 };
	// ctime_s(szBuffer, 512, &now);
	// fprintf (src, "//   This file was generated by snacc on %s", szBuffer);
	fprintf (src, "//   This file was generated by esnacc\n");
	fprintf (src, "//   esnacc by Jan Fellner - estos GmbH\n");
	fprintf (src, "\n");

} /* PrintSrcComment */


static void PrintSrcIncludes(FILE *src, ModuleList *mods, Module *m)
{
	if (m->cxxHdrFileName)
		fprintf(src, "#include \"%s\"\n", RemovePath(m->cxxHdrFileName));
} /* PrintSrcIncludes */


static int HasTypeDecl(TypeDef *td)
{
	switch (td->type->basicType->choiceId)
	{
	case BASICTYPE_COMPONENTSOF:
	case BASICTYPE_SELECTION:
	case BASICTYPE_UNKNOWN:
	case BASICTYPE_MACRODEF:
	case BASICTYPE_MACROTYPE:
		return 0; /* do nothing */

	default:
		if (IsNewType (td->type))
			return 1;
	}
	return 0;
}

static void PrintTypeDecl(FILE *f, TypeDef *td)
{
	switch (td->type->basicType->choiceId)
	{
	case BASICTYPE_COMPONENTSOF:
	case BASICTYPE_SELECTION:
	case BASICTYPE_UNKNOWN:
	case BASICTYPE_MACRODEF:
	case BASICTYPE_MACROTYPE:
		return; /* do nothing */

	default:
		if (IsNewType (td->type))
			fprintf (f, "class %s;\n", td->cxxTypeDefInfo->className);
	}

} /* PrintTypeDecl */


static void PrintCxxType(FILE *hdr,
						 ModuleList *mods,
						 Module *m,
						 CxxRules *r,
						 TypeDef *td,
						 Type *parent,
						 Type *t)
{
	char *pszNamespace=NULL;
	pszNamespace = LookupNamespace(t, mods);

	if (pszNamespace)
	{
		fprintf (hdr, "%s::%s       ", pszNamespace, t->cxxTypeRefInfo->className);
	}
	else
	{
		fprintf (hdr, "%s       ", t->cxxTypeRefInfo->className);
	}           // END IF BASICTYPE_IMPORTTYPEREF

	if (t->cxxTypeRefInfo->isPtr)
		fprintf (hdr, "*");
} /* PrintCxxType */



/*
*  Uses the Constructor that takes no args.
*  Assumes file f is positioned inside a class definition.
*  All Classes get this to support the ANY type.
*/
static void PrintCloneMethod(FILE *hdr,
							 FILE *src,
							 TypeDef *td)
{
	//    fprintf (hdr, "  AsnType		*Clone() const;\n\n", td->cxxTypeDefInfo->className);
	fprintf (hdr, "  AsnType		*Clone() const;\n\n");
	fprintf (src, "AsnType *%s::Clone() const\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  return new %s(*this);\n", td->cxxTypeDefInfo->className);
	fprintf (src, "}\n\n");

} /* PrintCloneMethod */


/*
* prints inline definition of constructors if this class is
* derived from a library class.
* assumes FILE *f is positioned in the derived class definition (.h)
*
* 12/92 MS - added overloaded "=" ops for string types.
*/
static void PrintDerivedConstructors(FILE *f,
									 CxxRules *r,
									 TypeDef *td)
{
	enum BasicTypeChoiceId typeId;
	char *derivedClassName;
	char *baseClassName;

	typeId = GetBuiltinType (td->type);
	derivedClassName = td->cxxTypeDefInfo->className;
	baseClassName = td->type->cxxTypeRefInfo->className;

	/* every class gets the no-arg constructor */
#if TCL
	if (printTclG && typeId == BASICTYPE_ENUMERATED)
	{
		fprintf (f, "#if TCL\n");
		fprintf (f, "			%s(): %s (_nmdescs[0].value) {}\n", derivedClassName, baseClassName);
		fprintf (f, "#else\n");
	}
	if (printTclG && typeId == BASICTYPE_ENUMERATED)
		fprintf (f, "#endif\n");
#endif /* TCL */

	switch (typeId)
	{
	case BASICTYPE_BOOLEAN:
		fprintf (f, "			%s(): %s() {}\n", derivedClassName, baseClassName);
		fprintf (f, "			%s (bool _b): %s (_b) {}\n", derivedClassName, baseClassName);
		break;

	case BASICTYPE_ENUMERATED:
	case BASICTYPE_INTEGER:
		fprintf (f, "			%s(): %s() {}\n", derivedClassName, baseClassName);
		fprintf (f, "			%s (int i): %s (i) {}\n", derivedClassName, baseClassName);
		break;

	case BASICTYPE_REAL:
		fprintf (f, "			%s(): %s() {}\n", derivedClassName, baseClassName);
		fprintf (f, "			%s (double d): %s (d) {}\n", derivedClassName, baseClassName);
		break;

	case BASICTYPE_OCTETSTRING:
	case BASICTYPE_OCTETCONTAINING:
		fprintf (f, "			%s(): %s() {}\n", derivedClassName, baseClassName);
		fprintf (f, "			%s (const char *str): %s (str) {}\n", derivedClassName, baseClassName);

		fprintf (f, "			%s (const char *str, const size_t len): %s (str, len) {}\n", derivedClassName, baseClassName);

		fprintf (f, "			%s (const %s &o): %s (o) {}\n", derivedClassName, derivedClassName, baseClassName);

		/* include overloading of = op. MS 12/92 */

		fprintf (f, "        %s		&operator = (const %s &o)	{ AsnOcts::operator=(o); return *this; }\n", derivedClassName, derivedClassName);
		fprintf (f, "        %s		&operator = (const char *str)	{ AsnOcts::operator=(str); return *this; }\n", derivedClassName);
		break;

	case BASICTYPE_BITSTRING:
		{
			const char *nblStr = "nblFlag = false;";
			int iNumNamedElements = 0;
			if (HasNamedElmts (td->type))
			{
				nblStr = "nblFlag = true;";
				iNumNamedElements = GetNumNamedElmts(td->type);
			}
			if (iNumNamedElements)
				fprintf(f, "\t%s() : %s(%d)\t\t\t{ %s }\n", derivedClassName,
				baseClassName, iNumNamedElements, nblStr);
			else
				fprintf(f, "\t%s() : %s()\t\t\t{ %s }\n", derivedClassName,
				baseClassName, nblStr);

			fprintf(f, "\t%s(size_t bits) : %s(bits)\t{ %s }\n",
				derivedClassName, baseClassName, nblStr);
			fprintf(f, "\t%s(const unsigned char* str, const size_t bitLen) :\n",
				derivedClassName);
			fprintf(f, "\t\t%s(str, bitLen)\t\t\t\t{ %s }\n", baseClassName,
				nblStr);
			//	Copy constructor not needed
			//				fprintf(f, "\t%s (const %s &o) : %s(o)\n",
			//					derivedClassName, baseClassName, baseClassName);
		}
		break;

	case BASICTYPE_OID:
		/* TBD: Fix this code -PIERCE

		fprintf (f, "			%s(): %s() {}\n", derivedClassName, baseClassName);
		fprintf (f, "			%s (const char *encOid, size_t len): %s (encOid, len) {}\n", derivedClassName, baseClassName);
		fprintf (f, "			%s (const %s &o): %s (o) {}\n", derivedClassName, baseClassName, baseClassName);
		fprintf (f, "			%s (unsigned long  a1, unsigned long  a2, long a3=-1, long a4=-1, long a5=-1, long a6=-1, long a7=-1, long a8=-1, long a9=-1, long a10=-1, long a11=-1): %s (a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) {}\n", baseClassName, derivedClassName, baseClassName);
		fprintf (f, "  %s		&operator = (const %s &o)	{ ReSet (o); return *this; }\n", derivedClassName, derivedClassName);
		*/
		printf("TBD: Attempt was made to generate code for a tagged OID outside of a SEQ or SET\n");
		break;
	default:
		break;
	}
} /* PrintDerivedConstructors */

#if DEPRECATED
static void PrintCxxEocEncoders(FILE *src,
								TypeDef *td,
								Type *t,
								char *bufVarName)
{
	TagList *tl;
	Tag *tag;
	int stoleChoiceTags;

	/* get all the tags on this type*/
	tl = (TagList*) GetTags (t, &stoleChoiceTags);
	FreeTags (tl);
} /* PrintCxxEocEncoders */
#endif

static int HasShortLen(Type *t)
{
	enum BasicTypeChoiceId typesType;
	/*
	* efficiency hack - use simple length (1 byte)
	* encoded for type (almost) guaranteed to have
	* encoded lengths of 0 <= len <= 127
	*/
	typesType = GetBuiltinType (t);
	/*RWC;8/9/01;REMOVED when INTEGER made AsnOcts;return typesType == BASICTYPE_BOOLEAN || typesType == BASICTYPE_INTEGER || typesType == BASICTYPE_NULL || typesType == BASICTYPE_REAL || typesType == BASICTYPE_ENUMERATED; */
	return typesType == BASICTYPE_BOOLEAN || typesType == BASICTYPE_NULL || typesType == BASICTYPE_REAL || typesType == BASICTYPE_ENUMERATED;
} /* HasShortLen */


/*
* prints length encoding code.  Primitives always use
* definite length and constructors get "ConsLen"
* which can be configured at compile to to be indefinite
* or definite.  Primitives can also be "short" (isShort is true)
* in which case a fast macro is used to write the length.
* Types for which isShort apply are: boolean, null and
* (almost always) integer and reals
*/
static void PrintCxxLenEncodingCode(FILE *f,
									int isCons,
									int isShort,
									char *lenVarName,
									char *bufVarName)
{
	if (isCons)
		fprintf (f, "    %s += BEncConsLen (%s, %s);\n", lenVarName, bufVarName, lenVarName);
	else
	{
		if (isShort)
		{
			fprintf (f, "    BEncDefLenTo127 (%s, %s);\n", bufVarName, lenVarName);
			fprintf (f, "    %s++;\n", lenVarName);
		}
		else
			fprintf (f, "    %s += BEncDefLen (%s, %s);\n", lenVarName, bufVarName, lenVarName);
	}
} /* PrintCxxLenEncodingCode */


/* prints last tag's encoding code first */
static void PrintCxxTagAndLenList(FILE *src,
								  Type *t,
								  TagList *tagList,
								  char *lenVarName,
								  char *bufVarName)
{
	char *classStr;
	char *formStr;
	Tag *tg;
	int tagLen;
	int isShort;

	if ((tagList == NULL) || LIST_EMPTY (tagList))
		return;

	/*
	* efficiency hack - use simple length (1 byte)
	* encoded for type (almost) guaranteed to have
	* encoded lengths of 0 <= len <= 127
	*/
	isShort = HasShortLen (t);

	/*
	* since encoding backward encode tags backwards
	*/
	FOR_EACH_LIST_ELMT_RVS (tg, tagList)
	{
		classStr = Class2ClassStr (tg->tclass);

		if (tg->form == CONS)
		{
			formStr = Form2FormStr (CONS);
			PrintCxxLenEncodingCode (src, TRUE, isShort, lenVarName, bufVarName);
		}
		else /* PRIM or ANY_FORM */
		{
			formStr = Form2FormStr (PRIM);
			PrintCxxLenEncodingCode (src, FALSE, isShort, lenVarName, bufVarName);
		}

		fprintf (src, "\n");

		//RWC;tagLen = TagByteLen (tg->code);

		if (tg->tclass == UNIV)
		{
			const char* ptr = DetermineCode(tg, &tagLen, 0);
			fprintf (src, "    %s += BEncTag%d (%s, %s, %s, %s);\n", lenVarName, tagLen, bufVarName, classStr, formStr, ptr);
		}                                                       //RWC;Code2UnivCodeStr (tg->code));
		else
		{
			const char* ptr = DetermineCode(tg, &tagLen, 1);
			fprintf (src, "    %s += BEncTag%d (%s, %s, %s, %s);\n", lenVarName, tagLen, bufVarName, classStr, formStr, ptr);
		}                                                       //RWC;tg->code);
	}
} /* PrintCxxTagAndLenList */

/*
*  Recursively walks through tags, printing lower lvl tags
*  first (since encoding is done backwards).
*
*/
static void PrintCxxTagAndLenEncodingCode(FILE *src,
										  TypeDef *td,
										  Type *t,
										  char *lenVarName,
										  char *bufVarName)
{
	TagList *tl;
	int stoleChoiceTags;

	/*
	* get all the tags on this type
	*/
	tl = (TagList*) GetTags (t, &stoleChoiceTags);

	/*
	* leave choice elmt tag enc to encoding routine
	*/
	if (!stoleChoiceTags)
		PrintCxxTagAndLenList (src, t, tl, lenVarName, bufVarName);

	FreeTags (tl);
} /* PrintCxxTagAndLenEncodingCode */


/*
* used to figure out local variables to declare
* for decoding tags/len pairs on type t
*/
static int CxxCountVariableLevels(Type *t)
{
	if (GetBuiltinType (t) == BASICTYPE_CHOICE)
		return CountTags (t) +1; /* since must decode 1 internal tag type */
	else
		return CountTags (t);
} /* CxxCountVariableLevels */


/*
* returns true if elmts curr following
*  onward are all optional ow. false
*/
static int RestAreTailOptional(NamedTypeList *e)
{
	NamedType *elmt;
	void *tmp;
	int retVal;

	if (e == NULL)
		return TRUE;

	tmp = (void*)CURR_LIST_NODE (e);
	retVal = TRUE;
	AsnListNext (e);
	FOR_REST_LIST_ELMT (elmt, e)
	{
		if ((!elmt->type->optional) && (elmt->type->defaultVal == NULL)&&(!elmt->type->extensionAddition))
		{
			retVal = FALSE;
			break;
		}
	}
	SET_CURR_LIST_NODE (e, tmp); /* reset list to orig loc */
	return retVal;
}

/*
* prints PrintROSEOnInvoke
*/
static void PrintROSEOnInvoke(FILE *hdr, int bEvents, Module* mod, ValueDef *vd)
{
	char* pszArgument = NULL;
	char* pszResult = NULL;
	char* pszError = NULL;

	if (GetROSEDetails(mod, vd, &pszArgument, &pszResult, &pszError, NULL, NULL, NULL, false))
	{
		if (pszResult && !bEvents)
		{
			//there is a result -> it is a Funktion
			//Header
			if (pszError)
			{
				fprintf(hdr, "\tvirtual SnaccInvokeResult OnInvoke_%s(%s* argument, %s* result, %s* error, SnaccInvokeContext* cxt) { return returnReject; }\n", vd->definedName,
					pszArgument, pszResult, pszError);
			}
			else
			{
				fprintf(hdr, "\tvirtual SnaccInvokeResult OnInvoke_%s(%s* argument, %s* result, SnaccInvokeContext* cxt) { return returnReject; }\n", vd->definedName,
					pszArgument, pszResult);
			}
		}
		else if (!pszResult && bEvents)
		{
			//there is no result -> it is an Event
			//Header
			fprintf(hdr, "\tvirtual void OnEvent_%s(%s* argument) { }\n", vd->definedName,
				pszArgument);
		}
	}
} /* PrintROSEInvoke */



/*
* prints PrintROSEInvoke
*/

/*
* prints PrintROSEOnInvokeswitchCase
*/
static void PrintROSEOnInvokeswitchCase(FILE *src, int bEvents, Module* mod, ValueDef *vd)
{
	char* pszArgument = NULL;
	char* pszResult = NULL;
	char* pszError = NULL;

	if (GetROSEDetails(mod, vd, &pszArgument, &pszResult, &pszError, NULL, NULL, NULL, false))
	{
		//there is an argument
		if (pszResult && !bEvents)
		{
			//there is a result -> it is a Funktion
			//Source here
			fprintf(src, "\t\tcase OPID_%s:\n", vd->definedName);
			fprintf(src, "\t\t{\n");
			fprintf(src, "\t\t\t%s argument;\n", pszArgument);
			fprintf(src, "\t\t\t%s result;\n", pszResult);
			if (pszError)
				fprintf(src, "\t\t\t%s error;\n", pszError);

			fprintf(src, "\t\t\tAsnLen len;\n");
			fprintf(src, "\t\t\tif (pinvoke->argument)\n");
			fprintf(src, "\t\t\t\tif (pinvoke->argument->anyBuf)\n");
			fprintf(src, "\t\t\t\t\targument.BDec(*pinvoke->argument->anyBuf, len);\n");
			fprintf(src, "\t\t\t\telse if (pinvoke->argument->jsonBuf)\n");
			fprintf(src, "\t\t\t\t\targument.JDec(*pinvoke->argument->jsonBuf);\n");

			fprintf(src, "\n");

			fprintf(src, "\t\t\tif(pBase->GetLogLevel(false))\n");
			fprintf(src, "\t\t\t\tpBase->PrintAsnType(false, &argument, pinvoke);\n");

			/* logging
			fprintf(src, "\t\t\tif(GetLogLevel(false))\n");
			fprintf(src, "\t\t\t{\n");
			fprintf(src, "\t\t\t\tstd::stringstream strOut;\n");
			fprintf(src, "\t\t\t\tstrOut << \"OnInvoke_%s\";\n", vd->definedName);
			fprintf(src, "\t\t\t\targument.Print(strOut);\n");
			fprintf(src, "\t\t\t\tPrintToLog(strOut.str());\n");
			fprintf(src, "\t\t\t}\n");
			*/

			/*
			fprintf(src, "\t\t\t\tcase 2:\n");
			fprintf(src, "\t\t\t\t{\n");
			fprintf(src, "\t\t\t\t\tstd::stringstream strOut;\n");
			fprintf(src, "\t\t\t\t\tstrOut << \"OnInvoke_%s\";\n", vd->definedName);
			fprintf(src, "\t\t\t\t\targument.PrintXML(strOut, \"%s\");\n", vd->definedName);
			fprintf(src, "\t\t\t\t\tPrintToLog(strOut.str());\n");
			fprintf(src, "\t\t\t\t} break;\n");
			*/

			fprintf(src, "\n");

			if (pszError)
				fprintf(src, "\t\t\tswitch (pInt->OnInvoke_%s(&argument, &result, &error, cxt))\n", vd->definedName);
			else
				fprintf(src, "\t\t\tswitch (pInt->OnInvoke_%s(&argument, &result, cxt))\n", vd->definedName);

			fprintf(src, "\t\t\t{\n");
			//InvokeResult::returnResult
			fprintf(src, "\t\t\tcase returnResult:\n");

			fprintf(src, "\t\t\t\tif(pBase->GetLogLevel(true))\n");
			fprintf(src, "\t\t\t\t\tpBase->PrintAsnType(true, &result, pinvoke);\n");

			fprintf(src, "\t\t\t\tlRoseResult = pBase->SendResult(pinvoke->invokeID, &result, pinvoke->sessionID ? pinvoke->sessionID->c_str() : 0);\n");
			fprintf(src, "\t\t\t\tbreak;\n");

			//InvokeResult::returnError
			fprintf(src, "\t\t\tcase returnError:\n");
			if (pszError)
			{
				fprintf(src, "\t\t\t\tif(pBase->GetLogLevel(true))\n");
				fprintf(src, "\t\t\t\t\tpBase->PrintAsnType(true, &error, pinvoke);\n");

				fprintf(src, "\t\t\t\tlRoseResult = pBase->SendError(pinvoke->invokeID, &error, pinvoke->sessionID ? pinvoke->sessionID->c_str() : 0);\n");
			}
			else
				fprintf(src, "\t\t\t\tlRoseResult = pBase->SendError(pinvoke->invokeID, NULL, pinvoke->sessionID ? pinvoke->sessionID->c_str() : 0);\n");

			fprintf(src, "\t\t\t\tbreak;\n");

			fprintf(src, "\t\t\tdefault:\n");
			fprintf(src, "\t\t\t\tlRoseResult = cxt->lRejectResult ? cxt->lRejectResult : ROSE_REJECT_FUNCTIONMISSING;\n");
			fprintf(src, "\t\t\t\tbreak;\n");

			fprintf(src, "\t\t\t}\n");
			fprintf(src, "\t\t}\n");
			fprintf(src, "\t\tbreak;\n");
		}
		else if (!pszResult && bEvents)
		{
			//there is no result -> it is an Event
			//Source here
			fprintf(src, "\t\tcase OPID_%s:\n", vd->definedName);
			fprintf(src, "\t\t{\n");
			fprintf(src, "\t\t\t%s argument;\n", pszArgument);
			fprintf(src, "\t\t\tif (pinvoke->argument)\n");
			fprintf(src, "\t\t\t\tif (pinvoke->argument->anyBuf)\n");
			fprintf(src, "\t\t\t\t{\n");
			fprintf(src, "\t\t\t\t\tAsnLen len;\n");
			fprintf(src, "\t\t\t\t\targument.BDec(*pinvoke->argument->anyBuf, len);\n");
			fprintf(src, "\t\t\t\t}\n");
			fprintf(src, "\t\t\t\telse if (pinvoke->argument->jsonBuf)\n");
			fprintf(src, "\t\t\t\t\targument.JDec(*pinvoke->argument->jsonBuf);\n");

			fprintf(src, "\t\t\tif(pBase->GetLogLevel(false))\n");
			fprintf(src, "\t\t\t\tpBase->PrintAsnType(false, &argument, pinvoke);\n");

			fprintf(src, "\n");

			fprintf(src, "\t\t\tpInt->OnEvent_%s(&argument);\n", vd->definedName);
			fprintf(src, "\t\t\tlRoseResult = ROSE_NOERROR;\n");
			fprintf(src, "\t\t}\n");
			fprintf(src, "\t\tbreak;\n");
		}
	}
} /* PrintROSEOnInvokeswitchCase */

// Print the forward Declaration for Arguments in Invokes and Events
static void PrintAllForwardDeclarations(FILE *hdr, Module *m)
{
	TypeDef *td;
	fprintf (hdr, "//------------------------------------------------------------------------------\n");
	fprintf (hdr, "// forward declarations:\n\n");

	//Alle klassen printen
	//walk through all Type Definitions
	FOR_EACH_LIST_ELMT (td, m->typeDefs)
	{
		if (IsNewType (td->type)) {
			if (HasTypeDecl(td)) {
				//class
				PrintTypeDecl(hdr, td);
			}
		}
	}
	//Alle typedefs printen
	FOR_EACH_LIST_ELMT (td, m->typeDefs)
	{
		if (!IsNewType (td->type)) {

			if (IsPrimitiveByDefOrRef(td->type) && gAlternateNamespaceString != 0)
			{
				fprintf(hdr, "class SNACC::%s;\n", td->type->cxxTypeRefInfo->className);
			}
			else
			{
				fprintf(hdr, "class %s;\n", td->type->cxxTypeRefInfo->className);
			}

			//typedef

			PrintTypeDefDefault(hdr, NULL, td);
		}
	}

	fprintf (hdr, "\n");
}

void PrintLongComment(FILE *hdr, const char* szLinePrefix, const char* szComment)
{
	if (!*szComment)
		return;

	fprintf(hdr, "%s", szLinePrefix);
	fprintf(hdr, "%s", "/* ");

	const char* sz = szComment;
	while (*sz)
	{
		if (strncmp(sz, "\\n", 2) == 0)
		{
			fprintf(hdr, "%s", "\n");
			fprintf(hdr, "%s", szLinePrefix);
			fprintf(hdr, "%s", "   ");
			sz++;
		}
		else
		{
			fprintf(hdr, "%c", *sz);
		}
		sz++;
	}

	fprintf(hdr, "%s", " */\n");
}

/*
* prints PrintROSEInvoke
*/
static void PrintROSEInvoke(FILE *hdr, FILE *src, Module *m, int bEvents, ValueDef *vd)
{
	char* pszArgument = NULL;
	char* pszResult = NULL;
	char* pszError = NULL;

	if (GetROSEDetails(m, vd, &pszArgument, &pszResult, &pszError, NULL, NULL, NULL, false))
	{
		if (pszResult && !bEvents)
		{
			asnoperationcomment com;
			if (GetOperationComment_ASCII(m->moduleName, vd->definedName, &com))
			{
				fprintf(hdr, "\t//@method %s %s%s%s\n", vd->definedName, com.iPrivate ? "private " : "", com.lDeprecated ? "deprecated " : "", com.szShort);
				PrintLongComment(hdr, "\t", com.szLong);
			}
			//there is a result -> it is a Funktion
			//Are there errors inside?
			if (pszError)
			{
				//Header
				fprintf(hdr, "\tlong Invoke_%s(%s* argument, %s* result, %s* error, int iTimeout = -1, SnaccInvokeContext* cxt = 0);\n", vd->definedName,
					pszArgument, pszResult, pszError);

				//Source
				fprintf(src, "long %s::Invoke_%s(%s* argument, %s* result, %s* error, int iTimeout, SnaccInvokeContext* cxt)\n", m->ROSEClassName, vd->definedName,
					pszArgument, pszResult, pszError);
			}
			else
			{
				//no special errors
				//Header
				fprintf(hdr, "\tlong Invoke_%s(%s* argument, %s* result, int iTimeout = -1, SnaccInvokeContext* cxt = 0);\n", vd->definedName,
					pszArgument, pszResult);

				//Source
				fprintf(src, "long %s::Invoke_%s(%s* argument, %s* result, int iTimeout, SnaccInvokeContext* cxt)\n", m->ROSEClassName, vd->definedName,
					pszArgument, pszResult);

			}

			fprintf(src, "{\n");

			fprintf(src, "\tlong lRoseResult = ROSE_NOERROR;\n");

			fprintf(src, "\tROSEInvoke InvokeMsg;\n");
			fprintf(src, "\tROSEResult* pResultMsg = NULL;\n");
			fprintf(src, "\tROSEError* pErrorMsg = NULL;\n");

			fprintf(src, "\n");

			fprintf(src, "\tInvokeMsg.invokeID = m_pSB->GetNextInvokeID();\n");
			fprintf(src, "\tInvokeMsg.operationID = OPID_%s;\n", vd->definedName);
			fprintf(src, "\tInvokeMsg.argument = new AsnAny;\n");
			fprintf(src, "\tInvokeMsg.argument->value = argument;\n");

			fprintf(src, "\tif(m_pSB->GetLogLevel(true))\n");
			fprintf(src, "\t\tm_pSB->PrintAsnType(true, argument, &InvokeMsg);\n");

			fprintf(src, "\tlRoseResult = m_pSB->SendInvoke(&InvokeMsg, &pResultMsg, &pErrorMsg, iTimeout, cxt);\n");

			fprintf(src, "\t//prevent autodelete of argument\n");
			fprintf(src, "\tInvokeMsg.argument->value = NULL;\n");

			fprintf(src, "\t//decode result\n");
			if (pszError)
				fprintf(src, "\tlRoseResult = m_pSB->HandleInvokeResult(lRoseResult, &InvokeMsg, pResultMsg, pErrorMsg, result, error, cxt);\n");
			else
				fprintf(src, "\tlRoseResult = m_pSB->HandleInvokeResult(lRoseResult, &InvokeMsg, pResultMsg, pErrorMsg, result, NULL, cxt);\n");


			fprintf(src, "\tif (pResultMsg)\n");
			fprintf(src, "\t\tdelete pResultMsg;\n");

			fprintf(src, "\tif (pErrorMsg)\n");
			fprintf(src, "\t\tdelete pErrorMsg;\n");

			fprintf(src, "\treturn lRoseResult;\n");
			fprintf(src, "}\n");
			fprintf(src, "\n");
		}
		else if (!pszResult && bEvents)
		{
			//there is no result -> it is an Event
			asnoperationcomment com;
			if (GetOperationComment_ASCII(m->moduleName, vd->definedName, &com))
			{
				fprintf(hdr, "\t//@method %s %s%s%s\n", vd->definedName, com.iPrivate ? "private " : "", com.lDeprecated ? "deprecated " : "", com.szShort);
				PrintLongComment(hdr, "\t", com.szLong);
			}

			//Header
			fprintf(hdr, "\tlong Event_%s(%s* argument);\n", vd->definedName,
				pszArgument);

			//Source
			fprintf(src, "long %s::Event_%s(%s* argument)\n", m->ROSEClassName, vd->definedName,
				pszArgument);
			fprintf(src, "{\n");

			fprintf(src, "\tlong lRoseResult = ROSE_NOERROR;\n");

			fprintf(src, "\tROSEInvoke InvokeMsg;\n");
			fprintf(src, "\n");
			//InvokeID for Events is always 99999
			fprintf(src, "\tInvokeMsg.invokeID = 99999;\n");
			fprintf(src, "\tInvokeMsg.operationID = OPID_%s;\n", vd->definedName);
			fprintf(src, "\tInvokeMsg.argument = new AsnAny;\n");
			fprintf(src, "\tInvokeMsg.argument->value = argument;\n");

			fprintf(src, "\tif(m_pSB->GetLogLevel(true))\n");
			fprintf(src, "\t\tm_pSB->PrintAsnType(true, argument, &InvokeMsg);\n");

			fprintf(src, "\tlRoseResult = m_pSB->SendEvent(&InvokeMsg);\n");

			fprintf(src, "\t//prevent autodelete of argument\n");
			fprintf(src, "\tInvokeMsg.argument->value = NULL;\n");

			fprintf(src, "\treturn lRoseResult;\n");
			fprintf(src, "}\n");
			fprintf(src, "\n");
		}
	}
} /* PrintROSEInvoke */





/*
* prints typedef or new class given an ASN.1  type def of a primitive type
* or typeref.  Uses inheritance to cover re-tagging and named elmts.
*/
static void PrintCxxSimpleDef(FILE *hdr, FILE *src, Module *m, CxxRules *r, TypeDef *td)
{
	Tag *tag;
	TagList *tags;
	char *formStr;
	char *classStr;
	int tagLen;
	int i;
	CNamedElmt *n;
	int stoleChoiceTags;
	int elmtLevel;
	enum BasicTypeChoiceId typeId;

	fprintf (hdr, "/* ");
	SpecialPrintType (hdr, td, td->type);
	fprintf (hdr, " */\n");

	/* check if has been re-tagged
	*   eg Foo ::= [APPLICATION 2] IMPLICIT REAL
	* or if it has named elmts in which case a new class must
	* be defined
	*  eg Foo ::= INTEGER { one (1), two (2), three (3) }
	*/

	if (IsNewType (td->type))
	{
		int	hasNamedElmts;

		char *ptr="";   /* NOT DLL Exported, or ignored on Unix. */
		if (bVDAGlobalDLLExport)
			ptr = bVDAGlobalDLLExport;

		fprintf (hdr, "class %s %s: public %s\n", ptr, td->cxxTypeDefInfo->className, td->type->cxxTypeRefInfo->className);
		fprintf (hdr, "{\n");
		fprintf (hdr, "public:\n");

		/*
		* must explicitly call constructors for base class
		*/
		PrintDerivedConstructors (hdr, r, td);

		/* do named elmts enum if any */
		/* for types with named elements, inherit from the base
		* class and define and enum eg:
		* Foo ::= INTEGER { one (1), two (2), five (5) }
		*  ->
		* class Foo: public AsnInt
		* {
		* public:
		*		Foo(): AsnInt() {}
		*		Foo (int val): AsnInt (int val) {}
		*    enum { one = 1, two = 2, five = 5 };
		* };
		* or
		* Foo2 ::= [APPLICATION 2] INTEGER
		* -->
		* class Foo: public AsnInt
		* {
		* public:
		*		Foo(): AsnInt() {}
		*		Foo (int val): AsnInt (int val) {}
		*     AsnLen	BEnc { ....... } <-- holds new tag enc/dec
		*     void	BDec { ....... }   <--/
		*     int	BEncPdu { ....... }
		*     int	BDecPdu { ....... }
		* };
		* (must 'inherit' constructors explicitly)
		*/

		if ((hasNamedElmts = HasNamedElmts (td->type)) != 0)
		{
			int count = 0;

			fprintf (hdr, "\n\tenum %senum\n", td->cxxTypeDefInfo->className);
			fprintf (hdr, "\t{\n");
			FOR_EACH_LIST_ELMT (n, td->type->cxxTypeRefInfo->namedElmts)
			{
				fprintf (hdr, "\t\t%s = %d", n->name, n->value);
				if (n != (CNamedElmt *)LAST_LIST_ELMT (td->type->cxxTypeRefInfo->namedElmts))
					fprintf (hdr, ",\n");
				else
					fprintf (hdr, "\n");

				count++;
			}
			fprintf (hdr, "\t};\n");


			if(td->type->basicType->choiceId == BASICTYPE_BITSTRING)
			{
				fprintf (hdr, "\tlong IndexFromNamedBit(%senum namedBit) const {\n", td->cxxTypeDefInfo->className);
				fprintf (hdr, "\t\tlong enumList[] = {");
				FOR_EACH_LIST_ELMT (n, td->type->cxxTypeRefInfo->namedElmts)
				{
					fprintf (hdr, "%d", n->value);
					if (n != (CNamedElmt *)LAST_LIST_ELMT (td->type->cxxTypeRefInfo->namedElmts))
						fprintf (hdr, ", ");
				}
				fprintf (hdr, " };\n");

				fprintf (hdr, "\t\tfor(int i = 0; i < sizeof(enumList)/sizeof(enumList[0]); i++) { if(enumList[i] == namedBit) return i; }\n");
				fprintf (hdr, "\t\treturn -1;\n");
				fprintf (hdr, "\t}\n");

				fprintf (hdr, "\tvoid SetNamedBit(%senum namedBit) {\n", td->cxxTypeDefInfo->className);
				fprintf (hdr, "\t\tlong lBit = IndexFromNamedBit(namedBit);\n");
				fprintf (hdr, "\t\tif (lBit >= 0) SetBit(lBit);\n");
				fprintf (hdr, "\t}\n");
				fprintf (hdr, "\tvoid ClrNamedBit(%senum namedBit) {\n", td->cxxTypeDefInfo->className);
				fprintf (hdr, "\t\tlong lBit = IndexFromNamedBit(namedBit);\n");
				fprintf (hdr, "\t\tif (lBit >= 0) ClrBit(lBit);\n");
				fprintf (hdr, "\t}\n");
				fprintf (hdr, "\tbool GetNamedBit(%senum namedBit) const {\n", td->cxxTypeDefInfo->className);
				fprintf (hdr, "\t\tlong lBit = IndexFromNamedBit(namedBit);\n");
				fprintf (hdr, "\t\tif (lBit >= 0) return GetBit(lBit);\n");
				fprintf (hdr, "\t\treturn false;\n");
				fprintf (hdr, "\t}\n");
			}
			else if (td->type->basicType->choiceId == BASICTYPE_ENUMERATED)
			{
				if (genPERCode)
				{
					fprintf(hdr, "\tAsnLen PEnc(AsnBufBits &_b)const;\n");

					fprintf(src, "\tAsnLen %s::PEnc(AsnBufBits &_b)const{\n", td->cxxTypeDefInfo->className);
					fprintf(src, "\t\tlong enumList[] = {");
					FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
					{
						fprintf(src, "%d", n->value);
						if (n != (CNamedElmt *)LAST_LIST_ELMT(td->type->cxxTypeRefInfo->namedElmts))
							fprintf(src, ", ");
					}
					fprintf(src, " };\n");
					fprintf(src, "\t\tAsnInt index = IndexedVal(enumList, %d);\n", count);
					fprintf(src, "\t\tAsnLen len = index.PEncFullyConstrained(_b, 0, %d);\n", count - 1);
					fprintf(src, "\t\treturn len;}\n");

					fprintf(hdr, "\tvoid PDec(AsnBufBits &_b, AsnLen &bitsDecoded);\n");

					fprintf(src, "\tvoid %s::PDec(AsnBufBits &_b, AsnLen &bitsDecoded){\n", td->cxxTypeDefInfo->className);
					fprintf(src, "\t\tlong enumList[] = {");
					FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
					{
						fprintf(src, "%d", n->value);
						if (n != (CNamedElmt *)LAST_LIST_ELMT(td->type->cxxTypeRefInfo->namedElmts))
							fprintf(src, ", ");
					}
					fprintf(src, " };\n");
					fprintf(src, "\t\tAsnInt index;\n");
					fprintf(src, "\t\tindex.PDecFullyConstrained(_b, 0, %d, bitsDecoded);\n", count - 1);
					fprintf(src, "\t\tSetIndex(enumList, %d, index);}\n", count);
				}
			}
		}

#if META
		if (printMetaG)
		{
			PrintCxxSimpleDefMeta_1(hdr, src, td, hasNamedElmts, n, m);
		}
#endif /* META */

		/*
		* Re-do BerEncode, BerDeocode, BerDecodePdu and BerDecodePdu
		* if this type has been re-tagged
		*/
		if ((IsDefinedByLibraryType (td->type) && !HasDefaultTag (td->type))
			|| (IsTypeRef (td->type) && ((td->type->tags != NULL)
			&& !LIST_EMPTY (td->type->tags))))
		{
			/* only BerEn/Decode BerEn/DecodePdu need to be re-done if tags are different */

			/* print clone routine for ANY mgmt */
			PrintCloneMethod (hdr, src, td);

			tags = GetTags (td->type, &stoleChoiceTags);
			typeId = GetBuiltinType (td->type);

			/* do BerEncode function */
			if (printEncodersG)
			{
				fprintf (hdr, "  %s		B%s (%s &_b) const;\n", lenTypeNameG, r->encodeBaseName, bufTypeNameG);
				fprintf (src, "%s %s::B%s (%s &_b) const\n", lenTypeNameG, td->cxxTypeDefInfo->className, r->encodeBaseName, bufTypeNameG);
				fprintf (src, "{\n");
				fprintf (src, "    %s l=0;\n", lenTypeNameG);
				fprintf (src, "    l = BEncContent (_b);\n");

				/* encode each tag/len pair if any */
				if (!stoleChoiceTags)
				{
					FOR_EACH_LIST_ELMT_RVS (tag, tags)
					{
						classStr = Class2ClassStr (tag->tclass);

						if (tag->form == ANY_FORM)
						{
							formStr = Form2FormStr (PRIM);
							PrintCxxLenEncodingCode (src, FALSE, HasShortLen (td->type), "l", "_b");
						}
						else
						{
							formStr = Form2FormStr (tag->form);
							PrintCxxLenEncodingCode (src, TRUE, HasShortLen (td->type), "l", "_b");
						}

						fprintf (src, "\n");
						//RWC;tagLen = TagByteLen (tag->code);

						if (tag->tclass == UNIV)
						{
							const char* ptr2 = DetermineCode(tag, &tagLen, 0);
							fprintf (src, "    l += BEncTag%d (_b, %s, %s, %s);\n", tagLen, classStr, formStr, ptr2);//RWC;Code2UnivCodeStr (tag->code));
						}
						else
						{
							const char* ptr2 = DetermineCode(tag, &tagLen, 1);
							fprintf (src, "    l += BEncTag%d (_b, %s, %s, %s);\n", tagLen, classStr, formStr, ptr2);//RWC;tag->code);
						}
					}
				}
				fprintf (src, "    return l;\n");
				fprintf (src, "}\n\n");
			}
			/* end of BEnc function */

			/* Do BDec function */
			if (printDecodersG)
			{
				fprintf (hdr, "  void			B%s (const %s &_b, %s &bytesDecoded);\n", r->decodeBaseName, bufTypeNameG, lenTypeNameG);// envTypeNameG);
				fprintf (src, "void %s::B%s (const %s &_b, %s &bytesDecoded)\n", td->cxxTypeDefInfo->className, r->decodeBaseName, bufTypeNameG, lenTypeNameG);//, envTypeNameG);
				fprintf (src, "{\n");
				fprintf (src, "    FUNC(\"%s::B%s\");\n", td->cxxTypeDefInfo->className, r->decodeBaseName);
				fprintf (src, "    %s tag;\n", tagTypeNameG);

				/* PL: removed to avoid unused variable warning
				fprintf (src, "    AsnBufLoc readLoc = _b.GetReadLoc();\n");
				*/

				/* print extra locals for redundant lengths */
				for (i = 1; (tags != NULL) && (i <= LIST_COUNT (tags)); i++)
					fprintf (src, "    %s elmtLen%d;\n", lenTypeNameG, i);
				if (typeId == BASICTYPE_CHOICE)
					fprintf (src, "    %s elmtLen%d;\n", lenTypeNameG, i++);
				fprintf (src, "\n");

				/*  decode tag/length pair (s) */
				elmtLevel = 0;
				if (!stoleChoiceTags)
				{
					FOR_EACH_LIST_ELMT (tag, tags)
					{
						classStr = Class2ClassStr (tag->tclass);


						if (tag->form == ANY_FORM)
							formStr = Form2FormStr (PRIM);
						else
							formStr = Form2FormStr (tag->form);

						fprintf (src, "    if (((tag = BDecTag (_b, bytesDecoded)) != ");

						if (tag->tclass == UNIV)
						{
							fprintf (src, "MAKE_TAG_ID (%s, %s, %s))", classStr, formStr, DetermineCode(tag, NULL, 0));//RWC;Code2UnivCodeStr (tag->code));
							if (tag->form == ANY_FORM)
								fprintf (src, "\n        && (tag != MAKE_TAG_ID (%s, %s, %s)))\n", classStr, Form2FormStr (CONS), DetermineCode(tag, NULL, 0));//RWC;Code2UnivCodeStr (tag->code));
							else
								fprintf (src, ")\n");
						}
						else
						{
							fprintf (src, "MAKE_TAG_ID (%s, %s, %s))", classStr, formStr, DetermineCode(tag, NULL, 1));//RWC;tag->code);
							if (tag->form == ANY_FORM)
								fprintf (src, "\n        && (tag != MAKE_TAG_ID (%s, %s, %s)))\n", classStr, Form2FormStr (CONS), DetermineCode(tag, NULL, 1));//RWC;tag->code);
							else
								fprintf (src, ")\n");
						}

						fprintf (src, "    {\n");
						fprintf (src, "        throw InvalidTagException(typeName(), tag, STACK_ENTRY);\n");
						fprintf (src, "    }\n");
						fprintf (src, "    elmtLen%d = BDecLen (_b, bytesDecoded);\n", ++elmtLevel);
					}
				}

				/* decode first tag from CHOICE's content */
				if (typeId == BASICTYPE_CHOICE)
				{
					fprintf (src, "    tag = BDecTag (_b, bytesDecoded);\n");
					fprintf (src, "    elmtLen%d = BDecLen (_b, bytesDecoded);\n", ++elmtLevel);
				}

				fprintf (src, "    B%s (_b, tag, elmtLen%d, bytesDecoded);\n", r->decodeContentBaseName, i-1);

				/* grab any EOCs that match redundant, indef lengths */
				for (i = elmtLevel-1; i > 0; i--)
				{
					fprintf (src, "    if (elmtLen%d == INDEFINITE_LEN)\n", i);
					fprintf (src, "        BDecEoc (_b, bytesDecoded);\n");
				}

				fprintf (src, "}\n\n");
			}
			/* end of BDec function */

			FreeTags (tags);
		}
		/* close class def */
		fprintf (hdr, "};\n\n\n");

	}
	else  /* isomorphic with referenced type, so just to a typedef */
	{

#if META
		if (printMetaG)
		{
			PrintCxxSimpleDefMeta_2(hdr, src, td, hasNamedElmts, n, m, r);
		}
#endif /* META */

		/* JKG 7/31/03 */
		/* The following code enclosed in this if/else statement */
		/* is constructed for constraint handling capability     */
		/* for primitives found outside of                       */
		/* sequences or sets                                     */

		if (td->type->subtypes != NULL)
		{
			switch (td->type->subtypes->choiceId)
			{
			case SUBTYPE_AND:
			case SUBTYPE_OR:
			case SUBTYPE_SINGLE:
				{
					struct NamedType temp;
					NamedType* tmp;
					tmp=&temp;
					tmp->type=td->type;
					tmp->type->cxxTypeRefInfo->fieldName=td->definedName;
					tmp->fieldName=td->definedName;

					if (!PrintCxxMultiConstraintOrHandler(hdr, src, NULL, tmp, 0))
						PrintTypeDefDefault(hdr, src, td);
					break;
				}

			default:
				{
					PrintTypeDefDefault(hdr, src, td);
					break;
				}
			}
		}
		else
		{
			PrintTypeDefDefault(hdr, src, td);
		}

#if META
		if (printMetaG)
			fprintf (hdr, "#endif // META\n\n");
#endif /* META */
	}
} /* PrintCxxSimpleDef */

void PrintChoiceDefCodeBerEncodeContent(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *choice)
{
	NamedType *e;
	CxxTRI *cxxtri;
	char *varName;
	enum BasicTypeChoiceId tmpTypeId;
	NamedType *defByNamedType;
	fprintf (hdr, "  %s		B%s (%s &_b) const;\n", lenTypeNameG, r->encodeContentBaseName, bufTypeNameG);

	fprintf (src, "%s\n", lenTypeNameG);
	fprintf (src, "%s::B%s (%s &_b) const\n", td->cxxTypeDefInfo->className, r->encodeContentBaseName, bufTypeNameG);
	fprintf (src, "{\n");
	fprintf (src, "    FUNC(\"%s::B%s (%s &_b)\");\n", td->cxxTypeDefInfo->className, r->encodeContentBaseName, bufTypeNameG);

	/* print local vars */
	fprintf (src, "  %s l=0;\n", lenTypeNameG);

	fprintf (src, "  switch (%s)\n", r->choiceIdFieldName);
	fprintf (src, "  {\n");
	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		cxxtri =  e->type->cxxTypeRefInfo;
		fprintf (src, "    case %s:\n", cxxtri->choiceIdSymbol);

		varName = cxxtri->fieldName;

		/* eSNACC 1.5 does not encode indefinite length
		*
		* PrintCxxEocEncoders (src, td, e->type, "_b");
		*/

		/* encode content */
		tmpTypeId = GetBuiltinType (e->type);
		if (tmpTypeId == BASICTYPE_ANYDEFINEDBY)
		{
			defByNamedType = e->type->basicType->a.anyDefinedBy->link;
			PrintCxxSetTypeByCode(defByNamedType, cxxtri, src);
		}
		else if (tmpTypeId == BASICTYPE_ANY)
		{
			fprintf (src, "    l = %s", varName);
			if (cxxtri->isPtr)
				fprintf (src, "->");
			else
				fprintf (src, ".");

			fprintf (src, "B%s (_b);\n", r->encodeBaseName);
		}
		else if (tmpTypeId == BASICTYPE_BITCONTAINING)
		{
			PrintCxxEncodeContaining(e->type, r, src);
		}
		else if ( tmpTypeId == BASICTYPE_EXTENSION )
		{
			fprintf (src, "      l = %s", varName);
			if (cxxtri->isPtr)
				fprintf (src, "->");
			else
				fprintf (src, ".");

			fprintf (src, "B%s (_b);\n", r->encodeBaseName);
		}
		else
		{
			fprintf (src, "      l = %s", varName);
			if (cxxtri->isPtr)
				fprintf (src, "->");
			else
				fprintf (src, ".");

			fprintf (src, "B%s (_b);\n", r->encodeContentBaseName);
		}

		/* encode tag (s) & len (s) */
		PrintCxxTagAndLenEncodingCode (src, td, e->type, "l", "_b");

		fprintf (src, "      break;\n\n");
	}
	fprintf (src, "      default:\n");
	fprintf (src, "         throw EXCEPT(\"Choice is empty\", ENCODE_ERROR);\n");
	fprintf (src, "  } // end switch\n");

	fprintf (src, "  return l;\n");
	fprintf (src, "} // %s::B%s\n\n\n", td->cxxTypeDefInfo->className, r->encodeContentBaseName);
}

void PrintChoiceDefCodeBerDecodeContent(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *choice)
{
	NamedType *e;
	char *codeStr;
	char *classStr;
	char *formStr;
	int i;
	Tag *tag;
	char *varName;
	CxxTRI *cxxtri;
	int elmtLevel=0;
	int varCount, tmpVarCount;
	enum BasicTypeChoiceId tmpTypeId;
	NamedType *defByNamedType;
	//char *ptr="";   /* NOT DLL Exported, or ignored on Unix. */
	int extensionsExist = FALSE;
	TagList *tags;
	int stoleChoiceTags;

	fprintf (hdr, "  void			B%s (const %s &_b, %s tag, %s elmtLen, %s &bytesDecoded /*, s env*/);\n", r->decodeContentBaseName, bufTypeNameG, tagTypeNameG, lenTypeNameG, lenTypeNameG);//, envTypeNameG);
	fprintf (src, "void %s::B%s (const %s &_b, %s tag, %s elmtLen0, %s &bytesDecoded /*, s env*/)\n", td->cxxTypeDefInfo->className, r->decodeContentBaseName, bufTypeNameG, tagTypeNameG, lenTypeNameG, lenTypeNameG);//, envTypeNameG);
	fprintf (src, "{\n");
	fprintf (src, "   FUNC(\"%s::B%s()\");\n",td->cxxTypeDefInfo->className, r->decodeContentBaseName);
	fprintf (src, "   Clear();\n");
	//fprintf (src, "   Init();\n");
	/* print local vars */
	/* count max number of extra length var nec
	* by counting tag/len pairs on components of the CHOICE
	*/

	varCount = 0;
	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		tmpVarCount = CxxCountVariableLevels (e->type);
		if (tmpVarCount > varCount)
			varCount = tmpVarCount;
	}
	/* write extra length vars - remember choice content
	* decoders are passed the 'key' tag so need one less
	* than max var count.
	*/
	for (i = 1; i < varCount; i++)
		fprintf (src, "  %s elmtLen%d = 0;\n", lenTypeNameG, i);

	/* switch on given tag - choices always have the key tag decoded */
	fprintf (src, "  switch (tag)\n");
	fprintf (src, "  {\n");
	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		if( e->type->basicType->choiceId == BASICTYPE_EXTENSION )
		{
			extensionsExist = TRUE;
		}
		else
		{

			cxxtri =  e->type->cxxTypeRefInfo;
			tags = GetTags (e->type, &stoleChoiceTags);

			if (LIST_EMPTY (tags))
			{
				fprintf (src, "    // ANY Type?\n");
				fprintf (src, "    case MAKE_TAG_ID (?, ?, ?):\n");
			}
			else
			{
				tag = (Tag*)FIRST_LIST_ELMT (tags);
				classStr = Class2ClassStr (tag->tclass);
				formStr = Form2FormStr (tag->form);

				if (tag->tclass == UNIV)
				{
					codeStr = DetermineCode(tag, NULL, 0);//RWC;Code2UnivCodeStr (tag->code);
				}
				else
				{
					codeStr = DetermineCode(tag, NULL, 1);
				}

				if (tag->form == ANY_FORM)
				{
					fprintf (src, "    case MAKE_TAG_ID (%s, %s, %s):\n", classStr, Form2FormStr (PRIM), codeStr);
					fprintf (src, "    case MAKE_TAG_ID (%s, %s, %s):\n", classStr, Form2FormStr (CONS), codeStr);
				}
				else
				{
					fprintf (src, "    case MAKE_TAG_ID (%s, %s, %s):\n", classStr, formStr, codeStr);
				}

				/* now decode extra tags/length pairs */
				AsnListFirst (tags);
				AsnListNext (tags);
				elmtLevel = 0;
				if (stoleChoiceTags)
				{
					FOR_REST_LIST_ELMT (tag, tags)
					{
						classStr = Class2ClassStr (tag->tclass);

						formStr = Form2FormStr (tag->form);

						if (tag->tclass == UNIV)
							codeStr = DetermineCode(tag, NULL, 0);//RWC;Code2UnivCodeStr (tag->code);
						else
							codeStr = DetermineCode(tag, NULL, 1);

						if (tag->form == ANY_FORM)
						{
							fprintf (src, "    case MAKE_TAG_ID (%s, %s, %s):\n", classStr, Form2FormStr (PRIM), codeStr);
							fprintf (src, "    case MAKE_TAG_ID (%s, %s, %s):\n", classStr, Form2FormStr (CONS), codeStr);
						}
						else
						{
							fprintf (src, "    case MAKE_TAG_ID (%s, %s, %s):\n", classStr, formStr, codeStr);
						}
					}
				}
				else /* didn't steal nested choice's tags */
				{
					FOR_REST_LIST_ELMT (tag, tags)
					{
						classStr = Class2ClassStr (tag->tclass);
						codeStr = DetermineCode(tag, NULL, 0);//RWC;Code2UnivCodeStr (tag->code);
						formStr = Form2FormStr (tag->form);

						fprintf (src, "      tag = BDecTag (_b, bytesDecoded);\n");
						if (tag->form == ANY_FORM)
						{
							if (tag->tclass == UNIV)
							{
								fprintf (src, "      if ((tag != MAKE_TAG_ID (%s, %s, %s))\n",  classStr, Form2FormStr (PRIM), codeStr);
								fprintf (src, "          && (tag != MAKE_TAG_ID (%s, %s, %s)))\n", classStr, Form2FormStr (CONS), codeStr);
							}
							else
							{
								fprintf (src, "      if ((tag != MAKE_TAG_ID (%s, %s, %s))\n", classStr, Form2FormStr (PRIM), DetermineCode(tag, NULL, 1));//RWC;tag->code);
								fprintf (src, "          && (tag != MAKE_TAG_ID (%s, %s, %s)))\n", classStr, Form2FormStr (CONS), DetermineCode(tag, NULL, 1));//RWC;tag->code);
							}
						}
						else
						{
							if (tag->tclass == UNIV)
								fprintf (src, "      if (tag != MAKE_TAG_ID (%s, %s, %s))\n", classStr, formStr, codeStr);
							else
								fprintf (src, "      if (tag != MAKE_TAG_ID (%s, %s, %s))\n", classStr, formStr, DetermineCode(tag, NULL, 1));//RWC;tag->code);
						}

						fprintf (src, "      {\n");

						fprintf (src, "        throw InvalidTagException(typeName(), tag, STACK_ENTRY);\n");
						fprintf (src, "      }\n\n");

						fprintf (src, "      elmtLen%d = BDecLen (_b, bytesDecoded);\n", ++elmtLevel);
					}
				}
			}
			/*
			* if the choices element is another choice &&
			* we didn't steal its tags then we must grab
			* the key tag out of the contained CHOICE
			*/
			if (!stoleChoiceTags && (GetBuiltinType (e->type) == BASICTYPE_CHOICE))
			{
				fprintf (src, "      tag = BDecTag (_b, bytesDecoded);\n");
				fprintf (src, "      elmtLen%d = BDecLen (_b, bytesDecoded);\n", ++elmtLevel);
			}

			varName = cxxtri->fieldName;
			/* set choice id for to this elment */
			fprintf (src, "      %s = %s;\n", r->choiceIdFieldName, cxxtri->choiceIdSymbol);

			/* alloc elmt if nec */
			if (cxxtri->isPtr)
			{
				fprintf (src, "        %s = new %s;\n", varName, cxxtri->className);
			}
			/* decode content */
			tmpTypeId = GetBuiltinType (e->type);
			if (tmpTypeId == BASICTYPE_ANYDEFINEDBY)
			{
				/*
				* must check for another EOC for ANYs
				* since the any decode routines decode
				* their own first tag/len pair
				*/
				elmtLevel++;
				defByNamedType = e->type->basicType->a.anyDefinedBy->link;
				PrintCxxSetTypeByCode(defByNamedType, cxxtri, src);

				fprintf (src, "      %s", varName);
				if (cxxtri->isPtr)
					fprintf (src, "->");
				else
					fprintf (src, ".");
				fprintf (src, "B%s (_b, bytesDecoded);\n",  r->decodeBaseName);
			}
			else if (tmpTypeId == BASICTYPE_ANY)
			{
				elmtLevel++;
				fprintf (src, "        %s", varName);
				if (cxxtri->isPtr)
					fprintf (src, "->");
				else
					fprintf (src, ".");
				fprintf (src, "B%s (_b, bytesDecoded);\n",  r->decodeBaseName);
			}
			else if (tmpTypeId == BASICTYPE_BITCONTAINING)
			{
				PrintCxxDecodeContaining(e->type, r, src);
			}
			else
			{
				fprintf (src, "        %s", varName);
				if (cxxtri->isPtr)
					fprintf (src, "->");
				else
					fprintf (src, ".");

				fprintf (src, "B%s (_b, tag, elmtLen%d, bytesDecoded);\n",  r->decodeContentBaseName, elmtLevel);
			}

			/* decode Eoc (s) */
			for (i = elmtLevel-1; i >= 0; i--)
			{
				fprintf (src, "      if (elmtLen%d == INDEFINITE_LEN)\n", i);
				fprintf (src, "        BDecEoc (_b, bytesDecoded);\n");
			}

			fprintf (src, "      break;\n\n");
			FreeTags (tags);
		}
	}

	fprintf (src, "    default:\n");
	fprintf (src, "    {");

	if(extensionsExist)
	{
		fprintf (src, "     AsnAny extAny;\n");
		fprintf (src, "     extension = new AsnExtension;\n");
		fprintf (src, "     choiceId = extensionCid;\n");
		fprintf (src, "     extAny.BDecContent(_b, tag, elmtLen0, bytesDecoded);\n");
		fprintf (src, "     extension->extList.insert( extension->extList.end(), extAny );\n");
	}
	else
	{
		fprintf (src, "        throw InvalidTagException(typeName(), tag, STACK_ENTRY);\n");
	}

	fprintf (src, "      break;\n");
	fprintf (src, "    }\n");
	fprintf (src, "  } // end switch\n");
	fprintf (src, "} // %s::B%s\n\n\n", td->cxxTypeDefInfo->className, r->decodeContentBaseName);
}

void PrintChoiceDefCodeBerEnc(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *choice)
{
	char *classStr;
	char *formStr;
	Tag *tag;
	int tagLen = 0;

	fprintf (hdr, "  %s		B%s (%s &_b) const;\n", lenTypeNameG, r->encodeBaseName, bufTypeNameG);
	fprintf (src, "%s %s::B%s (%s &_b) const\n", lenTypeNameG, td->cxxTypeDefInfo->className, r->encodeBaseName, bufTypeNameG);
	fprintf (src, "{\n");
	fprintf (src, "    %s l=0;\n", lenTypeNameG);
	fprintf (src, "    l = B%s (_b);\n", r->encodeContentBaseName);

	/* encode each tag/len pair if any */
	FOR_EACH_LIST_ELMT_RVS (tag, choice->tags)
	{
		classStr = Class2ClassStr (tag->tclass);
		formStr = Form2FormStr (CONS);  /* choices are constructed */
		//RWC;tagLen = TagByteLen (tag->code);
		fprintf (src, "    l += BEncConsLen (_b, l);\n");

		if (tag->tclass == UNIV)
		{
			const char* ptr = DetermineCode(tag, &tagLen, 1);
			fprintf (src, "    l += BEncTag%d (_b, %s, %s, %s);\n", tagLen, classStr, formStr, ptr);//RWC;Code2UnivCodeStr (tag->code));
		}
		else
		{
			const char* ptr = DetermineCode(tag, &tagLen, 1);
			fprintf (src, "    l += BEncTag%d (_b, %s, %s, %s);\n", tagLen, classStr, formStr, ptr);//RWC;tag->code);
		}
	}
	fprintf (src, "    return l;\n");
	fprintf (src, "}\n\n");
}

void PrintChoiceDefCodeBerDec(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *choice)
{
	char *classStr;
	char *formStr;
	Tag *tag;
	int i = 0;
	int elmtLevel=0;

	fprintf (hdr, "  void			B%s (const %s &_b, %s &bytesDecoded);\n", r->decodeBaseName, bufTypeNameG, lenTypeNameG);
	fprintf (src, "void %s::B%s (const %s &_b, %s &bytesDecoded)\n", td->cxxTypeDefInfo->className, r->decodeBaseName, bufTypeNameG, lenTypeNameG);//, envTypeNameG);
	fprintf (src, "{\n");

	if (choice->tags->count > 0)
		fprintf (src, "    FUNC(\"%s::B%s\")\n", td->cxxTypeDefInfo->className, r->decodeBaseName);

	fprintf (src, "    %s elmtLen = 0;\n", lenTypeNameG);
	fprintf (src, "    %s tag;\n", tagTypeNameG);

	/* print extra locals for redundant lengths */
	for (i = 1; (choice->tags != NULL) && (i <= LIST_COUNT (choice->tags)); i++)
	{
		fprintf (src, "    %s extraLen%d = 0;\n", lenTypeNameG, i);
	}
	fprintf (src, "\n");

	/*  decode tag/length pair (s) */
	elmtLevel = 0;
	FOR_EACH_LIST_ELMT (tag, choice->tags)
	{
		classStr = Class2ClassStr (tag->tclass);
		formStr = Form2FormStr (CONS);  /* choices are constructed */
		fprintf (src, "    AsnTag tagId = BDecTag (_b, bytesDecoded);\n");
		fprintf (src, "    if (tagId != ");
		if (tag->tclass == UNIV)
		{
			fprintf (src, "MAKE_TAG_ID (%s, %s, %s))", classStr, formStr, DetermineCode(tag, NULL, 0));//RWC;Code2UnivCodeStr (tag->code));
		}
		else
		{
			fprintf (src, "MAKE_TAG_ID (%s, %s, %s))", classStr, formStr, DetermineCode(tag, NULL, 1));//RWC;tag->code);
		}
		fprintf (src, "    {\n");
		fprintf (src, "          throw InvalidTagException(typeName(), tagId, STACK_ENTRY);\n");
		fprintf (src, "    }\n");
		fprintf (src, "    extraLen%d = BDecLen (_b, bytesDecoded);\n", ++elmtLevel);
	}

	/* decode identifying tag from choice body */
	fprintf (src, "    /*  CHOICEs are a special case - grab identifying tag */\n");
	fprintf (src, "    /*  this allows easier handling of nested CHOICEs */\n");
	fprintf (src, "    tag = BDecTag (_b, bytesDecoded);\n");
	fprintf (src, "    elmtLen = BDecLen (_b, bytesDecoded);\n");
	fprintf (src, "    B%s (_b, tag, elmtLen, bytesDecoded);\n", r->decodeContentBaseName);

	/* grab any EOCs that match redundant, indef lengths */
	for (i = elmtLevel; i > 0; i--)
	{
		fprintf (src, "    if (extraLen%d == INDEFINITE_LEN)\n", i);
		fprintf (src, "        BDecEoc (_b, bytesDecoded);\n");
	}

	fprintf (src, "}\n\n");
}

void PrintChoiceDefCodeJsonEnc(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *choice)
{
	NamedType *e;
	CxxTRI *cxxtri;
	char *varName;

	//void           JEnc (EJson::Value &b) const;

	fprintf (hdr, "  void			JEnc (EJson::Value &b) const;\n");
	fprintf (src, "void %s::JEnc (EJson::Value &b) const\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "   FUNC(\"%s::JEnc()\");\n",td->cxxTypeDefInfo->className);

	/* print local vars */
	fprintf (src, "  b = EJson::Value(EJson::objectValue);\n\n");
	fprintf (src, "  EJson::Value tmp;\n\n");

	/* print local vars */

	fprintf (src, "  switch (%s)\n", r->choiceIdFieldName);
	fprintf (src, "  {\n");
	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		cxxtri =  e->type->cxxTypeRefInfo;
		if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
		{
			fprintf(src, "    case %s:\n", cxxtri->choiceIdSymbol);

			varName = cxxtri->fieldName;

			/* encode content */
			fprintf(src, "    %s", varName);
			if (cxxtri->isPtr)
				fprintf(src, "->");
			else
				fprintf(src, ".");
			fprintf(src, "JEnc (tmp);\n");
			fprintf(src, "    b[\"%s\"] = tmp;\n", varName);

			fprintf(src, "      break;\n\n");
		}
		else
		{
			fprintf(src, "    //extensions allowed but ignored in json\n");

		}
	}
	fprintf (src, "      default:\n");
	fprintf (src, "         throw EXCEPT(\"Choice is empty\", ENCODE_ERROR);\n");
	fprintf (src, "  } // end switch\n");

	fprintf (src, "} // %s::JEnc\n\n\n", td->cxxTypeDefInfo->className);
}

void PrintChoiceDefCodeJsonDec(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *choice)
{
	NamedType *e;
	char *varName;
	CxxTRI *cxxtri;
	int iCounter = 0;

	fprintf (hdr, "  bool			JDec (const EJson::Value &b);\n\n");
	fprintf (src, "bool %s::JDec (const EJson::Value &b)", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "   FUNC(\"%s::JDec()\");\n",td->cxxTypeDefInfo->className);
	fprintf (src, "  Clear();\n");

	fprintf (src, "  if (!b.isObject()) return false;\n\n");
	fprintf (src, "  EJson::Value tmp;\n");


	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		cxxtri =  e->type->cxxTypeRefInfo;
		varName = cxxtri->fieldName;

		if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
		{
			if (iCounter == 0)
				fprintf(src, "  if (b.isMember(\"%s\")) {\n", varName);
			else
				fprintf(src, "  else if (b.isMember(\"%s\")) {\n", varName);

			fprintf(src, "    %s = %s;\n", r->choiceIdFieldName, cxxtri->choiceIdSymbol);
			if (cxxtri->isPtr)
			{
				fprintf(src, "    delete %s;\n", varName);
				fprintf(src, "    %s = new %s;\n", varName, cxxtri->className);
				fprintf(src, "    if (!%s->JDec(b[\"%s\"])) throw InvalidTagException(typeName(), \"decode failed: %s\", STACK_ENTRY);\n", varName, varName, varName);
			}
			else
			{
				fprintf(src, "    if (!%s.JDec(b[\"%s\"])) throw InvalidTagException(typeName(), \"decode failed: %s\", STACK_ENTRY);\n", varName, varName, varName);
			}

			fprintf(src, "  }\n");
		}
		else
		{
			fprintf(src, "  //extensions allowed but ignored in json\n");
		}
		iCounter++;
	}
	if (iCounter)
	{
		fprintf (src, "  else\n     throw InvalidTagException(typeName(), \"no valid choice member\", STACK_ENTRY);\n");
	}


	fprintf (src, "  return true;\n");
	fprintf (src, "} // %s::JDec\n\n\n", td->cxxTypeDefInfo->className);
}


void PrintChoiceDefCodePEREnc(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *choice)
{
	NamedType *e;
	char *varName;
	CxxTRI *cxxtri;
	enum BasicTypeChoiceId tmpTypeId;
	NamedType *defByNamedType;
	NamedType **ppElementNamedType;
	int *pElementTag;
	int ii;

	/****************************/
	/*** FIRST, handle index encoding for PER Choice.  Taking advantage of
	* the AsnInt class with constraints for the detailed encoding
	* details.  Declare outside scope of source method for PEnc/PDec. */
	fprintf (src, "class AsnIntChoice_%s: public AsnInt  {\n", td->cxxTypeDefInfo->className);
	fprintf (src, "  public:\n");
	fprintf (src, "  AsnIntChoice_%s(AsnIntType val=0):AsnInt(val){ }\n",
		td->cxxTypeDefInfo->className);
	fprintf (src, "  ValueRange* ValueRanges(int &sizeVRList)\n");
	fprintf (src, "  {\n");
	fprintf (src, "  	static ValueRange INT1_ValueRangeList[] = \n");
	fprintf (src, "  		{{ 0, %d, 1 }};\n",
		choice->basicType->a.choice->count);  /* CONSTANT value for this CHOICE. */
	fprintf (src, "  	sizeVRList = 1;\n");
	fprintf (src, "  	return INT1_ValueRangeList;\n");
	fprintf (src, "  }\n");
	fprintf (src, "};\n\n");

	//RWC;fprintf (hdr, "  AsnLen		PEnc(AsnBufBits &_b, bool bAlign = false) const {AsnLen len; len = 1;return len;};\n");
	fprintf (hdr, "  %s		P%s (AsnBufBits &_b) const;\n", lenTypeNameG, r->encodeBaseName);
	fprintf (src, "%s %s::P%s (AsnBufBits &_b) const\n", lenTypeNameG, td->cxxTypeDefInfo->className, r->encodeBaseName);
	fprintf (src, "{\n");
	fprintf (src, "    %s l=0;\n", lenTypeNameG);
	fprintf (src, "    FUNC(\"%s::P%s (AsnBufBits &_b)\");\n", td->cxxTypeDefInfo->className, r->encodeBaseName);

	/****************************/
	/*** PERFORM sorting of Choice elements for proper index setting. */
	PrintCxxDefCode_PERSort(&ppElementNamedType, &pElementTag, choice->basicType->a.choice);
	fprintf (src, "  AsnIntChoice_%s TmpAsnIntChoice(%d);\n",
		td->cxxTypeDefInfo->className,
		choice->basicType->a.choice->count);  /* CONSTANT value for this CHOICE. */
	for (ii=0; ii < choice->basicType->a.choice->count; ii++)
	{
		fprintf (src, "   if (%s == %s::%s)\n", r->choiceIdFieldName,
			td->cxxTypeDefInfo->className,
			ppElementNamedType[ii]->type->cxxTypeRefInfo->choiceIdSymbol);
		fprintf (src, "      TmpAsnIntChoice.Set(%d); // SORTED index value.\n", ii);
	}       // END FOR ii
	free(ppElementNamedType);
	free(pElementTag);

	/*** SETUP specific sorted index value. */
	fprintf (src, "  l = TmpAsnIntChoice.PEnc(_b); // LOAD PER encoded, constrained Choice index value.\n");

	/****************************/
	/*** NOW, setup each individual Choice element.*/
	//RWC;fprintf (src, "    l = P%s (_b);\n", r->encodeContentBaseName);
	/* print local vars */
	//RWC;fprintf (src, "  %s l=0;\n", lenTypeNameG);

	/* encode tag (s) & len (s) */
	//PrintCxxTagAndLenEncodingCode (src, td, e->type, "l", "_b");
	// RWC; TAGS already encoded if necessary above (non-UNIV).

	fprintf (src, "  switch (%s)\n", r->choiceIdFieldName);
	fprintf (src, "  {\n");
	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		cxxtri =  e->type->cxxTypeRefInfo;
		fprintf (src, "    case %s:\n", cxxtri->choiceIdSymbol);

		varName = cxxtri->fieldName;

		/* encode content */
		tmpTypeId = GetBuiltinType (e->type);
		if (tmpTypeId == BASICTYPE_ANYDEFINEDBY)
		{
			defByNamedType = e->type->basicType->a.anyDefinedBy->link;
			PrintCxxSetTypeByCode(defByNamedType, cxxtri, src);
		}
		else if (tmpTypeId == BASICTYPE_ANY)
		{
			fprintf (src, "    l += %s", varName);
			if (cxxtri->isPtr)
				fprintf (src, "->");
			else
				fprintf (src, ".");

			fprintf (src, "P%s (_b);\n", r->encodeBaseName);
		}
		else if (tmpTypeId == BASICTYPE_BITCONTAINING)
		{
			PrintCxxPEREncodeContaining(e->type, r, src);
		}
		else
		{
			fprintf (src, "      l += %s", varName);
			if (cxxtri->isPtr)
				fprintf (src, "->");
			else
				fprintf (src, ".");

			fprintf (src, "P%s (_b);\n", r->encodeBaseName);
		}
		fprintf (src, "      break;\n\n");
	}
	fprintf (src, "      default:\n");
	fprintf (src, "         throw EXCEPT(\"Choice is empty\", ENCODE_ERROR);\n");
	fprintf (src, "  } // end switch\n");
	/****************************/

	fprintf (src, "    return l;\n");
	fprintf (src, "}    //%s::P%s(...)\n\n", td->cxxTypeDefInfo->className, r->encodeBaseName);
}

void PrintChoiceDefCodePERDec(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *choice)
{
	NamedType *e;
	int i;
	char *varName;
	CxxTRI *cxxtri;
	int varCount, tmpVarCount;
	enum BasicTypeChoiceId tmpTypeId;
	NamedType *defByNamedType;
	NamedType **ppElementNamedType;
	int *pElementTag;
	int ii;

	fprintf (hdr, "  void			P%s (AsnBufBits &_b, %s &bitsDecoded);\n", r->decodeBaseName, lenTypeNameG);
	fprintf (src, "void %s::P%s (AsnBufBits &_b, %s &bitsDecoded)\n", td->cxxTypeDefInfo->className, r->decodeBaseName, lenTypeNameG);//, envTypeNameG);
	fprintf (src, "{\n");
	fprintf (src, "\tClear();\n");

	/* print extra locals for redundant lengths */
	for (i = 1; (choice->tags != NULL) && (i <= LIST_COUNT (choice->tags)); i++)
	{
		//fprintf (src, "    %s extraLen%d = 0; \n", lenTypeNameG, i);
	}
	fprintf (src, "\n");

	/****************************/
	fprintf (src, "  AsnIntChoice_%s TmpAsnIntChoice;\n", td->cxxTypeDefInfo->className);

	/*** SETUP specific sorted index value. */
	fprintf (src, "  TmpAsnIntChoice.PDec(_b, bitsDecoded); // LOAD PER decoded, constrained Choice index value.\n");

	/* decode identifying tag from choice body */
	fprintf (src, "    /*  CHOICEs are a special case - grab identifying tag */\n");
	fprintf (src, "    /*  this allows easier handling of nested CHOICEs */\n");

	/* print local vars */
	/* count max number of extra length var nec
	* by counting tag/len pairs on components of the CHOICE
	*/
	varCount = 0;
	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		tmpVarCount = CxxCountVariableLevels (e->type);
		if (tmpVarCount > varCount)
			varCount = tmpVarCount;
	}

	/*** PERFORM sorting of Choice elements for proper index setting, then
	determine actual Choice chosen by the user. */
	PrintCxxDefCode_PERSort(&ppElementNamedType, &pElementTag, choice->basicType->a.choice);

	if (!choice->basicType->a.choice)
		exit(3);

	for (ii=0; ii < choice->basicType->a.choice->count; ii++)
	{
		fprintf (src, "   if (TmpAsnIntChoice == %d)\n", ii);
		fprintf (src, "   {\n");
		fprintf (src, "      %s = %s::%s;\n", r->choiceIdFieldName,
			td->cxxTypeDefInfo->className,
			ppElementNamedType[ii]->type->cxxTypeRefInfo->choiceIdSymbol);

		/* Process specific tag - choices always have the key tag decoded */
		e = ppElementNamedType[ii];
		cxxtri =  e->type->cxxTypeRefInfo;

		varName = cxxtri->fieldName;

		/* alloc elmt if nec */
		if (cxxtri->isPtr)
		{
			fprintf (src, "      %s = new %s;\n", varName, cxxtri->className);
		}
		/* decode content */
		tmpTypeId = GetBuiltinType (e->type);
		if (tmpTypeId == BASICTYPE_ANYDEFINEDBY)
		{
			/*
			* must check for another EOC for ANYs
			* since the any decode routines decode
			* their own first tag/len pair
			*/
			defByNamedType = e->type->basicType->a.anyDefinedBy->link;
			PrintCxxSetTypeByCode(defByNamedType, cxxtri, src);

			fprintf (src, "      %s", varName);
			if (cxxtri->isPtr)
				fprintf (src, "->");
			else
				fprintf (src, ".");
			fprintf (src, "P%s (_b, bitsDecoded);\n",  r->decodeBaseName);
		}
		else if (tmpTypeId == BASICTYPE_ANY)
		{
			fprintf (src, "        %s", varName);
			if (cxxtri->isPtr)
				fprintf (src, "->");
			else
				fprintf (src, ".");
			fprintf (src, "P%s (_b, bitsDecoded);\n",  r->decodeBaseName);
		}
		else if (tmpTypeId == BASICTYPE_BITCONTAINING)
		{
			PrintCxxPERDecodeContaining(e->type, r, src);
		}
		else
		{
			fprintf (src, "      %s", varName);
			if (cxxtri->isPtr)
				fprintf (src, "->");
			else
				fprintf (src, ".");

			fprintf (src, "P%s (_b, bitsDecoded);\n",  r->decodeBaseName);
		}


		fprintf (src, "   }  // END if this Choice ID chosen\n");
	}       // END FOR ii

	free(ppElementNamedType);
	free(pElementTag);

	fprintf (src, "}   // END %s::P%s(...)\n\n", td->cxxTypeDefInfo->className, r->decodeBaseName);
}

static void PrintCxxChoiceDefCode(FILE *src, FILE *hdr, ModuleList *mods, Module *m, CxxRules *r ,
								  TypeDef *td,Type *parent, Type *choice, int novolatilefuncs)
{
	NamedType *e;
	enum BasicTypeChoiceId tmpTypeId;
	char *ptr="";   /* NOT DLL Exported, or ignored on Unix. */

	/* put class spec in hdr file */

	if (bVDAGlobalDLLExport != NULL)
		ptr = bVDAGlobalDLLExport;

	if (gAlternateNamespaceString == 0 &&
		(strcmp(td->cxxTypeDefInfo->className, "AsnOptionalParam") == 0 ||
		strcmp(td->cxxTypeDefInfo->className, "AsnOptionalParamChoice") == 0))
		return;

	fprintf (hdr, "class %s %s%s\n", ptr, td->cxxTypeDefInfo->className, baseClassesG);
	fprintf (hdr, "{\n");
	fprintf (hdr, "public:\n");

	/* write out choiceId enum type */

	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		if(e->type->subtypes != NULL)
		{    switch(e->type->subtypes->choiceId)
		{
					case SUBTYPE_AND:
					case SUBTYPE_OR:
					case SUBTYPE_SINGLE:
						{
							PrintCxxMultiConstraintOrHandler(hdr, src, td->cxxTypeDefInfo->className, e, 3);
							break;
						}
					default:
						{
							break;
						}
		}
		}

	}


	fprintf (hdr, "  enum %s\n", r->choiceIdEnumName);
	fprintf (hdr, "  {\n");
	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		fprintf (hdr, "     %s = %d", e->type->cxxTypeRefInfo->choiceIdSymbol, e->type->cxxTypeRefInfo->choiceIdValue);
		if (e != (NamedType*)LAST_LIST_ELMT (choice->basicType->a.choice))
			fprintf (hdr, ",\n");
		else
			fprintf (hdr, "\n");
	}
	fprintf (hdr, "  };\n\n");

	/* write out the choice Id field */
	fprintf (hdr, "  enum %s	%s;\n", r->choiceIdEnumName, r->choiceIdFieldName);

	/* write out the choice element anonymous union */
	fprintf (hdr, "  union\n");
	fprintf (hdr, "  {\n");
	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		fprintf (hdr, "     ");
		PrintCxxType (hdr, mods, m, r, td, choice, e->type);
		fprintf (hdr, "%s;\n", e->type->cxxTypeRefInfo->fieldName);
	}
	fprintf (hdr, "  };\n\n");

#if META
	if (printMetaG)
	{
		PrintCxxChoiceDefCodeMeta_1(hdr, src, td, choice, m, e);
	}
#endif /* META */

	fprintf (hdr, "\n");

	/* constructors and destructor */
	fprintf (hdr, "   %s() {Init();}\n", td->cxxTypeDefInfo->className);

	/* PIERCE 8-22-2001 */
	PrintCopyConstructor(hdr, src, td->cxxTypeDefInfo->className);
	PrintSimpleMeta(hdr,td->cxxTypeDefInfo->className,0);

	/* Init() member function*/
	fprintf (hdr, "   void Init(void);\n");
	fprintf (src, "void %s::Init(void)\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "   // initialize choice to no choiceId to first choice and set pointer to NULL\n");
	e = FIRST_LIST_ELMT (choice->basicType->a.choice);
	if (!e)
		exit(3);
	fprintf (src, "  choiceId = %sCid;\n", e->type->cxxTypeRefInfo->fieldName);
	fprintf (src, "  %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
	fprintf (src, "}\n\n");
	fprintf (hdr, "\n   virtual int checkConstraints(ConstraintFailList* pConstraintFails)const;\n\n");
	fprintf(src, "\nint %s::checkConstraints(ConstraintFailList* pConstraintFails) const\n{\n",
		td->cxxTypeDefInfo->className);

	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		if (e->type->cxxTypeRefInfo->isPtr)
		{
			fprintf(src, "\tif (%s != NULL)\n",
				e->type->cxxTypeRefInfo->fieldName);
			fprintf(src, "\t\t%s->checkConstraints(pConstraintFails);\n\n",
				e->type->cxxTypeRefInfo->fieldName);
		}
		else
		{
			fprintf(src, "\t%s.checkConstraints(pConstraintFails);\n\n",
				e->type->cxxTypeRefInfo->fieldName);
		}
	}
	fprintf(src, "\treturn 0;\n");
	fprintf(src, "}\n\n\n");

	/* virtual destructor*/
	fprintf (hdr, "   virtual ~%s() {Clear();}\n\n", td->cxxTypeDefInfo->className);

	/* Clear() member*/
	fprintf (hdr, "   void Clear();\n\n");
	fprintf (src, "void %s::Clear()\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  switch (choiceId)\n");
	fprintf (src, "  {\n");

	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		tmpTypeId = GetBuiltinType (e->type);

		fprintf (src, "    case %s:\n", e->type->cxxTypeRefInfo->choiceIdSymbol);
		if (e->type->cxxTypeRefInfo->isPtr)
		{
			fprintf (src, "      delete %s;\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "      %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
		}
		else if(!e->type->cxxTypeRefInfo->isPtr &&
			((tmpTypeId == BASICTYPE_CHOICE) ||
			(tmpTypeId == BASICTYPE_SET) ||
			(tmpTypeId == BASICTYPE_SEQUENCE)) )
		{
			fprintf (src, "  %s.Clear();\n", e->type->cxxTypeRefInfo->fieldName);
		}
		fprintf (src, "      break;\n");
	}

	fprintf (src, "  } // end of switch\n");
	fprintf (src, "} // end of Clear()\n");
	fprintf (src, "\n");

	/* print clone routine for ANY mgmt */
	PrintCloneMethod (hdr, src, td);

	fprintf (hdr, "  %s		&operator = (const %s &that);\n", td->cxxTypeDefInfo->className, td->cxxTypeDefInfo->className);
	fprintf (src, "%s &%s::operator = (const %s &that)\n", td->cxxTypeDefInfo->className, td->cxxTypeDefInfo->className, td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  if (this != &that)\n");
	fprintf (src, "  {\n");
	fprintf (src, "    Clear();\n");

	e = FIRST_LIST_ELMT (choice->basicType->a.choice);
	fprintf (src, "    // Check first type in choice to determine if choice is empty\n");
	fprintf (src, "    if (that.%s != NULL)\n", e->type->cxxTypeRefInfo->fieldName);
	fprintf (src, "    {\n");
	fprintf (src, "       switch (choiceId = that.choiceId)\n");
	fprintf (src, "       {\n");

	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		fprintf (src, "         case %s:\n", e->type->cxxTypeRefInfo->choiceIdSymbol);
		if (e->type->cxxTypeRefInfo->isPtr)
		{
			fprintf (src, "           %s = new %s(*that.%s);\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className,
				e->type->cxxTypeRefInfo->fieldName);
		}
		else
		{
			fprintf (src, "           %s = that.%s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->fieldName);
		}
		fprintf (src, "           break;\n");
	}

	fprintf (src, "       }// end of switch\n");
	fprintf (src, "     }// end of if\n");
	fprintf (src, "  }\n");
	fprintf (src, "\n");
	fprintf (src, "  return *this;\n");
	fprintf (src, "}\n\n");

	/* BerEncodeContent */
	if (printEncodersG)
	{
		PrintChoiceDefCodeBerEncodeContent(src, hdr, r, td, choice);
	}
	/* end of BerEncodeContent method */

	/* BerDecodeContent */
	if (printDecodersG)
	{
		PrintChoiceDefCodeBerDecodeContent(src, hdr, r, td, choice);
	}
	/* end of code for printing BDecodeContent method */

	/* do BEnc function */
	if (printEncodersG)
	{
		PrintChoiceDefCodeBerEnc(src, hdr, r, td, choice);

	}
	/* end of BEnc function */
	if (printJSONEncDecG)
		PrintChoiceDefCodeJsonEnc(src, hdr, r, td, choice);

	/* Do BDec function */
	if (printDecodersG)
	{
		PrintChoiceDefCodeBerDec(src, hdr, r, td, choice);
	}
	/* end of BDec function */

	if (printJSONEncDecG)
		PrintChoiceDefCodeJsonDec(src, hdr, r, td, choice);

	if(genPERCode)
	{
		/* do PER Encode, PEnc function */
		if (printEncodersG)
		{
			PrintChoiceDefCodePEREnc(src, hdr, r, td, choice);
		}
		/* end of PEnc function */

		/* Do PDec function */
		if (printDecodersG)
		{
			PrintChoiceDefCodePERDec(src, hdr, r, td, choice);
		}
		/* end of PDec function */
	} /* if genPERCode */


	/* ostream printing routine */
	if (printPrintersG)
	{
		fprintf(hdr, "  void Print(std::ostream& os, unsigned short indent = 0) const;\n");
		fprintf(src, "void %s::Print(std::ostream& os, unsigned short indent) const\n",
			td->cxxTypeDefInfo->className);
		fprintf(src, "{\n");
		fprintf(src, "\tswitch (choiceId)\n");
		fprintf(src, "\t{\n");

		FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
		{
			fprintf (src, "\tcase %s:\n", e->type->cxxTypeRefInfo->choiceIdSymbol);

			/* value notation so print the choice elmts field name */
			if (e->fieldName != NULL)
				fprintf(src, "\t\tos << \"%s \";\n", e->fieldName);

			if (e->type->cxxTypeRefInfo->isPtr)
			{
				fprintf(src, "\t\tif (%s)\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf(src, "\t\t\t%s->Print(os, indent);\n",
					e->type->cxxTypeRefInfo->fieldName);
				fprintf(src, "\t\telse\n");
				fprintf(src, "\t\t\tos << \"<CHOICE value is missing>\\n\";\n");
			}
			else
			{
				fprintf(src, "\t\t%s.Print(os, indent);\n",
					e->type->cxxTypeRefInfo->fieldName);
			}

			fprintf (src, "\t\tbreak;\n\n");
		}
		fprintf (src, "\t} // end of switch\n");

		fprintf (src, "} // end of %s::Print()\n\n", td->cxxTypeDefInfo->className);
		/* ################################################################## */
	}
	if (printPrintersXMLG)
	{
		/* RWC;1/12/00; ADDED XML output capability. */
		fprintf (hdr, "  void			PrintXML (std::ostream &os, const char *lpszTitle=NULL) const;\n");

		fprintf (src, "void %s::PrintXML (std::ostream &os, const char *lpszTitle) const\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");
		fprintf (src, "  if (lpszTitle)\n");
		fprintf (src, "  {\n");
		fprintf (src, "     os << \"<\" << lpszTitle;\n");
		fprintf (src, "        os << \" typeName=\\\"%s\\\" type=\\\"CHOICE\\\">\";\n", td->cxxTypeDefInfo->className);
		fprintf (src, "  }\n");
		fprintf (src, "  else\n");
		fprintf (src, "        os << \"<%s type=\\\"CHOICE\\\">\";\n", td->cxxTypeDefInfo->className);
		fprintf (src, "  switch (choiceId)\n");
		fprintf (src, "  {\n");

		FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
		{
			fprintf (src, "    case %s:\n", e->type->cxxTypeRefInfo->choiceIdSymbol);

			/* value notation so print the choice elmts field name */
			if (e->type->cxxTypeRefInfo->isPtr)
			{
				fprintf (src, "      if (%s)\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "        %s->PrintXML(os", e->type->cxxTypeRefInfo->fieldName);
				if (e->fieldName != NULL)
					fprintf (src, ",\"%s\");\n", e->fieldName);
				else
					fprintf (src, ");\n");
				fprintf (src, "      else\n");
				fprintf (src, "      {\n");

				if (e->fieldName != NULL)
				{
					fprintf (src, "        os << \"<%s -- void3 -- /%s>\" << std::endl;\n", e->fieldName, e->fieldName);
				}
				else
				{
					fprintf (src, "        os << \"<%s -- void3 -- /%s>\" << std::endl;\n",
						e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->fieldName);
				}

				fprintf (src, "      }\n");
			}
			else
				fprintf (src, "      %s.PrintXML(os, \"%s\");\n", e->type->cxxTypeRefInfo->fieldName, e->fieldName);

			fprintf (src, "      break;\n\n");
		}
		fprintf (src, "  } // end of switch\n");
		fprintf (src, "  if (lpszTitle)\n");
		fprintf (src, "     os << \"</\" << lpszTitle << \">\";\n");
		fprintf (src, "  else\n");
		fprintf (src, "        os << \"</%s>\";\n", td->cxxTypeDefInfo->className);
		fprintf (src, "} // %s::PrintXML\n\n", td->cxxTypeDefInfo->className);

		/* END XML Print capability. */
		/* ################################################################## */
	}
	/* end of Print Method code */

	/* close class definition */
	fprintf (hdr, "};\n\n\n");
} /* PrintCxxChoiceDefCode */


void PrintSeqDefCodeBerEncodeContent(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *seq)
{
	NamedType *e;
	char *varName;
	CxxTRI *cxxtri=NULL;
	enum BasicTypeChoiceId tmpTypeId;
	NamedType *defByNamedType;

	fprintf (hdr, "  %s		B%s (%s &_b) const;\n", lenTypeNameG, r->encodeContentBaseName, bufTypeNameG);
	fprintf (src, "%s\n", lenTypeNameG);
	fprintf (src, "%s::B%s (%s &_b) const\n", td->cxxTypeDefInfo->className, r->encodeContentBaseName, bufTypeNameG);
	fprintf (src, "{\n");

	/* print local vars */
	fprintf (src, "  %s totalLen = 0;\n", lenTypeNameG);
	fprintf (src, "  %s l=0;\n\n", lenTypeNameG);

	FOR_EACH_LIST_ELMT_RVS (e, seq->basicType->a.sequence)
	{
		cxxtri =  e->type->cxxTypeRefInfo;
		varName = cxxtri->fieldName;

		/* print optional test if nec */
		if (e->type->defaultVal != NULL)
		{
			Value *defVal = GetValue(e->type->defaultVal->value);
			/** PIERCE added DER DEFAULT encoding rules 8-16-2000
			**/

			/* HANDLE DEFAULT VALUE ENCODING FOR DER
			*/
			switch(ParanoidGetBuiltinType(e->type))
			{
			case BASICTYPE_INTEGER:
			case BASICTYPE_ENUMERATED:
				fprintf(src,"  if ( %s(%s) && *%s != %d )\n  {\n",
					cxxtri->optTestRoutineName,
					varName, varName, defVal->basicValue->a.integer);
				break;
			case BASICTYPE_BITSTRING:
				{
					if (defVal->basicValue->choiceId == BASICVALUE_VALUENOTATION)
					{
						char *defBitStr;
						normalizeValue(&defBitStr, defVal->basicValue->a.valueNotation->octs);

						//ste inserted check for default empty
						if (strlen(defBitStr))
						{
							//check for default bit.
							fprintf(src,"  if ( %s(%s) && (! %s->soloBitCheck(%s::%s)) ) \n  {\n",
								cxxtri->optTestRoutineName, varName, varName, cxxtri->className, defBitStr);
						}
						else
						{
							//if default is empty then check for empty
							fprintf(src,"  if ( %s(%s) && (! %s->IsEmpty()) ) \n  {\n",
								cxxtri->optTestRoutineName, varName, varName);

						}

						//RWC;ALLOW "}" alignment using editor...
						free(defBitStr);
					}
					else
						printf("\nWARNING: unsupported use of default BIT STRING\n");
				}
				break;
			case BASICTYPE_BOOLEAN:
				fprintf(src,"  if ( %s(%s) && *%s != %s )\n  {\n",
					cxxtri->optTestRoutineName, varName, varName, defVal->basicValue->a.boolean ? "true" : "false");
				//RWC;ALLOW "}" alignment using editor...
				break;
			default:
				/* TBD print error? */
				break;
			}
		}
		else if (e->type->optional)
		{
			fprintf (src, "  if (%s (%s))\n", cxxtri->optTestRoutineName, varName);
			fprintf (src, "  {\n");
			//RWC;ALLOW "}" alignment using editor...
		}


		/* eSNACC 1.5 does not encode indefinite length
		*
		* PrintCxxEocEncoders (src, td, e->type, "_b");
		*/

		/* encode content */
		tmpTypeId = GetBuiltinType (e->type);
		if (tmpTypeId == BASICTYPE_ANYDEFINEDBY)
		{
			defByNamedType = e->type->basicType->a.anyDefinedBy->link;
			PrintCxxSetTypeByCode(defByNamedType, cxxtri, src);

			fprintf (src, "    l = %s", varName);
			if (cxxtri->isPtr)
				fprintf (src, "->");
			else
				fprintf (src, ".");
			fprintf (src, "B%s (_b);\n", r->encodeBaseName);
		}
		else if (tmpTypeId == BASICTYPE_ANY)
		{
			fprintf (src, "    l = %s", varName);
			if (cxxtri->isPtr)
				fprintf (src, "->");
			else
				fprintf (src, ".");

			fprintf (src, "B%s (_b);\n", r->encodeBaseName);
		}
		else if (tmpTypeId == BASICTYPE_BITCONTAINING)
		{
			PrintCxxEncodeContaining(e->type, r, src);
		}
		else if ( tmpTypeId == BASICTYPE_EXTENSION )
		{
			fprintf (src, "    l = %s", varName);
			if (cxxtri->isPtr)
				fprintf (src, "->");
			else
				fprintf (src, ".");

			fprintf (src, "BEnc (_b);\n");
		}
		else
		{
			fprintf (src, "    l = %s", varName);
			if (cxxtri->isPtr)
				fprintf (src, "->");
			else
				fprintf (src, ".");

			fprintf (src, "B%s (_b);\n", r->encodeContentBaseName);
		}


		/* encode tag (s) & len (s) */
		PrintCxxTagAndLenEncodingCode (src, td, e->type, "l", "_b");
		fprintf (src, "    totalLen += l;\n");

		/* close optional test if nec */
		if (e->type->optional || (e->type->defaultVal != NULL))
			fprintf (src, "  }\n\n");
		else
			fprintf (src, "\n");
	}
	fprintf (src, "  return totalLen;\n");
	fprintf (src, "} // %s::B%s\n\n\n", td->cxxTypeDefInfo->className, r->encodeContentBaseName);
}

void PrintSeqDefCodeBerDecodeContent(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *seq)
{
	NamedType *e;
	char *classStr;
	char *formStr;
	char *codeStr;
	int i=0;
	Tag *tag;
	TagList *tags;
	char *varName;
	CxxTRI *cxxtri=NULL;
	int elmtLevel=0;
	int varCount, tmpVarCount;
	int stoleChoiceTags;
	int inTailOptElmts;
	enum BasicTypeChoiceId tmpTypeId;
	NamedType *defByNamedType;
	NamedType *tmpElmt;
	int extensionAdditionFound = FALSE;

	fprintf (hdr, "  void			B%s (const %s &_b, %s tag, %s elmtLen, %s &bytesDecoded);\n\n", r->decodeContentBaseName, bufTypeNameG, tagTypeNameG, lenTypeNameG, lenTypeNameG);//, envTypeNameG);
	fprintf (src, "void %s::B%s (const %s &_b, %s /*tag0*/, %s elmtLen0, %s &bytesDecoded)\n", td->cxxTypeDefInfo->className, r->decodeContentBaseName, bufTypeNameG, tagTypeNameG, lenTypeNameG, lenTypeNameG);//, envTypeNameG);
	fprintf (src, "{\n");
	fprintf (src, "  FUNC(\" %s::B%s\");\n", td->cxxTypeDefInfo->className, r->decodeContentBaseName);
	fprintf (src, "  Clear();\n");

	/* PL: removed to avoid used variable warning
	fprintf (src, "    AsnBufLoc readLoc = _b.GetReadLoc();\n");
	*/

	/* print local vars */
	fprintf (src, "  %s tag1 = %s();\n", tagTypeNameG, tagTypeNameG);
	fprintf (src, "  %s seqBytesDecoded = 0;\n", lenTypeNameG);
	/* count max number of extra length var nec */
	varCount = 0;

	FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
	{
		tmpVarCount = CxxCountVariableLevels (e->type);

		if (tmpVarCount > varCount)
			varCount = tmpVarCount;
	}

	/* write extra length vars */
	for (i = 1; i <= varCount; i++)
		fprintf (src, "  %s elmtLen%d = 0;\n", lenTypeNameG, i);

	/* handle empty seq */
	if ((seq->basicType->a.sequence == NULL) || LIST_EMPTY (seq->basicType->a.sequence))
	{
		fprintf (src, "  if (elmtLen0 == INDEFINITE_LEN)\n");
		fprintf (src, "    BDecEoc (_b, bytesDecoded);\n");
		fprintf (src, "  else if (elmtLen0 != 0)\n");
		fprintf (src, "  {\n");
		fprintf (src, "          throw EXCEPT(\"Expected an empty sequence\", DECODE_ERROR);\n");
		fprintf (src, "  }\n");
	}
	else
	{
		/*  check if all elmts are optional */
		AsnListFirst (seq->basicType->a.sequence);
		inTailOptElmts = IsTailOptional (seq->basicType->a.sequence);
		e = (NamedType*)FIRST_LIST_ELMT (seq->basicType->a.sequence);
		if (!e)
			exit(3);
		tmpTypeId = GetBuiltinType (e->type);

		if (!inTailOptElmts)
		{
			if (((tmpTypeId == BASICTYPE_ANY) || (tmpTypeId == BASICTYPE_ANYDEFINEDBY)) && (CountTags (e->type) == 0))
			{
				if ((e->type->optional) && (e != (NamedType*)LAST_LIST_ELMT (seq->basicType->a.sequence)))
					fprintf (src, "<untagged optional ANY - you must fix this>\n");
			}
			else
			{
				if (!e->type->extensionAddition)
				{
                    fprintf(src, "  if (elmtLen0 == 0)\n"); // Wenn die Länge 0 ist, gibts nix zum Auslesen -> fertig
                    fprintf(src, "    return;\n");
					fprintf(src, "  tag1 = BDecTag (_b, seqBytesDecoded);\n\n");
				}
			}
		}
		else
		{
			fprintf (src, "  if (elmtLen0 == 0)\n");
			fprintf (src, "    return;\n");
			fprintf (src, "  else\n");
			fprintf (src, "  {\n");
			if (((tmpTypeId == BASICTYPE_ANY) || (tmpTypeId == BASICTYPE_ANYDEFINEDBY)) && (CountTags (e->type) == 0))
			{
				if ((e->type->optional) && (e != (NamedType*)LAST_LIST_ELMT (seq->basicType->a.sequence)))
					fprintf (src, "<untagged optional ANY - you must fix this>\n");
			}
			else
				fprintf (src, "    tag1 = BDecTag (_b, seqBytesDecoded);\n\n");
			fprintf (src, "    if ((elmtLen0 == INDEFINITE_LEN) && (tag1 == EOC_TAG_ID))\n");
			fprintf (src, "    {\n");
			fprintf (src, "      BDEC_2ND_EOC_OCTET (_b, seqBytesDecoded);\n");
			fprintf (src, "      bytesDecoded += seqBytesDecoded;\n");
			fprintf (src, "      return;\n");
			fprintf (src, "    }\n");
			fprintf (src, "  }\n\n");
		}

		/***********************************************************************/
		/***********************************************************************/
		/***********************************************************************/
		FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
		{

			if( e->type->basicType->choiceId != BASICTYPE_EXTENSION )
			{
				cxxtri =  e->type->cxxTypeRefInfo;
				elmtLevel = 0;

				tags = GetTags (e->type, &stoleChoiceTags);

				if (LIST_EMPTY (tags))
					fprintf (src, "  // ANY type\n");
				else
				{
					tag = (Tag*)FIRST_LIST_ELMT (tags);
					if (!tag)
						exit(3);
					classStr = Class2ClassStr (tag->tclass);
					codeStr = DetermineCode(tag, NULL, 0);//RWC;Code2UnivCodeStr (tag->code);
					formStr = Form2FormStr (tag->form);

					fprintf (src, "  if (");
					if (tag->tclass == UNIV)
					{
						if (tag->form == ANY_FORM)
						{
							fprintf (src, "(tag1 == MAKE_TAG_ID (%s, %s, %s))\n", classStr, Form2FormStr (PRIM), codeStr);
							fprintf (src, "    || (tag1 == MAKE_TAG_ID (%s, %s, %s))", classStr, Form2FormStr (CONS), codeStr);
						}
						else
							fprintf (src, "tag1 == MAKE_TAG_ID (%s, %s, %s)", classStr, formStr, codeStr);
					}
					else
					{
						if (tag->form == ANY_FORM)
						{
							fprintf (src, "(tag1 == MAKE_TAG_ID (%s, %s, %s))\n", classStr, Form2FormStr (PRIM), DetermineCode(tag, NULL, 1));//RWC;tag->code);
							fprintf (src, "    || (tag1 == MAKE_TAG_ID (%s, %s, %s))", classStr, Form2FormStr (CONS), DetermineCode(tag, NULL, 1));//RWC;tag->code);
						}
						else
							fprintf (src, "tag1 == MAKE_TAG_ID (%s, %s, %s)", classStr, formStr, DetermineCode(tag, NULL, 1));//RWC;tag->code);
					}

					/* now decode extra tags/length pairs */
					AsnListFirst (tags);
					AsnListNext (tags);
					if (stoleChoiceTags)
					{
						FOR_REST_LIST_ELMT (tag, tags)
						{
							fprintf (src, "\n    || ");
							classStr = Class2ClassStr (tag->tclass);
							formStr = Form2FormStr (tag->form);

							if (tag->tclass == UNIV)
								codeStr = DetermineCode(tag, NULL, 0);//RWC;Code2UnivCodeStr (tag->code);
							else
								codeStr = DetermineCode(tag, NULL, 1);
							if (tag->form == ANY_FORM)
							{
								fprintf (src, "(tag1 == MAKE_TAG_ID (%s, %s, %s))\n", classStr, Form2FormStr (PRIM), codeStr);
								fprintf (src, "    || (tag1 == MAKE_TAG_ID (%s, %s, %s))", classStr, Form2FormStr (CONS), codeStr);
							}
							else
							{
								fprintf (src, "tag1 == MAKE_TAG_ID (%s, %s, %s)", classStr, formStr, codeStr);
							}
						}
						fprintf (src, ")\n");
						fprintf (src, "  {\n");
						fprintf (src, "    elmtLen%d = BDecLen (_b, seqBytesDecoded);\n", ++elmtLevel);
					}
					else /* didn't steal nested choice's tags */
					{
						fprintf (src, ")\n");
						fprintf (src, "  {\n");
						fprintf (src, "    elmtLen%d = BDecLen (_b, seqBytesDecoded);\n", ++elmtLevel);

						FOR_REST_LIST_ELMT (tag, tags)
						{
							classStr = Class2ClassStr (tag->tclass);
							formStr = Form2FormStr (tag->form);

							fprintf (src, "    tag1 = BDecTag (_b, seqBytesDecoded);\n\n");
							if (tag->tclass == UNIV)
								codeStr = DetermineCode(tag, NULL, 0);//RWC;Code2UnivCodeStr (tag->code);
							else
								codeStr = DetermineCode(tag, NULL, 1);
							if (tag->form == ANY_FORM)
							{
								fprintf (src, "    if ((tag1 != MAKE_TAG_ID (%s, %s, %s))\n", classStr, Form2FormStr (PRIM), codeStr);
								fprintf (src, "       && (tag1 != MAKE_TAG_ID (%s, %s, %s)))\n", classStr, Form2FormStr (CONS), codeStr);
							}
							else
								fprintf (src, "    if (tag1 != MAKE_TAG_ID (%s, %s, %s))\n", classStr, formStr, codeStr);

							fprintf (src, "    {\n");
							fprintf (src, "          throw InvalidTagException(typeName(), tag1, STACK_ENTRY);\n");
							fprintf (src, "    }\n\n");
							fprintf (src, "    elmtLen%d = BDecLen (_b, seqBytesDecoded);\n", ++elmtLevel);
						}
					}
				}

				/*
				* if this seq element is CHOICE &&
				* we didn't steal its tags then we must grab
				* the key tag out of the contained CHOICE
				*/
				if (!stoleChoiceTags && (GetBuiltinType (e->type) == BASICTYPE_CHOICE))
				{
					fprintf (src, "    tag1 = BDecTag (_b, seqBytesDecoded);\n");
					fprintf (src, "    elmtLen%d = BDecLen (_b, seqBytesDecoded);\n", ++elmtLevel);
				}

				varName = cxxtri->fieldName;

				/* decode content */
				if (cxxtri->isPtr)
				{
					if (e->type->defaultVal)
					{
						fprintf(src, "  // delete default value\n");
						fprintf(src, "  delete %s;\n", varName);
					}
					fprintf (src, "  %s = new %s;\n", varName, cxxtri->className);
				}

				/* decode content */
				tmpTypeId = GetBuiltinType (e->type);
				if (tmpTypeId == BASICTYPE_ANYDEFINEDBY)
				{
					/*
					* must check for another EOC for ANYs
					* since the any decode routines decode
					* their own first tag/len pair
					*/
					elmtLevel++;

					defByNamedType = e->type->basicType->a.anyDefinedBy->link;
					PrintCxxSetTypeByCode(defByNamedType, cxxtri, src);

					fprintf (src, "  %s", varName);
					if (cxxtri->isPtr)
						fprintf (src, "->");
					else
						fprintf (src, ".");
					fprintf (src, "B%s (_b, seqBytesDecoded);\n",  r->decodeBaseName);
				}
				else if (tmpTypeId == BASICTYPE_ANY)
				{
					elmtLevel++;
					fprintf (src, "  %s", varName);
					if (cxxtri->isPtr)
						fprintf (src, "->");
					else
						fprintf (src, ".");
					fprintf (src, "B%s (_b, seqBytesDecoded);\n",  r->decodeBaseName);
				}
				else if (tmpTypeId == BASICTYPE_BITCONTAINING)
				{
					PrintCxxDecodeContaining(e->type, r, src);
				}
				else
				{
					fprintf (src, "    %s", varName);
					if (cxxtri->isPtr)
						fprintf (src, "->");
					else
						fprintf (src, ".");

					fprintf (src, "B%s (_b, tag1, elmtLen%d, seqBytesDecoded);\n",  r->decodeContentBaseName, elmtLevel);
				}

				/* decode Eoc (s) */
				for (i = elmtLevel-1; i > 0; i--)
				{
					fprintf (src, "    if (elmtLen%d == INDEFINITE_LEN)\n", i);
					fprintf (src, "      BDecEoc (_b, seqBytesDecoded);\n\n");
				}

				/*
				* print code for getting the next tag
				*/
				inTailOptElmts = RestAreTailOptional (seq->basicType->a.sequence);
				if  (e != (NamedType*)LAST_LIST_ELMT (seq->basicType->a.sequence))
				{
					tmpElmt = (NamedType*)NEXT_LIST_ELMT (seq->basicType->a.sequence);
					if (!tmpElmt)
						exit(3);
					tmpTypeId = GetBuiltinType (tmpElmt->type);
					if (!inTailOptElmts)
					{
						if ((tmpTypeId == BASICTYPE_ANY || tmpTypeId == BASICTYPE_ANYDEFINEDBY) && CountTags (tmpElmt->type) == 0)
						{
							/* don't get a tag since ANY's decode their own */
							if (e->type->optional || (tmpElmt->type->optional && tmpElmt != (NamedType*)LAST_LIST_ELMT (seq->basicType->a.sequence)))
								/* let this cause a compile error in the generated code */
								fprintf (src, "  <problems with untagged ANY that is optional or follows an optional sequence element - you must fix this>\n");
						}
						else
						{
							if(!tmpElmt->type->extensionAddition)
							{
								fprintf (src, "    tag1 = BDecTag (_b, seqBytesDecoded);\n");
							}
						}
					}
					else
					{
						fprintf (src, "    if (seqBytesDecoded == elmtLen0)\n");
						fprintf (src, "    {\n");
						fprintf (src, "      bytesDecoded += seqBytesDecoded;\n");
						fprintf (src, "      return;\n");
						fprintf (src, "    }\n");
						fprintf (src, "    else\n");
						fprintf (src, "    {\n");

						if ((tmpTypeId == BASICTYPE_ANY || tmpTypeId == BASICTYPE_ANYDEFINEDBY) && !CountTags (tmpElmt->type))
						{
							/* don't get a tag since ANY's decode their own */
							if (e->type->optional || (tmpElmt->type->optional && tmpElmt != (NamedType *)LAST_LIST_ELMT (seq->basicType->a.sequence)))
							{
								/*
								* let this cause a compile error in the generated code
								*/
								fprintf (src, "  <problems with untagged ANY that is optional or follows an optional sequence element - you must fix this>\n");
							}
							fprintf (src, "      tag1 = _b.PeekByte();\n\n");
							fprintf (src, "      if ((elmtLen0 == INDEFINITE_LEN) && (tag1 == EOC_TAG_ID))\n");
							fprintf (src, "      {\n");
							fprintf (src, "        BDecEoc (_b, seqBytesDecoded);\n\n");
							fprintf (src, "        bytesDecoded += seqBytesDecoded;\n");
							fprintf (src, "        return;\n");
							fprintf (src, "      }\n");
						}
						else if (tmpTypeId == BASICTYPE_EXTENSION)
						{
							fprintf (src, "      tag1 = _b.PeekByte();\n\n");
							fprintf (src, "      if ((elmtLen0 == INDEFINITE_LEN) && (tag1 == EOC_TAG_ID))\n");
							fprintf (src, "      {\n");
							fprintf (src, "        BDecEoc (_b, seqBytesDecoded);\n\n");
							fprintf (src, "        bytesDecoded += seqBytesDecoded;\n");
							fprintf (src, "        return;\n");
							fprintf (src, "      }\n");
						}
						else
						{
							fprintf (src, "      tag1 = BDecTag (_b, seqBytesDecoded);\n\n");
							fprintf (src, "      if ((elmtLen0 == INDEFINITE_LEN) && (tag1 == EOC_TAG_ID))\n");
							fprintf (src, "      {\n");
							fprintf (src, "        BDEC_2ND_EOC_OCTET (_b, seqBytesDecoded);\n");
							fprintf (src, "        bytesDecoded += seqBytesDecoded;\n");
							fprintf (src, "        return;\n");
							fprintf (src, "      }\n");
						}
						fprintf (src, "    }\n");
					}
				}

				/*
				* close tag check if (if there is one) and
				* print else clause to handle missing non-optional elmt
				* errors
				*/
				tmpTypeId = GetBuiltinType (e->type);
				if ((tmpTypeId == BASICTYPE_ANYDEFINEDBY || tmpTypeId == BASICTYPE_ANY) && !CountTags (e->type))
				{
					/* do nothing - no tag check if stmt to close */
					fprintf (src, "\n\n");
				}
				else if (!e->type->optional && !e->type->defaultVal)
				{
					fprintf (src, "  }\n"); /* end of tag check if */
					fprintf (src, "  else\n");
					fprintf (src, "  {\n");

					if(e->type->extensionAddition)
					{
						fprintf (src, "      throw EXCEPT(\"SEQUENCE is missing non-optional expected extension addition elmt\", DECODE_ERROR);\n");
					}
					else
					{
						fprintf (src, "      throw EXCEPT(\"SEQUENCE is missing non-optional root elmt\", DECODE_ERROR);\n");
					}
					fprintf (src, "  }\n\n");
				}
				else
					fprintf (src, "  }\n\n"); /* end of tag check if */

				FreeTags (tags);
			}
			else
			{
				extensionAdditionFound = TRUE;
			}
		}

		/***********************************************************************/
		/***********************************************************************/
		/***********************************************************************/

		/* for last elmt only */
		// fprintf (src, "  bytesDecoded += seqBytesDecoded;\n"); nenene ... das muss unten passieren! wenn auch noch alles gelesen wurde was wir nicht kennen!
		fprintf (src, "  if (elmtLen0 == INDEFINITE_LEN)\n");
		fprintf (src, "  {\n");
		if(extensionAdditionFound)
		{
			fprintf (src, "    // keep grabbing any's into the extension list\n");
			fprintf (src, "    // until EOC is found\n");
			fprintf (src, "    bool b_not_EOC = true;\n");
			fprintf (src, "    while(b_not_EOC)\n");
			fprintf (src, "    {\n");
			fprintf (src, "      tag1 = _b.PeekByte();\n\n");
			fprintf (src, "      if ((elmtLen0 == INDEFINITE_LEN) && (tag1 == EOC_TAG_ID))\n");
			fprintf (src, "      {\n");
			fprintf (src, "        BDecEoc (_b, seqBytesDecoded);\n\n");
			fprintf (src, "        bytesDecoded += seqBytesDecoded;\n");
			fprintf (src, "        b_not_EOC = false;\n");
			fprintf (src, "      }\n");
			fprintf (src, "      else\n");
			fprintf (src, "      {\n");
			fprintf (src, "        AsnAny extAny;\n");
			fprintf (src, "        extAny.BDec(_b, seqBytesDecoded);\n");
			fprintf (src, "        extension.extList.insert(extension.extList.end(), extAny);\n");
			fprintf (src, "      }\n");
			fprintf (src, "    }\n");
		}
		else
		{
			fprintf (src, "    BDecEoc (_b, bytesDecoded);\n");
			fprintf (src, "    return;\n");
		}
		fprintf (src, "  }\n");
		fprintf (src, "  else if (seqBytesDecoded != elmtLen0)\n");
		fprintf (src, "  {\n");

		if(extensionAdditionFound)
		{
			fprintf (src, "    while(seqBytesDecoded < elmtLen0)\n");
            fprintf (src, "    {\n");
            fprintf (src, "       // Grapping Anys until length is reached\n");
			fprintf (src, "       AsnAny extAny;\n");
			fprintf (src, "       extAny.BDec(_b, seqBytesDecoded);\n");
			fprintf (src, "       extension.extList.insert(extension.extList.end(), extAny);\n");
			fprintf (src, "    }\n");
            fprintf (src, "    // don't forget to add theese bytes too ...\n");
            fprintf (src, "    bytesDecoded += seqBytesDecoded;\n");
            fprintf (src, "    return;\n");
        }
		else
		{
			fprintf (src, "    throw EXCEPT(\"Length discrepancy on sequence\", DECODE_ERROR);\n");
		}
		fprintf (src, "  }\n");
        fprintf (src, "  else\n");
        fprintf (src, "  {\n");
        fprintf (src, "    // nothing more to read, just add the bytes read previously\n");
        fprintf (src, "    bytesDecoded += seqBytesDecoded;\n");
        fprintf (src, "  }\n");

	} /* end of non-empty set else clause */

	fprintf (src, "} // %s::B%s\n\n", td->cxxTypeDefInfo->className, r->decodeContentBaseName);
}

void PrintSeqDefCodeBerEnc(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *seq)
{
	char *classStr;
	char *formStr;
	Tag *tag;
	int tagLen;

	fprintf (hdr, "  %s		B%s (%s &_b) const;\n", lenTypeNameG, r->encodeBaseName, bufTypeNameG);
	fprintf (src, "%s %s::B%s (%s &_b) const\n", lenTypeNameG, td->cxxTypeDefInfo->className, r->encodeBaseName,  bufTypeNameG);
	fprintf (src, "{\n");
	fprintf (src, "  %s l=0;\n", lenTypeNameG);
	fprintf (src, "  l = B%s (_b);\n", r->encodeContentBaseName);

	/* encode each tag/len pair if any */
	FOR_EACH_LIST_ELMT_RVS (tag, seq->tags)
	{
		classStr = Class2ClassStr (tag->tclass);
		formStr = Form2FormStr (CONS);  /* seq's are constructed */

		fprintf (src, "  l += BEncConsLen (_b, l);\n");

		if (tag->tclass == UNIV)
		{
			const char* ptr = DetermineCode(tag, &tagLen, 0);
			fprintf (src, "  l += BEncTag%d (_b, %s, %s, %s);\n", tagLen, classStr, formStr, ptr);//RWC;Code2UnivCodeStr (tag->code));
		}
		else
		{
			const char* ptr = DetermineCode(tag, &tagLen, 1);
			fprintf (src, "  l += BEncTag%d (_b, %s, %s, %s);\n", tagLen, classStr, formStr, ptr);//RWC;tag->code);
		}
	}

	fprintf (src, "  return l;\n");
	fprintf (src, "}\n\n");
}

void PrintSeqDefCodeBerDec(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *seq)
{
	char *classStr;
	char *formStr;
	int i=0;
	Tag *tag;
	int elmtLevel=0;

	fprintf (hdr, "  void			B%s (const %s &_b, %s &bytesDecoded);\n", r->decodeBaseName, bufTypeNameG, lenTypeNameG);//, envTypeNameG);
	fprintf (src, "void %s::B%s (const %s &_b, %s &bytesDecoded)\n", td->cxxTypeDefInfo->className, r->decodeBaseName,  bufTypeNameG, lenTypeNameG);//, envTypeNameG);
	fprintf (src, "{\n");
	fprintf (src, "  FUNC(\" %s::B%s\");\n", td->cxxTypeDefInfo->className, r->decodeBaseName);
	fprintf (src, "  %s tag;\n", tagTypeNameG);

	/* print extra locals for redundant lengths */
	for (i = 1; (seq->tags != NULL) && (i <= LIST_COUNT (seq->tags)); i++)
		fprintf (src, "  %s elmtLen%d;\n", lenTypeNameG, i);
	fprintf (src, "\n");

	/*  decode tag/length pair (s) */
	elmtLevel = 0;
	FOR_EACH_LIST_ELMT (tag, seq->tags)
	{
		classStr = Class2ClassStr (tag->tclass);
		formStr = Form2FormStr (CONS);  /* seqs are constructed */

		fprintf (src, "  if ((tag = BDecTag (_b, bytesDecoded)) != ");

		if (tag->tclass == UNIV)
			fprintf (src, "MAKE_TAG_ID (%s, %s, %s))\n", classStr, formStr, DetermineCode(tag, NULL, 0));//RWC;Code2UnivCodeStr (tag->code));
		else
			fprintf (src, "MAKE_TAG_ID (%s, %s, %s))\n", classStr, formStr, DetermineCode(tag, NULL, 1));//RWC;tag->code);
		fprintf (src, "  {\n");
		fprintf (src, "    throw InvalidTagException(typeName(), tag, STACK_ENTRY);\n");
		fprintf (src, "  }\n");

		fprintf (src, "  elmtLen%d = BDecLen (_b, bytesDecoded);\n", ++elmtLevel);
	}

	fprintf (src, "  B%s (_b, tag, elmtLen%d, bytesDecoded);\n", r->decodeContentBaseName, elmtLevel);

	/* grab any EOCs that match redundant, indef lengths */
	for (i = elmtLevel-1; i > 0; i--)
	{
		fprintf (src, "  if (elmtLen%d == INDEFINITE_LEN)\n", i);
		fprintf (src, "    BDecEoc (_b, bytesDecoded);\n");
	}

	fprintf (src, "}\n\n");
}

void PrintSeqDefCodeJsonEnc(FILE *src, FILE *hdr, TypeDef *td, Type *seq)
{
	NamedType *e;
	char *varName;
	CxxTRI *cxxtri=NULL;

	//void           JEnc (EJson::Value &b) const;

	fprintf (hdr, "  void			JEnc (EJson::Value &b) const;\n");
	fprintf (src, "void %s::JEnc (EJson::Value &b) const\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");

	/* print local vars */
	fprintf (src, "  b = EJson::Value(EJson::objectValue);\n\n");
	fprintf (src, "  EJson::Value tmp;\n\n");

	FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
	{
		cxxtri =  e->type->cxxTypeRefInfo;
		varName = cxxtri->fieldName;

		if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
		{
			/* print optional test if nec */
			if (e->type->defaultVal != NULL)
			{
				Value *defVal = GetValue(e->type->defaultVal->value);
				/** PIERCE added DER DEFAULT encoding rules 8-16-2000
				**/

				/* HANDLE DEFAULT VALUE ENCODING FOR DER
				*/
				switch (ParanoidGetBuiltinType(e->type))
				{
				case BASICTYPE_INTEGER:
				case BASICTYPE_ENUMERATED:
					fprintf(src, "  if ( %s(%s) && *%s != %d )\n  {\n",
						cxxtri->optTestRoutineName,
						varName, varName, defVal->basicValue->a.integer);
					break;
				case BASICTYPE_BITSTRING:
				{
					if (defVal->basicValue->choiceId == BASICVALUE_VALUENOTATION)
					{
						char *defBitStr;
						normalizeValue(&defBitStr, defVal->basicValue->a.valueNotation->octs);

						//ste inserted check for default empty
						if (strlen(defBitStr))
						{
							//check for default bit.
							fprintf(src, "  if ( %s(%s) && (! %s->soloBitCheck(%s::%s)) ) \n  {\n",
								cxxtri->optTestRoutineName, varName, varName, cxxtri->className, defBitStr);
						}
						else
						{
							//if default is empty then check for empty
							fprintf(src, "  if ( %s(%s) && (! %s->IsEmpty()) ) \n  {\n",
								cxxtri->optTestRoutineName, varName, varName);

						}

						//RWC;ALLOW "}" alignment using editor...
						free(defBitStr);
					}
					else
						printf("\nWARNING: unsupported use of default BIT STRING\n");
				}
				break;
				case BASICTYPE_BOOLEAN:
					fprintf(src, "  if ( %s(%s) && *%s != %s )\n  {\n",
						cxxtri->optTestRoutineName, varName, varName, defVal->basicValue->a.boolean ? "true" : "false");
					//RWC;ALLOW "}" alignment using editor...
					break;
				default:
					/* TBD print error? */
					break;
				}
			}
			else if (e->type->optional)
			{
				fprintf(src, "  if (%s (%s))\n", cxxtri->optTestRoutineName, varName);
				fprintf(src, "  {\n");
				//RWC;ALLOW "}" alignment using editor...
			}

			/* encode content */
			fprintf(src, "    %s", varName);
			if (cxxtri->isPtr)
				fprintf(src, "->");
			else
				fprintf(src, ".");
			fprintf(src, "JEnc (tmp);\n");
			fprintf(src, "    b[\"%s\"] = tmp;\n", varName);

			/* close optional test if nec */
			if (e->type->optional || (e->type->defaultVal != NULL))
				fprintf(src, "  }\n\n");
			else
				fprintf(src, "\n");
		}
		else
		{
			//external (...)
			fprintf(src, "  //extensions allowed but ignored in json\n");
		}
	}

	fprintf (src, "}\n\n\n");
	//fprintf (src, "} // %s::B%s\n\n\n", td->cxxTypeDefInfo->className, r->encodeContentBaseName);
}

void PrintSeqDefCodeJsonDec(FILE *src, FILE *hdr, TypeDef *td, Type *seq)
{
	NamedType *e;
	char *varName;
	CxxTRI *cxxtri=NULL;

	////bool           JDec (const AsnJSONBuf &b);
	fprintf (hdr, "  bool			JDec (const EJson::Value &b);\n\n");
	fprintf (src, "bool %s::JDec (const EJson::Value &b)", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  Clear();\n");

	fprintf (src, "  if (!b.isObject()) return false;\n\n");
	fprintf (src, "  EJson::Value tmp;\n");

	FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
	{
		cxxtri =  e->type->cxxTypeRefInfo;
		varName = cxxtri->fieldName;

		//do not print code for extensions (...)
		if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
		{
			fprintf(src, "  if (b.isMember(\"%s\")) {\n", varName);
			if (cxxtri->isPtr)
			{
				fprintf(src, "     delete %s;\n", varName);
				fprintf(src, "    %s = new %s;\n", varName, cxxtri->className);
				fprintf(src, "    if (!%s->JDec(b[\"%s\"])) throw InvalidTagException(typeName(), \"decode failed: %s\", STACK_ENTRY);\n", varName, varName, varName);
			}
			else
			{
				fprintf(src, "    if (!%s.JDec(b[\"%s\"])) throw InvalidTagException(typeName(), \"decode failed: %s\", STACK_ENTRY);\n", varName, varName, varName);
			}
			fprintf(src, "  }\n");

			//if not optional, throw not found error
			if (!cxxtri->isPtr)
			{
				fprintf(src, "  else throw InvalidTagException(typeName(), \"not found: %s\", STACK_ENTRY);\n", varName);
			}
		}
		else
		{
			fprintf(src, "  //extensions allowed but ignored in json\n");
		}
	}
	fprintf (src, "  return true;\n");
	fprintf (src, "} // %s::JDec\n\n", td->cxxTypeDefInfo->className);
}

void PrintSeqDefCodePEREnc(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *seq, NamedType **pSeqElementNamedType)
{
	NamedType *e;
	int ii = 0;
	/*** RWC;PRESENT. ***/
	/* FIRST, create array containing all SEQUENCE elements. */
	pSeqElementNamedType = (NamedType **)calloc(
		seq->basicType->a.sequence->count,
		sizeof(NamedType *));
	ii = 0;
	FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
	{
		pSeqElementNamedType[ii++] = e;
	}       /* END FOR each SEQUENCE element. */
	/*RWC;ONLY FOR SET.PrintCxxDefCode_PERSort(&pSetElementNamedType, &pSetElementTag,
	set->basicType->a.set);*/

	/* THIRD, perform actual encoding using SEQUENCE element pointers.
	* FOR_EACH_LIST_ELMT (e, set->basicType->a.set) */
	if (!seq->basicType->a.sequence)
		exit(3);
	PrintCxxDefCode_SetSeqPEREncode (src, hdr, r, td, pSeqElementNamedType,
		seq->basicType->a.sequence->count);
}

void PrintSeqDefCodePERDec(FILE *src, FILE *hdr, CxxRules *r, TypeDef *td, Type *seq, NamedType **pSeqElementNamedType)
{
	PrintCxxDefCode_SetSeqPERDecode (src, hdr, r, td, pSeqElementNamedType,
		seq->basicType->a.sequence->count);
}

static void PrintCxxSeqDefCode(FILE *src, FILE *hdr, ModuleList *mods, Module *m, CxxRules *r ,TypeDef *td, Type *parent, Type *seq, int novolatilefuncs)
{
	NamedType *e;
	CxxTRI *cxxtri=NULL;

	enum BasicTypeChoiceId tmpTypeId;

	// DEFINE PER encode/decode tmp vars.
	NamedType **pSeqElementNamedType=NULL;


	/* put class spec in hdr file */
	char *ptr="";   /* NOT DLL Exported, or ignored on Unix. */
	if (bVDAGlobalDLLExport != NULL)
		ptr = bVDAGlobalDLLExport;

	if (gAlternateNamespaceString == 0 &&
		(strcmp(td->cxxTypeDefInfo->className, "AsnOptionalParam") == 0 ||
		strcmp(td->cxxTypeDefInfo->className, "AsnOptionalParameters") == 0))
		return;


	fprintf (hdr, "class %s %s%s\n", ptr, td->cxxTypeDefInfo->className, baseClassesG);
	fprintf (hdr, "{\n");
	fprintf (hdr, "public:\n");

	/* write out the sequence elmts */
	FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
	{
		fprintf (hdr, "  ");

		/* JKG 7/31/03 */
		/*The following code enclosed in this if/else statement */
		/*is constructed for constraint handling capability     */
		/*for primitives found in sequences or sets             */

		if(e->type->subtypes!=NULL)
		{
			switch(e->type->subtypes->choiceId)
			{
			case SUBTYPE_AND:
			case SUBTYPE_OR:
			case SUBTYPE_SINGLE:
				{
					if(!PrintCxxMultiConstraintOrHandler(hdr, src, td->cxxTypeDefInfo->className, e, 1))
					{
						PrintCxxType (hdr, mods, m, r, td, seq, e->type);
						fprintf (hdr, "%s;\n", e->type->cxxTypeRefInfo->fieldName);
					}
					break;
				}
			default:
				{
					PrintCxxType (hdr, mods, m, r, td, seq, e->type);
					fprintf (hdr, "%s;\n", e->type->cxxTypeRefInfo->fieldName);
					break;
				}
			}
		}
		else
		{
			PrintCxxType (hdr, mods, m, r, td, seq, e->type);
			fprintf (hdr, "%s;\n", e->type->cxxTypeRefInfo->fieldName);
		}
	}
	fprintf (hdr, "\n");
#if META
	if (printMetaG)
	{
		PrintCxxSeqDefCodeMeta_1(hdr, src, td, seq, m, e)
	}
#endif /* META */

	/* constructors and destructor: */
	/* Default constructor
	*/
	fprintf (hdr, "   %s() {Init();}\n", td->cxxTypeDefInfo->className);

	/* Init() member function
	*/
	fprintf (hdr, "   void Init(void);\n");
	fprintf (src, "void %s::Init(void)\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
	{
		if ((e->type->cxxTypeRefInfo->isPtr))
		{

#if TCL
			fprintf (src, "#if TCL\n");
			fprintf (src, "  %s = new %s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className);
			fprintf (src, "#else\n");
			fprintf (src, "  %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "#endif // TCL\n");
#else

			/* Default member initialization is a work in progress for eSNACC 1.6
			*/

			/* initialize default members to their default values */
			if (e->type->defaultVal != NULL)
			{
				Value *defVal = GetValue(e->type->defaultVal->value);

				/* HANDLE DEFAULT VALUE DECODING FOR DER by initializing the respective members to their default
				* values.
				*/
				switch(ParanoidGetBuiltinType(e->type))
				{
				case BASICTYPE_INTEGER:
				case BASICTYPE_ENUMERATED:
					fprintf(src,"  %s = new %s(%d);\n",
						e->type->cxxTypeRefInfo->fieldName,
						e->type->cxxTypeRefInfo->className,
						defVal->basicValue->a.integer);
					break;
				case BASICTYPE_BOOLEAN:
					fprintf(src,"  %s = new %s(%s);\n",
						e->type->cxxTypeRefInfo->fieldName,
						e->type->cxxTypeRefInfo->className,
						defVal->basicValue->a.boolean == 0 ? "false": "true");
					break;
				case BASICTYPE_BITSTRING:
					{
						NamedNumberList *pNNL = GetNamedElmts(e->type);
						BasicValue *pBV;
						pBV = GetLastNamedNumberValue(pNNL);

						if (defVal->basicValue->choiceId == BASICVALUE_VALUENOTATION &&
							pBV != NULL)
						{
							char *defBitStr;
							normalizeValue(&defBitStr, defVal->basicValue->a.valueNotation->octs);

							fprintf(src,"  %s = new %s(%d);\n",
								e->type->cxxTypeRefInfo->fieldName,
								e->type->cxxTypeRefInfo->className,
								pBV->a.integer);

							//ste modified for empty default values
							if (strlen(defBitStr))
								fprintf(src, "  %s->SetBit(%s::%s);\n",
								e->type->cxxTypeRefInfo->fieldName,
								e->type->cxxTypeRefInfo->className,
								defBitStr);
							free(defBitStr);
						}
						else
							printf("\nWARNING: unsupported use of default BIT STRING\n");
					}
					break;
				default:
					{
						fprintf (src, "  %s = new %s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className);
					}

				} /* end switch */
			}
			else
			{
				if(e->type->optional)
				{
					fprintf (src, "  %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
				}
				else
				{
					fprintf (src, "  %s = new %s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className);
				}
			}
			/* end of default member initialization */

#endif /* TCL */
		}
	}
	fprintf (src, "}\n\n");

	/* virtual destructor
	*/
	fprintf (hdr, "   virtual ~%s() {Clear();}\n", td->cxxTypeDefInfo->className);
	/* Clear() member
	*/
	fprintf (hdr, "   void Clear();\n");

	/*JKG: 7/30/03 -- virtual check Constraints method*/

	fprintf(hdr, "\n  int checkConstraints(ConstraintFailList* pConstraintFails) const;\n\n");
	fprintf(src, "\nint %s::checkConstraints(ConstraintFailList* pConstraintFails) const{\n",
		td->cxxTypeDefInfo->className);

	FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
	{
		if (e->type->cxxTypeRefInfo->isPtr)
		{
			fprintf(src, "\tif (%s != NULL)\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf(src, "\t\t%s->checkConstraints(pConstraintFails);\n\n",
				e->type->cxxTypeRefInfo->fieldName);
		}
		else
		{
			fprintf(src, "\t%s.checkConstraints(pConstraintFails);\n\n",
				e->type->cxxTypeRefInfo->fieldName);
		}
	}

	fprintf(src, "\treturn 0;\n");
	fprintf(src, "}\n\n\n");

	fprintf (src, "void %s::Clear()\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
	{
		tmpTypeId = GetBuiltinType (e->type);
		if (e->type->cxxTypeRefInfo->isPtr)
		{
			fprintf (src, "  if(%s)\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "    delete %s;\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "  %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
		}
		else if(!e->type->cxxTypeRefInfo->isPtr &&
			((tmpTypeId == BASICTYPE_CHOICE) ||
			(tmpTypeId == BASICTYPE_SET) ||
			(tmpTypeId == BASICTYPE_SEQUENCE)) )
		{
			fprintf (src, "  %s.Clear();\n", e->type->cxxTypeRefInfo->fieldName);
		}
	}

	fprintf (src, "}\n\n");

	/* PIERCE 8-22-2001 */
	PrintCopyConstructor(hdr, src, td->cxxTypeDefInfo->className);
	PrintSimpleMeta(hdr, td->cxxTypeDefInfo->className,0);

	/* print clone routine for ANY mgmt */
	PrintCloneMethod (hdr, src, td);

	/* print operator = overload */
	fprintf (hdr, "  %s		&operator = (const %s &that);\n", td->cxxTypeDefInfo->className, td->cxxTypeDefInfo->className);
	fprintf (src, "%s &%s::operator = (const %s &that)\n", td->cxxTypeDefInfo->className, td->cxxTypeDefInfo->className, td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  if (this != &that)\n");
	fprintf (src, "  {\n");
	fprintf (src, "    Clear();\n");

	FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
	{
		if (e->type->cxxTypeRefInfo->isPtr)
		{
			fprintf (src, "    if (that.%s)\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "    {\n");
			fprintf (src, "      if (!%s)\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "        %s = new %s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className);
			fprintf (src, "      *%s = *that.%s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "    }\n");
			fprintf (src, "    else\n");
			fprintf (src, "    {\n");
			fprintf (src, "      delete %s;\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "      %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "    }\n");
		}
		else
		{
			fprintf (src, "    %s = that.%s;\n", e->type->cxxTypeRefInfo->fieldName,
				e->type->cxxTypeRefInfo->fieldName);
		}
	}
	fprintf (src, "  }\n");
	fprintf (src, "\n");
	fprintf (src, "  return *this;\n");
	fprintf (src, "}\n\n");

	/* BerEncodeContent method */
	if (printEncodersG)
	{
		PrintSeqDefCodeBerEncodeContent(src, hdr, r, td, seq);
	}
	/* end of BerEncodeContent method printing code */

	/* write BerDecodeContent to src */
	if (printDecodersG)
	{
		PrintSeqDefCodeBerDecodeContent(src, hdr, r, td, seq);
	}
	/* end of code for printing ber decode content method */

	/* do BEnc function */
	if (printEncodersG)
	{
		PrintSeqDefCodeBerEnc(src, hdr, r, td, seq);
	}
	/* end of BEnc function */

	if (printJSONEncDecG)
		PrintSeqDefCodeJsonEnc(src, hdr, td, seq);

	/* Do BDec function */
	if (printDecodersG)
	{
		PrintSeqDefCodeBerDec(src, hdr, r, td, seq);
	}
	/* end of BDec function */

	if (printJSONEncDecG)
		PrintSeqDefCodeJsonDec(src, hdr, td, seq);

	if(genPERCode)
	{
		/* PER Encode, PerEnc */
		if (printEncodersG)     //RWC;TBD; SHOULD set printEncodersP with FLAG!!!!
		{
			PrintSeqDefCodePEREnc(src, hdr, r, td, seq, pSeqElementNamedType);
		}       // END IF printEncodersG
		/* end of PerEncode */

		/* PER Decode, PerDecode */
		if (printDecodersG)
		{
			PrintSeqDefCodePERDec(src, hdr, r, td, seq, pSeqElementNamedType);
		}

	}

	/* end of PerDecode */
	if (pSeqElementNamedType)
		free(pSeqElementNamedType);

	/* write code for printing */
	if (printPrintersG)
	{
		PrintCxxSeqSetPrintFunction(src, hdr, td->cxxTypeDefInfo->className,
			seq->basicType);
	}
	if (printPrintersXMLG)
	{
		/* ##################################################################
		RWC;1/12/00; ADDED XML output capability. */

		fprintf (hdr, "  void		PrintXML (std::ostream &os, const char *lpszTitle=NULL) const;\n");
		fprintf (src,"void %s::PrintXML (std::ostream &os, \n", td->cxxTypeDefInfo->className);
		fprintf (src,"                   const char *lpszTitle) const\n");
		fprintf (src, "{\n");
		fprintf (src, "  if (lpszTitle)\n");
		fprintf (src, "  {\n");
		fprintf (src, "     os << \"<\" << lpszTitle;\n");
		fprintf (src, "     if (typeName() && strlen(typeName()))\n");
		fprintf (src, "     {\n");
		fprintf (src, "        os << \" typeName=\\\"\" << typeName() << \"\\\"\";\n");
		fprintf (src, "     }\n");
		fprintf (src, "  }\n");
		fprintf (src, "  else\n");
		fprintf (src, "  {\n");
		fprintf (src, "     os << \"<NONE\";\n");
		fprintf (src, "  }\n");
		fprintf (src, "  if (typeName() && strlen(typeName()))\n");
		fprintf (src, "  {\n");
		fprintf (src, "     if (typeName() && strlen(typeName()))\n");
		fprintf (src, "     {\n");
		fprintf (src, "        os << \"<\" << typeName();\n");
		fprintf (src, "     }\n");
		fprintf (src, "  }\n");
		fprintf (src, " os << \" type=\\\"SEQUENCE\\\">\" << std::endl;\n");
		FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
		{
			cxxtri =  e->type->cxxTypeRefInfo;

			if (e->type->cxxTypeRefInfo->isPtr)
			{
				fprintf (src, "  if (%s (%s))\n", cxxtri->optTestRoutineName,
					e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "  {\n");
				fprintf (src, "    %s->PrintXML(os",
					e->type->cxxTypeRefInfo->fieldName);

				if (e->fieldName != NULL)
					fprintf (src, ", \"%s\"", e->fieldName);

				fprintf (src, ");\n");
				fprintf (src, "  }\n");
			}
			else
			{
				fprintf (src, "    %s.PrintXML(os", e->type->cxxTypeRefInfo->fieldName);
				if (e->fieldName != NULL)
					fprintf (src, ", \"%s\"", e->fieldName);
				fprintf (src, ");\n");
			}
			fprintf (src, "\n");
		}
		fprintf (src, "  if (lpszTitle)\n");
		fprintf (src, "  {\n");
		fprintf (src, "     os << \"</\" << lpszTitle << \">\" << std::endl;\n");
		fprintf (src, "  }\n");
		fprintf (src, "  else\n");
		fprintf (src, "     if (typeName() && strlen(typeName()))\n");
		fprintf (src, "     {\n");
		fprintf (src, "        os << \"</\" << typeName() << \">\" << std::endl;\n");
		fprintf (src, "     }\n");
		fprintf (src, "} // %s::PrintXML\n\n\n", td->cxxTypeDefInfo->className);
		/* END XML Print capability.
		################################################################## */
	}
	/* end of print method code printer */

	/* close class definition */
	fprintf (hdr, "};\n\n\n");
} /* PrintCxxSeqDefCode */


static void
PrintCxxSetDefCode (FILE *src, FILE *hdr, ModuleList *mods, Module *m,
					CxxRules *r, TypeDef *td, Type *parent, Type *set, int novolatilefuncs)
{
	NamedType *e;
	char *classStr;
	char *formStr;
	char *codeStr;
	int tagLen;
	int i=0;
	Tag *tag;
	TagList *tags;
	char *varName;
	CxxTRI *cxxtri=NULL;
	int elmtLevel=0;
	int varCount, tmpVarCount;
	int stoleChoiceTags;
	int mandatoryElmtCount;
	enum BasicTypeChoiceId tmpTypeId;
	NamedType *defByNamedType;

	// DEFINE PER encode/decode tmp vars.
	int *pSetElementTag=NULL;
	NamedType **pSetElementNamedType=NULL;
	int extensionAdditions = FALSE;

	/* put class spec in hdr file */
	char *ptr="";   /* NOT DLL Exported, or ignored on Unix. */
	if (bVDAGlobalDLLExport != NULL)
		ptr = bVDAGlobalDLLExport;

	fprintf (hdr, "class %s %s%s\n", ptr, td->cxxTypeDefInfo->className, baseClassesG);
	fprintf (hdr, "{\n");
	fprintf (hdr, "public:\n");

	/* write out the set elmts */
	FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
	{
		fprintf (hdr, "  ");

		/* JKG 7/31/03 */
		/*The following code enclosed in this if/else statement */
		/*is constructed for constraint handling capability     */
		/*for primitives found in sequences or sets             */

		if(e->type->subtypes!=NULL)
		{
			switch(e->type->subtypes->choiceId)
			{
			case SUBTYPE_AND:
			case SUBTYPE_OR:
			case SUBTYPE_SINGLE:
				{
					if(!PrintCxxMultiConstraintOrHandler(hdr, src, td->definedName, e, 1))
					{
						PrintCxxType (hdr, mods, m, r, td, set, e->type);
						fprintf (hdr, "%s;\n\n", e->type->cxxTypeRefInfo->fieldName);
					}
					break;
				}
			default:
				{
					PrintCxxType (hdr, mods, m, r, td, set, e->type);
					fprintf (hdr, "%s;\n\n", e->type->cxxTypeRefInfo->fieldName);
					break;
				}
			}
		}
		else
		{
			PrintCxxType (hdr, mods, m, r, td, set, e->type);
			fprintf (hdr, "%s;\n\n", e->type->cxxTypeRefInfo->fieldName);
		}

	}
	fprintf (hdr, "\n");

#if META
	if (printMetaG)
	{
		PrintCxxSetDefCodeMeta_1(hdr, src, td, set, m, e);
	}
#endif /* META */

	/* Default constructor
	*/
	fprintf (hdr, "   %s(){Init();}\n", td->cxxTypeDefInfo->className);

	/* Init() member function
	*/
	fprintf (hdr, "   void Init(void);\n");
	fprintf (src, "void %s::Init(void)\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
	{
		if (e->type->cxxTypeRefInfo->isPtr)
		{
#if TCL
			fprintf (src, "#if TCL\n");
			fprintf (src, "  %s = new %s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className);
			fprintf (src, "#else // TCL\n");
			fprintf (src, "  %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
#else

			if (e->type->defaultVal != NULL)
			{
				Value *defVal = GetValue(e->type->defaultVal->value);

				/* HANDLE DEFAULT VALUE DECODING FOR DER by initializing the respective members to their default
				* values.
				*/
				switch(ParanoidGetBuiltinType(e->type))
				{
				case BASICTYPE_INTEGER:
				case BASICTYPE_ENUMERATED:
					fprintf(src,"  %s = new %s(%d);\n",
						e->type->cxxTypeRefInfo->fieldName,
						e->type->cxxTypeRefInfo->className,
						defVal->basicValue->a.integer);
					break;
				case BASICTYPE_BOOLEAN:
					fprintf(src,"  %s = new %s(%s);\n",
						e->type->cxxTypeRefInfo->fieldName,
						e->type->cxxTypeRefInfo->className,
						defVal->basicValue->a.boolean == 0 ? "false": "true");
					break;
				case BASICTYPE_BITSTRING:
					{
						NamedNumberList *pNNL = GetNamedElmts(e->type);
						BasicValue *pBV;
						pBV = GetLastNamedNumberValue(pNNL);

						if (defVal->basicValue->choiceId == BASICVALUE_VALUENOTATION &&
							pBV != NULL)
						{
							char *defBitStr;
							normalizeValue(&defBitStr, defVal->basicValue->a.valueNotation->octs);

							fprintf(src,"  %s = new %s(%d);\n",
								e->type->cxxTypeRefInfo->fieldName,
								e->type->cxxTypeRefInfo->className,
								pBV->a.integer);

							fprintf(src, "  %s->SetBit(%s::%s);\n",
								e->type->cxxTypeRefInfo->fieldName,
								e->type->cxxTypeRefInfo->className,
								defBitStr);
							free(defBitStr);
						}
						else
							printf("\nWARNING: unsupported use of default BIT STRING\n");
					}
					break;
				default:
					{
						fprintf (src, "  %s = new %s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className);
					}
				} /* end switch */
			}
			else
			{
				if(e->type->optional)
				{
					fprintf (src, "  %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
				}
				else
				{
					fprintf (src, "  %s = new %s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className);
				}
			}

#endif
		}
	}
	fprintf (src, "}\n\n");

	/* PIERCE added 8-22-2001

	Simple META support */
	PrintCopyConstructor(hdr, src, td->cxxTypeDefInfo->className);
	PrintSimpleMeta(hdr, td->cxxTypeDefInfo->className,0);

	/* virtual destructor */
	fprintf (hdr, "  virtual ~%s() {Clear();}\n", td->cxxTypeDefInfo->className);

	/* Clear() member*/
	fprintf (hdr, "  void Clear();\n");
	fprintf (src, "void %s::Clear()\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");

	FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
	{
		tmpTypeId = GetBuiltinType (e->type);

		if(e->type->cxxTypeRefInfo->isPtr){
			fprintf (src, "  if(%s)\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "    delete %s;\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "  %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
		}
		else if(!e->type->cxxTypeRefInfo->isPtr &&
			((tmpTypeId == BASICTYPE_CHOICE) ||
			(tmpTypeId == BASICTYPE_SET) ||
			(tmpTypeId == BASICTYPE_SEQUENCE)) )
		{
			fprintf (src, "  %s.Clear();\n", e->type->cxxTypeRefInfo->fieldName);
		}
	}
	fprintf (src, "}\n\n");

	fprintf(hdr, "\n   virtual int checkConstraints(ConstraintFailList* pConstraintFails) const;\n\n");
	fprintf(src, "\nint %s::checkConstraints(ConstraintFailList* pConstraintFails) const\n{\n",
		td->cxxTypeDefInfo->className);
	FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
	{
		if (e->type->cxxTypeRefInfo->isPtr)
		{
			fprintf(src, "\tif (%s != NULL)\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf(src, "\t\t%s->checkConstraints(pConstraintFails);\n\n",
				e->type->cxxTypeRefInfo->fieldName);
		}
		else
		{
			fprintf(src, "\t\t%s.checkConstraints(pConstraintFails);\n\n",
				e->type->cxxTypeRefInfo->fieldName);
		}
	}

	fprintf(src, "\treturn 0;\n");
	fprintf(src, "}\n\n\n");

	/* print clone routine for ANY mgmt */
	PrintCloneMethod (hdr, src, td);

	fprintf (hdr, "  %s		&operator = (const %s &that);\n", td->cxxTypeDefInfo->className, td->cxxTypeDefInfo->className);
	fprintf (src, "%s &%s::operator = (const %s &that)\n", td->cxxTypeDefInfo->className, td->cxxTypeDefInfo->className, td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  if (this != &that)\n");
	fprintf (src, "  {\n");
	fprintf (src, "    Clear();\n");

	FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
	{
		if (e->type->cxxTypeRefInfo->isPtr)
		{
			fprintf (src, "    if (that.%s)\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "    {\n");
			fprintf (src, "      if (!%s)\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "        %s = new %s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className);
			fprintf (src, "      *%s = *that.%s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "    }\n");
			fprintf (src, "    else\n");
			fprintf (src, "    {\n");
			fprintf (src, "      delete %s;\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "      %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "    }\n");
		}
		else
		{
			fprintf (src, "    %s = that.%s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->fieldName);
		}
	}
	fprintf (src, "  }\n");
	fprintf (src, "\n");
	fprintf (src, "  return *this;\n");
	fprintf (src, "}\n\n");

	/* BerEncode content*/
	if (printEncodersG)
	{
		fprintf (hdr, "  %s		B%s (%s &_b) const;\n", lenTypeNameG, r->encodeContentBaseName, bufTypeNameG);
		fprintf (src, "%s %s::B%s (%s &_b) const\n", lenTypeNameG, td->cxxTypeDefInfo->className, r->encodeContentBaseName, bufTypeNameG);
		fprintf (src, "{\n");

		/* print local vars */
		fprintf (src, "  %s totalLen = 0;\n", lenTypeNameG);
		fprintf (src, "  %s l=0;\n\n", lenTypeNameG);

		/* PIERCE changed 11-05-2002
		*   use an std::list of AsnBuf instead of an array of CSM_Buffers to
		*   sort the elements of the set.
		*/
		fprintf (src, "    std::list<AsnBuf> bufList;\n");
		fprintf (src, "    std::list<AsnBuf>::iterator iBuf;\n");

		FOR_EACH_LIST_ELMT_RVS (e, set->basicType->a.set)
		{
			cxxtri =  e->type->cxxTypeRefInfo;
			varName = cxxtri->fieldName;

			/* print optional test if nec*/
			if (e->type->optional || (e->type->defaultVal != NULL))
			{
				fprintf (src, "  if (%s (%s))\n", cxxtri->optTestRoutineName, varName);
				fprintf (src, "  {\n");
			}

			/* encode content */
			tmpTypeId = GetBuiltinType (e->type);
			if (tmpTypeId == BASICTYPE_ANYDEFINEDBY)
			{
				fprintf(src, "    iBuf = bufList.insert(bufList.begin(), AsnBuf());\n");
				defByNamedType = e->type->basicType->a.anyDefinedBy->link;
				PrintCxxSetTypeByCode(defByNamedType, cxxtri, src);

				fprintf (src, "    l = %s", varName);
				if (cxxtri->isPtr)
					fprintf (src, "->");
				else
					fprintf (src, ".");
				fprintf (src, "B%s (*iBuf);\n", r->encodeBaseName);

			}
			else if (tmpTypeId == BASICTYPE_ANY)
			{
				fprintf(src, "    iBuf = bufList.insert(bufList.begin(), AsnBuf());\n");
				fprintf (src, "    l = %s", varName);
				if (cxxtri->isPtr)
					fprintf (src, "->");
				else
					fprintf (src, ".");

				fprintf (src, "B%s (*iBuf);\n", r->encodeBaseName);
			}
			else if (tmpTypeId == BASICTYPE_BITCONTAINING)
			{
				fprintf(src, "    iBuf = bufList.insert(bufList.begin(), AsnBuf());\n");
				PrintCxxEncodeContaining(e->type, r, src);
			}
			else if ( tmpTypeId == BASICTYPE_EXTENSION )
			{
				fprintf (src, "       l = %s", varName);
				if (cxxtri->isPtr)
					fprintf (src, "->");
				else
					fprintf (src, ".");

				fprintf (src, "B%s (_b);\n", r->encodeBaseName);
			}
			else
			{
				fprintf(src, "    iBuf = bufList.insert(bufList.begin(), AsnBuf());\n");
				fprintf (src, "    l = %s", varName);
				if (cxxtri->isPtr)
					fprintf (src, "->");
				else
					fprintf (src, ".");

				fprintf (src, "B%s (*iBuf);\n", r->encodeContentBaseName);
			}

			/* encode tag (s) & len (s) */

			PrintCxxTagAndLenEncodingCode (src, td, e->type, "l", "(*iBuf)");
			fprintf (src, "    totalLen += l;\n");


			/** NOW, encode for SET DER rule ordering.*/
			/** RWC; Buffers handle files or memory. **/

			/* close optional test if nec */
			if (e->type->optional || (e->type->defaultVal != NULL))
				fprintf (src, "  }\n\n");
			else
				fprintf (src, "\n");
		}
#if 0
		/** LAST, Order for SET DER rule ordering.*/
		/**   re-order all elements, add to "_b".*/
		fprintf (src, "      vdasnacc_sortSet(tmpEnc, iii);\n");

		/** These "SET" components are now ordered in ascending order,
		** ready to be loaded into the output buffer. (RWC; TBD; make output
		** buffers accept these allocated buffers directly, no copy).
		**/

		fprintf (src, "      tmpCount = iii;  /** REMEMBER how many we have**/\n");
		fprintf (src, "      for (iii=0; iii < tmpCount; iii++)\n");
		fprintf (src, "         SM_WriteToAsnBuf(tmpEnc[iii], _b);\n");

		fprintf (src, "      for (iii=0; iii < tmpCount; iii++) delete tmpEnc[iii];\n");
		fprintf (src, "      free(outputBuf.DataPtr());\n");
#endif
		fprintf (src, "    sortSet(bufList);\n");
		fprintf (src, "    iBuf = bufList.begin();\n");
		fprintf (src, "    while(iBuf != bufList.end())\n");
		fprintf (src, "    {\n");
		fprintf (src, "       iBuf->ResetMode();\n");
		fprintf (src, "       _b.splice(*iBuf);\n");
		fprintf (src, "       iBuf++;\n");
		fprintf (src, "    }\n");

		/** internal definition bracket for "tmpCount".**/
		fprintf (src, "  return totalLen;\n");
		fprintf (src, "} // %s::B%s\n\n\n", td->cxxTypeDefInfo->className, r->encodeContentBaseName);
	}
	/* end of BerEncodeContent */

	/* write BerDecodeContent */
	if (printDecodersG)
	{
		fprintf (hdr, "  void			B%s (const %s &_b, %s tag, %s elmtLen, %s &bytesDecoded);\n\n", r->decodeContentBaseName, bufTypeNameG, tagTypeNameG, lenTypeNameG, lenTypeNameG);//, envTypeNameG);

		fprintf (src, "void %s::B%s (const %s &_b, %s /*tag0*/, %s elmtLen0, %s &bytesDecoded)\n", td->cxxTypeDefInfo->className, r->decodeContentBaseName, bufTypeNameG, tagTypeNameG, lenTypeNameG, lenTypeNameG);//, envTypeNameG);
		fprintf (src, "{\n");
		fprintf (src, "   FUNC(\"%s::B%s\");\n",td->cxxTypeDefInfo->className, r->decodeContentBaseName);
		fprintf (src, "   Clear();\n");

		fprintf (src, "    AsnBufLoc readLoc;\n   readLoc = _b.GetReadLoc();\n");

		/* print local vars */
		fprintf (src, "  %s tag1 = %s();\n", tagTypeNameG, tagTypeNameG);
		fprintf (src, "  %s setBytesDecoded = 0;\n", lenTypeNameG);
		fprintf (src, "  unsigned int mandatoryElmtsDecoded = 0;\n");
		/* count max number of extra length var nec */
		varCount = 0;
		FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
		{
			tmpVarCount = CxxCountVariableLevels (e->type);
			if (tmpVarCount > varCount)
				varCount = tmpVarCount;
		}
		/* write extra length vars */
		for (i = 1; i <= varCount; i++)
			fprintf (src, "  %s elmtLen%d;\n", lenTypeNameG, i);
		fprintf (src, "\n");

		/* handle empty set */
		if ((set->basicType->a.set == NULL) || LIST_EMPTY (set->basicType->a.set))
		{
			fprintf (src, "  if (elmtLen0 == INDEFINITE_LEN)\n");
			fprintf (src, "    BDecEoc (_b, bytesDecoded);\n");
			fprintf (src, "  else if (elmtLen0 != 0)\n");
			fprintf (src, "  {\n");

			fprintf (src, "    throw EXCEPT(\"Expected an empty sequence\", DECODE_ERROR);\n");
			fprintf (src, "  }\n");
		}
		else
		{
			fprintf (src, "  for (; (setBytesDecoded < elmtLen0) || (elmtLen0 == INDEFINITE_LEN); )\n");
			fprintf (src, "  {\n");
			fprintf (src, "    readLoc = _b.GetReadLoc();\n");
			fprintf (src, "    tag1 = BDecTag (_b, setBytesDecoded);\n\n");

			fprintf (src, "    if ((elmtLen0 == INDEFINITE_LEN) && (tag1 == EOC_TAG_ID))\n");
			fprintf (src, "    {\n");
			fprintf (src, "      BDEC_2ND_EOC_OCTET (_b, setBytesDecoded);\n");
			fprintf (src, "      break; /* exit for loop */\n");
			fprintf (src, "    }\n");

			fprintf (src, "    elmtLen1 = BDecLen (_b, setBytesDecoded);\n");
			fprintf (src, "    switch (tag1)\n");
			fprintf (src, "    {\n");
			mandatoryElmtCount = 0;
			FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
			{
				if( e->type->basicType->choiceId != BASICTYPE_EXTENSION )
				{
					cxxtri =  e->type->cxxTypeRefInfo;
					tags = GetTags (e->type, &stoleChoiceTags);

					if (LIST_EMPTY (tags))
					{
						fprintf (src, "      // ANY Type ?\n");
						fprintf (src, "      case MAKE_TAG_ID (?, ?, ?):\n");
					}
					else
					{
						tag = (Tag*)FIRST_LIST_ELMT (tags);
						classStr = Class2ClassStr (tag->tclass);
						formStr = Form2FormStr (tag->form);

						if (tag->tclass == UNIV)
							codeStr = DetermineCode(tag, NULL, 0);//RWC;Code2UnivCodeStr (tag->code);
						else
							codeStr = DetermineCode(tag, NULL, 1);
						if (tag->form == ANY_FORM)
						{
							fprintf (src, "      case MAKE_TAG_ID (%s, %s, %s):\n", classStr, Form2FormStr (PRIM), codeStr);
							fprintf (src, "      case MAKE_TAG_ID (%s, %s, %s):\n", classStr, Form2FormStr (CONS), codeStr);
						}
						else
							fprintf (src, "      case MAKE_TAG_ID (%s, %s, %s):\n", classStr, formStr, codeStr);

						/* now decode extra tags/length pairs */
						AsnListFirst (tags);
						AsnListNext (tags);
						elmtLevel = 1;
						if (stoleChoiceTags)
						{
							FOR_REST_LIST_ELMT (tag, tags)
							{
								classStr = Class2ClassStr (tag->tclass);
								formStr = Form2FormStr (tag->form);

								if (tag->tclass == UNIV)
									codeStr = DetermineCode(tag, NULL, 0);//RWC;Code2UnivCodeStr (tag->code);
								else
									codeStr = DetermineCode(tag, NULL, 1);
								if (tag->form == ANY_FORM)
								{
									fprintf (src, "      case MAKE_TAG_ID (%s, %s, %s):\n", classStr, Form2FormStr (PRIM), codeStr);
									fprintf (src, "      case MAKE_TAG_ID (%s, %s, %s):\n", classStr, Form2FormStr (CONS), codeStr);
								}
								else
									fprintf (src, "      case MAKE_TAG_ID (%s, %s, %s):\n", classStr, formStr, codeStr);
							}
						}
						else /* didn't steal nested choice's tags */
						{
							FOR_REST_LIST_ELMT (tag, tags)
							{
								classStr = Class2ClassStr (tag->tclass);
								codeStr = DetermineCode(tag, NULL, 0);//RWC;Code2UnivCodeStr (tag->code);
								formStr = Form2FormStr (tag->form);

								fprintf (src, "        tag1 = BDecTag (_b, setBytesDecoded);\n");
								if (tag->form == ANY_FORM)
								{
									if (tag->tclass == UNIV)
									{
										fprintf (src, "        if ((tag1 != MAKE_TAG_ID (%s, %s, %s))\n",  classStr, Form2FormStr (PRIM), codeStr);
										fprintf (src, "           && (tag1 != MAKE_TAG_ID (%s, %s, %s)))\n", classStr, Form2FormStr (CONS), codeStr);
									}
									else
									{
										fprintf (src, "        if ((tag1 != MAKE_TAG_ID (%s, %s, %s))\n", classStr, Form2FormStr (PRIM), DetermineCode(tag, NULL, 1));//RWC;tag->code);
										fprintf (src, "           && (tag1 != MAKE_TAG_ID (%s, %s, %s)))\n", classStr, Form2FormStr (CONS), DetermineCode(tag, NULL, 1));//RWC;tag->code);
									}
								}
								else
								{
									if (tag->tclass == UNIV)
										fprintf (src, "        if (tag1 != MAKE_TAG_ID (%s, %s, %s))\n", classStr, formStr, codeStr);
									else
										fprintf (src, "        if (tag1 != MAKE_TAG_ID (%s, %s, %s))\n", classStr, formStr, DetermineCode(tag, NULL, 1));//RWC;tag->code);
								}

								fprintf (src, "        {\n");
								fprintf (src, "           throw InvalidTagException(typeName(), tag1, STACK_ENTRY);\n");
								fprintf (src, "        }\n\n");
								fprintf (src, "        elmtLen%d = BDecLen (_b, setBytesDecoded);\n", ++elmtLevel);
							}
						}
					}
					/*
					* if the choices element is another choice &&
					* we didn't steal its tags then we must grab
					* the key tag out of the contained CHOICE
					*/
					if (!stoleChoiceTags && (GetBuiltinType (e->type) == BASICTYPE_CHOICE))
					{
						fprintf (src, "        tag1 = BDecTag (_b, setBytesDecoded);\n");
						fprintf (src, "        elmtLen%d = BDecLen (_b, setBytesDecoded);\n", ++elmtLevel);
					}

					varName = cxxtri->fieldName;

					/* decode content */
					if (cxxtri->isPtr)
					{
						if (e->type->defaultVal)
						{
							fprintf(src, "  // delete default value\n");
							fprintf(src, "  delete %s;\n", varName);
						}
						fprintf (src, "\t\t%s = new %s;\n", varName, cxxtri->className);
					}

					/* decode content */
					tmpTypeId = GetBuiltinType (e->type);
					/*fprintf (src, "    tag1 = BDecTag (_b, setBytesDecoded);\n\n");*/
					/*fprintf (src, "    elmtLen1 = BDecLen (_b, setBytesDecoded);\n");*/
					if (tmpTypeId == BASICTYPE_ANYDEFINEDBY)
					{
						/*
						* must check for another EOC for ANYs
						* since the any decode routines decode
						* their own first tag/len pair
						*/
						elmtLevel++;

						fprintf (src, "        %s", varName);
						if (cxxtri->isPtr)
							fprintf (src, "->");
						else
							fprintf (src, ".");

						defByNamedType = e->type->basicType->a.anyDefinedBy->link;
						PrintCxxSetTypeByCode(defByNamedType, cxxtri, src);

						fprintf (src, "        %s", varName);
						if (cxxtri->isPtr)
							fprintf (src, "->");
						else
							fprintf (src, ".");
						fprintf (src, "B%s (_b, setBytesDecoded);\n",  r->decodeBaseName);
					}
					else if (tmpTypeId == BASICTYPE_ANY)
					{
						elmtLevel++;

						fprintf (src, "        %s", varName);
						if (cxxtri->isPtr)
							fprintf (src, "->");
						else
							fprintf (src, ".");
						fprintf (src, "B%s (_b, setBytesDecoded);\n",  r->decodeBaseName);
					}
					else if (tmpTypeId == BASICTYPE_BITCONTAINING)
					{
						PrintCxxDecodeContaining(e->type, r, src);
					}
					else
					{
						fprintf (src, "        %s", varName);
						if (cxxtri->isPtr)
							fprintf (src, "->");
						else
							fprintf (src, ".");

						fprintf (src, "B%s (_b, tag1, elmtLen%d, setBytesDecoded);\n",  r->decodeContentBaseName, elmtLevel);
					}

					/* decode Eoc (s) */
					for (i = elmtLevel-1; i >= 1; i--)
					{
						fprintf (src, "          if (elmtLen%d == INDEFINITE_LEN)\n", i);
						fprintf (src, "            BDecEoc (_b, setBytesDecoded);\n\n");
					}

					/* keep track of decoded non-optional elmts */
					if ( (!e->type->optional) && (e->type->defaultVal == NULL) && (!e->type->extensionAddition) )
					{
						mandatoryElmtCount++;
						fprintf (src, "          mandatoryElmtsDecoded++;\n");
					}
					fprintf (src, "          break;\n\n");

					FreeTags (tags);
				}
				else
				{
					extensionAdditions = TRUE;
				}
			} /* for each elmt */
			fprintf (src, "       default:\n");
			if(extensionAdditions)
			{
				fprintf (src, "       {\n");
				fprintf (src, "         _b.SetReadLoc(readLoc);\n");
				fprintf (src, "         AsnAny extAny;\n");
				fprintf (src, "         extAny.BDec(_b, setBytesDecoded);\n");
				fprintf (src, "         extension.extList.insert(extension.extList.end(), extAny);\n");
				fprintf (src, "       }\n");
			}
			else
			{
				fprintf (src, "         throw InvalidTagException(typeName(), tag1, STACK_ENTRY);\n");
			}

			fprintf (src, "    } // end switch\n");
			fprintf (src, "  } // end for loop\n");
			fprintf (src, "  bytesDecoded += setBytesDecoded;\n");
			fprintf (src, "  if (mandatoryElmtsDecoded < %d)\n", mandatoryElmtCount);
			fprintf (src, "  {\n");

			fprintf (src, "     throw EXCEPT(\"non-optional SET element missing\", DECODE_ERROR);\n");
			fprintf (src, "  }\n");
		} /* if not empty set clause */

		fprintf (src, "} // %s::B%s\n\n", td->cxxTypeDefInfo->className, r->decodeContentBaseName);
	}
	/* end of decode content method code */
	if(genPERCode && set->basicType->a.set)
	{
		/* PerEncode */
		if (printEncodersG)     //RWC;TBD; SHOULD set printEncodersP with FLAG!!!!
		{
			PrintCxxDefCode_PERSort(&pSetElementNamedType, &pSetElementTag,
				set->basicType->a.set);
			//
			// THIRD, perform actual encoding using re-arranged Set element
			//    pointers.
			//FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
			PrintCxxDefCode_SetSeqPEREncode (src, hdr, r, td, pSetElementNamedType,
				set->basicType->a.set->count);
		}       // END IF printEncodersG
		/* end of PerEncode */

		/* PerDecode */
		if (printDecodersG)
		{
			PrintCxxDefCode_SetSeqPERDecode (src, hdr, r, td,
				pSetElementNamedType, set->basicType->a.set->count);
		}
	}

	/* end of PerDecode */
	if (pSetElementTag)
		free(pSetElementTag);
	if (pSetElementNamedType)
		free(pSetElementNamedType);

	/* BerEncode */
	if (printEncodersG)
	{
		fprintf (hdr, "  %s		B%s (%s &_b) const;\n", lenTypeNameG, r->encodeBaseName, bufTypeNameG);
		fprintf (src, "%s\n", lenTypeNameG);
		fprintf (src, "%s::B%s (%s &_b) const\n", td->cxxTypeDefInfo->className, r->encodeBaseName, bufTypeNameG);
		fprintf (src, "{\n");
		fprintf (src, "  %s l=0;\n", lenTypeNameG);
		fprintf (src, "  l = B%s (_b);\n", r->encodeContentBaseName);

		/* encode each tag/len pair if any */
		FOR_EACH_LIST_ELMT_RVS (tag, set->tags)
		{
			classStr = Class2ClassStr (tag->tclass);
			formStr = Form2FormStr (CONS);  /* set's are constructed */
			//RWC;tagLen = TagByteLen (tag->code);

			fprintf (src, "  l += BEncConsLen (_b, l);\n");

			if (tag->tclass == UNIV)
				fprintf (src, "  l += BEncTag%d (_b, %s, %s, %s);\n", tagLen, classStr, formStr, DetermineCode(tag, &tagLen, 0));//RWC;Code2UnivCodeStr (tag->code));
			else
				fprintf (src, "  l += BEncTag%d (_b, %s, %s, %s);\n", tagLen, classStr, formStr, DetermineCode(tag, &tagLen, 1));//RWC;tag->code);
		}
		fprintf (src, "  return l;\n");
		fprintf (src, "}\n\n");
	}
	/* end of BerEncode */

	/* BerDecode */
	if (printDecodersG)
	{
		fprintf (hdr, "  void			B%s (const %s &_b, %s &bytesDecoded);\n", r->decodeBaseName, bufTypeNameG, lenTypeNameG);//, envTypeNameG);
		fprintf (src, "void %s::B%s (const %s &_b, %s &bytesDecoded)\n", td->cxxTypeDefInfo->className, r->decodeBaseName, bufTypeNameG, lenTypeNameG);//, envTypeNameG);
		fprintf (src, "{\n");
		fprintf (src, "   FUNC(\"%s::B%s\");\n", td->cxxTypeDefInfo->className, r->decodeBaseName);
		fprintf (src, "   %s tag;\n", tagTypeNameG);

		/* print extra locals for redundant lengths */
		for (i = 1; (set->tags != NULL) && (i <= LIST_COUNT (set->tags)); i++)
			fprintf (src, "  %s elmtLen%d;\n", lenTypeNameG, i);
		fprintf (src, "\n");

		/*  decode tag/length pair (s) */
		elmtLevel = 0;
		FOR_EACH_LIST_ELMT (tag, set->tags)
		{
			classStr = Class2ClassStr (tag->tclass);
			formStr = Form2FormStr (CONS);  /* sets are constructed */

			fprintf (src, "  if ((tag = BDecTag (_b, bytesDecoded)) != ");

			if (tag->tclass == UNIV)
				fprintf (src, "MAKE_TAG_ID (%s, %s, %s))\n", classStr, formStr, DetermineCode(tag, NULL, 0));//RWC;Code2UnivCodeStr (tag->code));
			else
				fprintf (src, "MAKE_TAG_ID (%s, %s, %s))\n", classStr, formStr, DetermineCode(tag, NULL, 1));//RWC;tag->code);
			fprintf (src, "  {\n");
			fprintf (src, "        throw InvalidTagException(typeName(), tag, STACK_ENTRY);\n");
			fprintf (src, "  }\n");

			fprintf (src, "  elmtLen%d = BDecLen (_b, bytesDecoded);\n", ++elmtLevel);
		}

		fprintf (src, "  B%s (_b, tag, elmtLen%d, bytesDecoded);\n", r->decodeContentBaseName, i-1);

		/* grab any EOCs that match redundant, indef lengths */
		for (i = elmtLevel-1; i > 0; i--)
		{
			fprintf (src, "  if (elmtLen%d == INDEFINITE_LEN)\n", i);
			fprintf (src, "    BDecEoc (_b, bytesDecoded);\n");
		}

		fprintf (src, "}\n\n");
	}
	/* end of BerDecode */

	/* write code for printing */
	if (printPrintersG)
	{
		PrintCxxSeqSetPrintFunction(src, hdr, td->cxxTypeDefInfo->className,
			set->basicType);
	}
	if (printPrintersXMLG)
	{
		/* ##################################################################
		RWC;1/12/00; ADDED XML output capability. */
		fprintf (hdr, "  void PrintXML (std::ostream &os, const char *lpszTitle=NULL) const;\n");
		fprintf (src, "void %s::PrintXML (std::ostream &os, const char *lpszTitle) const\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");

		fprintf (src, "  if (lpszTitle)\n");
		fprintf (src, "  {\n");
		fprintf (src, "     os << \"<\" << lpszTitle;\n");
		fprintf (src, "     os << \" typeName=\\\"%s\\\" type=\\\"SET\\\">\" << std::endl;\n", td->cxxTypeDefInfo->className);
		fprintf (src, "  }\n");
		fprintf (src, "  else\n");
		fprintf (src, "     os << \"<%s type=\\\"SET\\\">\" << std::endl;\n", td->cxxTypeDefInfo->className);
		FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
		{
			if (e->type->cxxTypeRefInfo->isPtr)
				fprintf (src, "  if (%s (%s))\n", cxxtri->optTestRoutineName, e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "  {\n");
			if (e->type->cxxTypeRefInfo->isPtr)
			{
				fprintf (src, "    (*%s).PrintXML(os", e->type->cxxTypeRefInfo->fieldName);
				if (e->fieldName != NULL)
					fprintf (src, ", \"%s\"", e->fieldName);
				fprintf (src, ");\n");
			}
			else
			{
				fprintf (src, "    %s.PrintXML(os", e->type->cxxTypeRefInfo->fieldName);
				if (e->fieldName != NULL)
					fprintf (src, ", \"%s\"", e->fieldName);
				fprintf (src, ");\n");
			}
			fprintf (src, "  }\n");
			if (e->type->cxxTypeRefInfo->isPtr)
			{
				fprintf (src, "  else\n");
				if (e->fieldName)
					fprintf (src, "        os << \"<%s -- void2 -- />\" << std::endl;\n", e->fieldName);
				else
					fprintf (src, "        os << \"<%s -- void2 -- />\" << std::endl;\n", e->type->cxxTypeRefInfo->fieldName);
			}
			fprintf (src, "\n");

		}      /* END For each set element */
		fprintf (src, "  if (lpszTitle)\n");
		fprintf (src, "     os << \"</\" << lpszTitle << \">\" << std::endl;\n");
		fprintf (src, "  else\n");
		fprintf (src, "     os << \"</%s>\" << std::endl;\n", td->cxxTypeDefInfo->className);
		fprintf (src, "} // %s::PrintXML\n\n\n", td->cxxTypeDefInfo->className);
		/* END XML Print capability.
		################################################################## */
	}
	/* end of print method code */

	/* close class definition */
	fprintf (hdr, "};\n\n\n");
} /* PrintCxxSetDefCode */


/*
* PIERCE added 8-21-2001 template code to handle SET/SEQ OF
*
*/
static void
PrintCxxListClass (FILE *hdr, FILE * src, TypeDef *td, Type *lst, Module* m, ModuleList *mods)
{
	struct NamedType p_etemp;
	NamedType* p_e;
	char typeNameStr[256];
	char *lcn; /* list class name */
	char *ecn; /* (list) elmt class name */
	const char* exportStr;
	char *pszNamespace;
	char pcvarname[100];
	p_e=&p_etemp;
	p_e->type=lst->basicType->a.setOf;

	// Set DLL export string
	if (bVDAGlobalDLLExport == NULL)
		exportStr = "";
	else
		exportStr = bVDAGlobalDLLExport;

	ecn = lst->basicType->a.setOf->cxxTypeRefInfo->className;

	if(p_e->type->subtypes!=NULL)
	{
		sprintf_s(pcvarname, 100, "PERGen_%s%ld", ecn, lconstraintvar);
		lconstraintvar+=1;
		p_e->type->cxxTypeRefInfo->fieldName=&pcvarname[0];

		switch (p_e->type->subtypes->choiceId)
		{
		case SUBTYPE_AND:
		case SUBTYPE_OR:
		case SUBTYPE_SINGLE:
			{
				if(PrintCxxMultiConstraintOrHandler(hdr, src, td->definedName, p_e, 0))
				{
					constraints_flag=1;
				}
				break;
			}
		default:
			{
				break;
			}
		}
	}


	ecn=lst->basicType->a.setOf->cxxTypeRefInfo->className;
	pszNamespace = LookupNamespace(lst, mods);
	lcn = td->cxxTypeDefInfo->className;

	fprintf (hdr, "class %s : public ", lcn);

	switch (lst->basicType->choiceId)
	{
	case BASICTYPE_SEQUENCEOF:
		sprintf_s(typeNameStr, 256, "\"%s\"", lcn);
		if (pszNamespace)
			fprintf (hdr, "AsnSeqOf<%s::%s>\n", pszNamespace, ecn);
		else
			fprintf (hdr, "AsnSeqOf<%s>\n", ecn);
		break;
	case BASICTYPE_SETOF:
		sprintf_s(typeNameStr, 256, "\"%s\"", lcn);
		if (pszNamespace)
			fprintf (hdr, "AsnSetOf<%s::%s>\n", pszNamespace, ecn);
		else
			fprintf (hdr, "AsnSetOf<%s>\n", ecn);
		break;

	default:
		/* TBD print error? */
		break;
	}
	fprintf (hdr, "{\n");

	// Print typeName() and Clone() functions
	fprintf(hdr, "public:\n");
	fprintf(hdr, "  %s const char* typeName() const\t{ return \"%s\"; }\n",
		exportStr, lcn);
	fprintf(hdr, "  %s AsnType* Clone() const\t\t\t{ return new %s(*this); }\n",
		exportStr, lcn);

	// JKG--added functionality for sequence of and set of constraints
	if (td->type->subtypes != NULL)
	{
		/*Subtype* s_type;*/
		/*s_type = (Subtype*)td->type->subtypes->a.single->a.sizeConstraint->a.or->last->data;*/
		/* Only single size constraints that are themselves single are supported */
		if ((td->type->subtypes->choiceId == SUBTYPE_SINGLE) &&
			(td->type->subtypes->a.single->choiceId == SUBTYPEVALUE_SIZECONSTRAINT) &&
			(td->type->subtypes->a.single->a.sizeConstraint->choiceId == SUBTYPE_SINGLE))
		{
			PrintCxxSetOfSizeConstraint(hdr,
				td->type->subtypes->a.single->a.sizeConstraint->a.single,
				m, td->type);
		}
		else
		{
			PrintErrLoc(m->asn1SrcFileName, (long)td->type->lineNo);
			fprintf(errFileG, "ERROR - unsupported constraint\n");
		}
	}

	fprintf(hdr, "};\n\n");
}

static void PrintCxxSetOfDefCode(FILE *src,
								 FILE *hdr,
								 ModuleList *mods,
								 Module *m,
								 CxxRules *r,
								 TypeDef *td,
								 Type *parent,
								 Type *setOf,
								 int novolatilefuncs)
{
	if (gAlternateNamespaceString == 0 && strcmp(td->cxxTypeDefInfo->className, "AsnOptionalParameters") == 0)
		return;

	/* do class */
	PrintCxxListClass (hdr, src, td, setOf, m, mods);

} /* PrintCxxSetOfDefCode */


static void PrintCxxAnyDefCode(FILE *src, FILE *hdr, ModuleList *mods, Module *m, CxxRules *r, TypeDef *td, Type *parent, Type *any)
{
	fprintf (hdr, "/* ");
	SpecialPrintType (hdr, td, td->type);
	fprintf (hdr, " */\n");
	//fprintf (hdr, "typedef %s %s;\n\n", td->type->cxxTypeRefInfo->className, td->cxxTypeDefInfo->className);
	fprintf (hdr, "class %s : public %s {};\n\n", td->cxxTypeDefInfo->className, td->type->cxxTypeRefInfo->className);
} /* PrintCxxAnyDefCode */


static void PrintCxxTypeDefCode(FILE *src, FILE *hdr, ModuleList *mods, Module *m, CxxRules *r, TypeDef *td, int novolatilefuncs)
{
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
	case BASICTYPE_BITSTRING:  /* library type */
	case BASICTYPE_ENUMERATED:  /* library type */
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
		PrintCxxSimpleDef (hdr, src, m, r, td);
		break;
	case BASICTYPE_SEQUENCEOF:  /* list types */
	case BASICTYPE_SETOF:
		PrintCxxSetOfDefCode (src, hdr, mods, m, r, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_IMPORTTYPEREF:  /* type references */
	case BASICTYPE_LOCALTYPEREF:
		/*
		* if this type has been re-tagged then
		* must create new class instead of using a typedef
		*/
		PrintCxxSimpleDef (hdr, src, m, r, td);
		break;
	case BASICTYPE_ANYDEFINEDBY:  /* ANY types */
	case BASICTYPE_ANY:
		PrintCxxAnyDefCode (src, hdr, mods, m, r, td, NULL, td->type);
		break;
	case BASICTYPE_CHOICE:
		PrintCxxChoiceDefCode (src, hdr, mods, m, r, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_SET:
		PrintCxxSetDefCode (src, hdr, mods, m, r, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_SEQUENCE:
		PrintCxxSeqDefCode (src, hdr, mods, m, r, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_COMPONENTSOF:
	case BASICTYPE_SELECTION:
	case BASICTYPE_UNKNOWN:
	case BASICTYPE_MACRODEF:
	case BASICTYPE_MACROTYPE:
		/* do nothing */
		break;
	default:
		/* TBD: print error? */
		break;
	}
} /* PrintCxxTypeDefCode */

int HasROSEOperations(Module *m)
{
	ValueDef *vd;
	int iHasOperations = 0;
	//check for existing operation defines....
	FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (IsROSEValueDef(m, vd)) {
			iHasOperations = 1;
			break;
		}
	}

	return iHasOperations;
}

void PrintROSENamespaceOpenCode(FILE *hfile, Module *m)
{
	/* 7-09-2001 Pierce Leonberger
	*   Added code to include all SNACC generated code in the SNACC namespace.
	*   If the namespace name was overridden with the '-ns' switch then
	*   use the name specified.  If the '-nons' switch was used then don't
	*   use namespaces for SNACC generated code.
	*/
	if (gNO_NAMESPACE == 0)
	{
		fprintf(hfile,"#ifndef NO_NAMESPACE\n");
		if (gAlternateNamespaceString)
		{
			fprintf(hfile,"namespace %s {\n", gAlternateNamespaceString);
			fprintf(hfile, "using namespace SNACC;\n");
		}
		else if (m->namespaceToUse)
		{            // PRINT namespace designated in .asn1 file for this module.
			fprintf(hfile,"namespace %s {\n", m->namespaceToUse);
			fprintf(hfile, "using namespace SNACC;\n");
		}
		else
		{
			fprintf(hfile,"namespace SNACC {\n");
		}
		fprintf(hfile,"#endif\n");
	}
}

void PrintROSENamespaceCloseCode(FILE *hfile)
{
	/* 7-09-2001 Pierce Leonberger
	*/
	if (gNO_NAMESPACE == 0)
	{
		fprintf(hfile, "#ifndef NO_NAMESPACE\n");
		fprintf(hfile, "} // namespace close\n");
		fprintf(hfile, "#endif\n");
	}
}

void PrintForwardDeclarationsCode(FILE* hdrForwardDecl, ModuleList *mods, Module *m)
{
	Module *currMod;
	AsnListNode *currModTmp;

	PrintHdrComment (hdrForwardDecl, m);
	PrintConditionalIncludeOpen (hdrForwardDecl, m->ROSEHdrForwardDeclFileName);
	fprintf (hdrForwardDecl, "\n");

	// print Forward Declaration includes Imported files
	FOR_EACH_LIST_ELMT (currMod, mods)
	{
		if ((strcmp(m->cxxHdrFileName, currMod->cxxHdrFileName) == 0))
		{
			// Code to see the import module list AND load possible "namespace" refs.
			ImportModuleList *ModLists;
			ImportModule *impMod;
			char *ImpFile = NULL;
			ModLists = currMod->imports;
			currModTmp = mods->curr;    //RWC;
			FOR_EACH_LIST_ELMT(impMod, ModLists)
			{
				ImpFile = GetImportForwardDeclFileName (impMod->modId->name, mods);
				if (ImpFile != NULL)
					fprintf (hdrForwardDecl, "#include \"%s\"\n", RemovePath(ImpFile));
			}
			mods->curr = currModTmp;    // RWC;RESET loop control
		}
	}

	PrintROSENamespaceOpenCode(hdrForwardDecl, m);

	//Print only the class forward declarations, that are used in the ROSE Methods
	PrintAllForwardDeclarations(hdrForwardDecl, m);

	fprintf (hdrForwardDecl, "\n");

	PrintROSENamespaceCloseCode(hdrForwardDecl);
	PrintConditionalIncludeClose (hdrForwardDecl, m->ROSEHdrForwardDeclFileName);

}

void PrintROSECode(FILE *src, FILE *hdr, FILE* hdrInterface, ModuleList *mods, Module *m, CxxRules *r)
{
	ValueDef *vd;
	int iFirstIIDFound = 0;

	PrintSrcComment (src, m);
	PrintHdrComment (hdr, m);
	PrintHdrComment (hdrInterface, m);

	if (genCodeCPPPrintStdAfxInclude) {
		fprintf (src, "#include \"stdafx.h\"\n");
	}

	PrintConditionalIncludeOpen (hdr, m->ROSEHdrFileName);
	PrintConditionalIncludeOpen (hdrInterface, m->ROSEHdrInterfaceFileName);

	fprintf (hdr, "#include <cpp-lib/include/SnaccROSEInterfaces.h>\n");

	fprintf (hdrInterface, "#include <cpp-lib/include/SnaccROSEInterfaces.h>\n");
	fprintf (hdrInterface, "#include \"%s\"\n", RemovePath(m->ROSEHdrForwardDeclFileName));

	fprintf (src, "\n");    //RWC; PRINT before possible "namespace" designations.

	PrintSrcIncludes (src, mods, m);
	fprintf (src, "#include \"%s\"\n", RemovePath(m->ROSEHdrFileName));
	fprintf (src, "#include \"%s\"\n", RemovePath(m->ROSEHdrInterfaceFileName));
	fprintf (src, "#include <cpp-lib/include/SnaccROSEBase.h>\n");
	fprintf (src, "#include <cpp-lib/include/SNACCROSE.h>\n");

	fprintf (hdr, "#include \"%s\"\n", RemovePath(m->ROSEHdrForwardDeclFileName));
	/*
	FOR_EACH_LIST_ELMT (currMod, mods)
	{
		if ((strcmp(m->cxxHdrFileName, currMod->cxxHdrFileName) == 0))
		{
			// Code to see the import module list AND load possible "namespace" refs.
			ImportModuleList *ModLists;
			ImportModule *impMod;
			char *ImpFile = NULL;
			ModLists = currMod->imports;
			currModTmp = mods->curr;    //RWC;
			FOR_EACH_LIST_ELMT(impMod, ModLists)
			{
				ImpFile = GetImportFileName (impMod->modId->name, mods);
				if (ImpFile != NULL)
					fprintf (hdr, "#include \"%s\"\n", ImpFile);
				if (impMod->moduleRef == NULL)  // RWC; attempt to update...
					impMod->moduleRef = GetImportModuleRef(impMod->modId->name, mods);
				if (impMod->moduleRef &&
					impMod->moduleRef->namespaceToUse)
				{
					fprintf(src,"using namespace %s;\n", impMod->moduleRef->namespaceToUse);
				}
			}
			mods->curr = currModTmp;    // RWC;RESET loop control
		}
		// Don't duplicate header file referenced in source
		//		if ((strcmp(m->cxxHdrFileName, currMod->cxxHdrFileName) != 0))
		//		{
		//			if ((ImportedFilesG == FALSE) || (currMod->ImportedFlag == TRUE))
		//				fprintf (hdr, "#include \"%s\"\n", currMod->cxxHdrFileName);
		//		}
	}
	*/

	fprintf (hdr, "\n");
	fprintf (hdrInterface, "\n");
	fprintf (src, "\n");

	PrintROSENamespaceOpenCode(src, m);
	PrintROSENamespaceOpenCode(hdr, m);
	PrintROSENamespaceOpenCode(hdrInterface, m);

	//print Operation defines
	fprintf (hdr, "//------------------------------------------------------------------------------\n");
	fprintf (hdr, "// Operation defines\n\n");
	FOR_EACH_LIST_ELMT (vd, m->valueDefs)
		PrintROSEOperationDefines(hdr, r, vd, 0);
	fflush(hdr);
	fprintf (hdr, "\n");
	fprintf (hdr, "//------------------------------------------------------------------------------\n");

	/* nun alle Makro definitionen iterieren. */
	fprintf (hdr, "//------------------------------------------------------------------------------\n");

	//forward decl
	fprintf (hdr, "class %sInterface;\n", m->ROSEClassName);


	fprintf (hdr, "//------------------------------------------------------------------------------\n");
	fprintf (hdr, "// class definitions:\n\n");

	fprintf (src, "//------------------------------------------------------------------------------\n");
	fprintf (src, "// class member definitions:\n\n");

	PrintCxxAnyCode (src, hdr, r, mods, m);


	//Generate the ROSE class here.
	fprintf (hdr, "class %s : public SnaccROSEComponent\n{\n", m->ROSEClassName);
	fprintf (hdr, "public:\n");

	//Constructor
	fprintf (hdr, "\t%s(SnaccROSESender* pBase);\n", m->ROSEClassName);
	fprintf (src, "%s::%s(SnaccROSESender* pBase) : SnaccROSEComponent(pBase)\n", m->ROSEClassName, m->ROSEClassName);
	fprintf (src, "{\n}\n\n");

	//Function for triggering the registration of all Operations (name/id)
	fprintf (hdr, "\t//Function Registers all known operations in SnaccRoseOperationLookup\n");
	fprintf (hdr, "\tstatic void RegisterOperations();\n");
	fprintf (src, "void %s::RegisterOperations()\n", m->ROSEClassName);
	fprintf (src, "{\n");
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (PrintROSEOperationRegistration(src, r, vd) && !iFirstIIDFound) {
			iFirstIIDFound = 1;
			fprintf(hdr, "\tstatic const int m_iid = %d;\n", vd->value->basicValue->a.integer);
		}
	}
	fprintf (src, "}\n\n");
	fflush(src);
	fflush(hdr);


	//generate the InvokeHandler
	fprintf (hdr, "\t//The main Invoke Dispatcher\n");
	fprintf (hdr, "\tstatic long OnInvoke(SNACC::ROSEInvoke* pinvoke, SnaccROSESender* pBase, %sInterface* pInt, SnaccInvokeContext* cxt);\n", m->ROSEClassName);
	fprintf (src, "long %s::OnInvoke(SNACC::ROSEInvoke* pinvoke, SnaccROSESender* pBase, %sInterface* pInt, SnaccInvokeContext* cxt)\n", m->ROSEClassName, m->ROSEClassName);
	fprintf (src, "{\n");
	fprintf (src, "\tlong lRoseResult = ROSE_REJECT_UNKNOWNOPERATION;\n");
	fprintf (src, "\n");
	fprintf (src, "\t#ifndef ROSENOINVOKEDISPATCH\n");
	fprintf (src, "\tswitch (pinvoke->operationID)\n");
	fprintf (src, "\t{\n");

	//Rose Interface class
	fprintf (hdrInterface, "class %sInterface\n{\n", m->ROSEClassName);
	fprintf (hdrInterface, "public:\n");

	FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintROSEOnInvokeswitchCase(src, 0, m, vd);
		}
	}

	FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintROSEOnInvokeswitchCase(src, 1, m, vd);
		}
	}

	fprintf (src, "\t}\n");
	fprintf (src, "\t#endif //#ifndef ROSENOINVOKEDISPATCH\n");
	fprintf (src, "\treturn lRoseResult;\n");
	fprintf (src, "}\n");
	fprintf (src, "\n");

	fprintf (hdr, "\n");
	fprintf (hdr, "\t//Invoke Messages\n");
	//Now generate the invoke messages
	FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintROSEInvoke(hdr, src, m, 0, vd);
		}
	}

	fprintf (hdr, "\t//Event Messages\n");
	//Now generate the invoke messages
	FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintROSEInvoke(hdr, src, m, 1, vd);
		}
	}

	fprintf (hdr, "\t//Invoke Handler Messages\n");
	//Now generate the invoke handler messages
	FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintROSEOnInvoke(hdrInterface, 0, m, vd);
		}
	}
	fprintf (hdr, "\t//Event Handler Messages\n");
	//Now generate the Event handler messages
	FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintROSEOnInvoke(hdrInterface, 1, m, vd);
		}
	}

	fprintf (hdr, "\n");
	fprintf (hdr, "public:\n");
	fprintf (hdr, "\t//Invoke Overrides\n");
	fprintf (hdr, "}; //%s\n", m->ROSEClassName);

	fprintf (hdrInterface, "\n");
	fprintf (hdrInterface, "public:\n");
	fprintf (hdrInterface, "\t//Invoke Overrides\n");
	fprintf (hdrInterface, "}; //%s\n", m->ROSEClassName);

	fprintf (src, "\n");
	fprintf (hdr, "\n");
	fprintf (hdrInterface, "\n");


	PrintROSENamespaceCloseCode(src);
	PrintROSENamespaceCloseCode(hdr);
	PrintROSENamespaceCloseCode(hdrInterface);

	PrintConditionalIncludeClose (hdr, m->ROSEHdrFileName);
	PrintConditionalIncludeClose (hdrInterface, m->ROSEHdrInterfaceFileName);

} /* PrintROSECode */


void PrintCxxCode(FILE *src,
				  FILE *hdr,
				  if_META (MetaNameStyle printMeta _AND_)
				  if_META (const Meta *meta _AND_)
				  if_META (MetaPDU *meta_pdus _AND_)
				  ModuleList *mods,
				  Module *m,
				  CxxRules *r,
				  long longJmpVal,
				  int printTypes,
				  int printValues,
				  int printEncoders,
				  int printDecoders,
				  int printJSONEncDec,
				  int printPrinters,
				  int printPrintersXML,
				  int printFree
				  if_TCL (_AND_ int printTcl),
				  int novolatilefuncs)
{
	Module *currMod;
	AsnListNode *currModTmp;
	TypeDef *td;
	ValueDef *vd;

	longJmpValG = longJmpVal;
	printTypesG = printTypes;
	printEncodersG = printEncoders;
	printDecodersG = printDecoders;
	printJSONEncDecG = printJSONEncDec;
	printPrintersG = printPrinters;
	printPrintersXMLG = printPrintersXML;
	printFreeG = printFree;

#if META
	printMetaG = printMeta;
	meta_pdus_G = meta_pdus;
#if TCL
	printTclG = printTcl;
#endif /* TCL */
#endif /* META */

	PrintSrcComment (src, m);
	PrintHdrComment (hdr, m);
	PrintConditionalIncludeOpen (hdr, m->cxxHdrFileName);

	if (genCodeCPPPrintStdAfxInclude) {
		fprintf (src, "#include \"stdafx.h\"\n");
	}

#if META
	if (printMetaG)
	{
		fprintf (src, "\n");
		fprintf (src, "#ifndef META\n");
		fprintf (src, "#define META	1\n");
		fprintf (src, "#endif\n");
#if TCL
		if (printTclG)
		{
			fprintf (src, "#ifndef TCL\n");
			fprintf (src, "#define TCL	META\n");
			fprintf (src, "#endif\n");
		}
#endif /* TCL */
	}
#endif /* META */

	fprintf (hdr, "#include <cpp-lib/include/asn-incl.h>\n");
	fprintf (hdr, "#include <cpp-lib/include/asn-listset.h>\n");
	fprintf (src, "\n");    //RWC; PRINT before possible "namespace" designations.

	PrintSrcIncludes (src, mods, m);

	FOR_EACH_LIST_ELMT (currMod, mods)
	{
		if ((strcmp(m->cxxHdrFileName, currMod->cxxHdrFileName) == 0))
		{
			// Code to see the import module list AND load possible "namespace" refs.
			ImportModuleList *ModLists;
			ImportModule *impMod;
			char *ImpFile = NULL;
			ModLists = currMod->imports;
			currModTmp = mods->curr;    //RWC;
			FOR_EACH_LIST_ELMT(impMod, ModLists)
			{
				ImpFile = GetImportFileName (impMod->modId->name, mods);
				if (ImpFile != NULL)
					fprintf (hdr, "#include \"%s\"\n", RemovePath(ImpFile));
				if (impMod->moduleRef == NULL)  // RWC; attempt to update...
					impMod->moduleRef = GetImportModuleRef(impMod->modId->name, mods);
				if (impMod->moduleRef &&
					impMod->moduleRef->namespaceToUse)
				{
					fprintf(src,"using namespace %s;\n", impMod->moduleRef->namespaceToUse);
				}
			}
			mods->curr = currModTmp;    // RWC;RESET loop control
		}
		// Don't duplicate header file referenced in source
		//		if ((strcmp(m->cxxHdrFileName, currMod->cxxHdrFileName) != 0))
		//		{
		//			if ((ImportedFilesG == FALSE) || (currMod->ImportedFlag == TRUE))
		//				fprintf (hdr, "#include \"%s\"\n", currMod->cxxHdrFileName);
		//		}
	}

	fprintf (hdr, "\n");
	fprintf (src, "\n");

	/* 7-09-2001 Pierce Leonberger
	*   Added code to include all SNACC generated code in the SNACC namespace.
	*   If the namespace name was overridden with the '-ns' switch then
	*   use the name specified.  If the '-nons' switch was used then don't
	*   use namespaces for SNACC generated code.
	*/
	if (gNO_NAMESPACE == 0)
	{
		fprintf(hdr,"#ifndef NO_NAMESPACE\n");
		if (gAlternateNamespaceString)
		{
			fprintf(hdr,"namespace %s {\n", gAlternateNamespaceString);
			fprintf(src,"namespace %s {\n", gAlternateNamespaceString);
			fprintf(hdr, "using namespace SNACC;\n");
			fprintf(src, "using namespace SNACC;\n");
		}
		else if (m->namespaceToUse)
		{            // PRINT namespace designated in .asn1 file for this module.
			fprintf(hdr,"namespace %s {\n", m->namespaceToUse);
			fprintf(src,"namespace %s {\n", m->namespaceToUse);
			fprintf(hdr, "using namespace SNACC;\n");
			fprintf(src, "using namespace SNACC;\n");
		}
		else
		{
			fprintf(hdr,"namespace SNACC {\n");
			fprintf(src,"namespace SNACC{\n");
		}
		fprintf(hdr,"#endif\n");
	}

	if (bVDAGlobalDLLExport)
	{
		/* RWC; VDA Enhanced to allow produced files to be DLLs exporting
		* SNACC classes.
		*/
		fprintf (hdr,
			"// RWC; IF static refs to this class set, compiler define %s=""""\n",
			bVDAGlobalDLLExport);

		fprintf (hdr, "#ifndef %s\n", bVDAGlobalDLLExport);
		fprintf (hdr, "#if defined(_WIN32)\n");
		if (strcmp(bVDAGlobalDLLExport, "SNACCDLL_API") == 0)
		{
			/* Special case for compatibility */
			fprintf (hdr, "#ifdef SNACCDLL_EXPORTS\n");
		}
		else
		{
			fprintf (hdr, "#ifdef %s_EXPORTS\n", bVDAGlobalDLLExport);
		}

		fprintf (hdr, "#define %s __declspec(dllexport)\n", bVDAGlobalDLLExport);
		fprintf (hdr, "#else\n");
		fprintf (hdr, "#define %s __declspec(dllimport)\n", bVDAGlobalDLLExport);
		fprintf (hdr, "#endif      // %s\n", bVDAGlobalDLLExport);
		fprintf (hdr, "#else       // Handle Unix...\n");
		fprintf (hdr, "#define %s \n", bVDAGlobalDLLExport);
		fprintf (hdr, "#endif      // _WIN32\n");
		fprintf (hdr, "#endif      // %s\n", bVDAGlobalDLLExport);
	}       /* RWC;VDA; END Additional support for DLLs. */

	fprintf (hdr, "//------------------------------------------------------------------------------\n");
	fprintf (hdr, "// class declarations:\n\n");
	FOR_EACH_LIST_ELMT (td, m->typeDefs)
		PrintTypeDecl (hdr, td);
	fprintf (hdr, "\n");

#if META
	if (printMeta)
	{
		fprintf (hdr, "#if META\n");
		fprintf (src, "#if META\n\n");

		fprintf (hdr, "//------------------------------------------------------------------------------\n");

		char *ptr="";   /* NOT DLL Exported, or ignored on Unix. */
		if (bVDAGlobalDLLExport != NULL)
			ptr = bVDAGlobalDLLExport;

		fprintf (hdr, "extern const %s AsnModuleDesc %sModuleDesc;\n", ptr, m->cxxname);

		fprintf (src, "//------------------------------------------------------------------------------\n");
		fprintf (src, "static const AsnTypeDesc *%sModuleTypes[] =\n", m->cxxname);
		fprintf (src, "{\n");
		FOR_EACH_LIST_ELMT (td, m->typeDefs)
			fprintf (src, "  &%s::_desc,\n", td->cxxTypeDefInfo->className);
		fprintf (src, "  NULL\n");
		fprintf (src, "};\n\n");

#if 0 /* yet unused: */
		if (printMetaG == META_backend_names)
		else /* META_asn1_names */
#endif

		fprintf (src, "const AsnModuleDesc %sModuleDesc = { \"%s\", %sModuleTypes };\n\n", m->cxxname, m->modId->name, m->cxxname);

		fprintf (hdr, "#endif // META\n\n");
		fprintf (src, "#endif // META\n\n");
	}
#endif /* META */

	/* REMOVE PIERCE changed to print the Value definitions to the header file
	* 10-25-2001
	*/
	if (printValues)
	{
		fprintf (src, "//------------------------------------------------------------------------------\n");
		fprintf (src, "// value defs\n\n");
		FOR_EACH_LIST_ELMT (vd, m->valueDefs)
			PrintCxxValueDef (src, r, vd);
		fprintf (src, "\n");
	}
	/* REMOVE PIERCE changed const values to print to the header file only.  This negates the
	* need to extern or export the const values.
	* 10-25-2001
	*/
	if (printValues)
	{
		fprintf (hdr, "//------------------------------------------------------------------------------\n");
		fprintf (hdr, "// externs for value defs\n\n");
		FOR_EACH_LIST_ELMT (vd, m->valueDefs)
			PrintCxxValueExtern (hdr, r, vd);
	}

	fprintf (hdr, "//------------------------------------------------------------------------------\n");

	fprintf (hdr, "//------------------------------------------------------------------------------\n");
	fprintf (hdr, "// class definitions:\n\n");
	fprintf (src, "//------------------------------------------------------------------------------\n");
	fprintf (src, "// class member definitions:\n\n");

	PrintCxxAnyCode (src, hdr, r, mods, m);

	FOR_EACH_LIST_ELMT (td, m->typeDefs)
		PrintCxxTypeDefCode (src, hdr, mods, m, r, td, novolatilefuncs);

	/* 7-09-2001 Pierce Leonberger
	*/
	if (gNO_NAMESPACE == 0)
	{
		fprintf(hdr, "#ifndef NO_NAMESPACE\n");
		fprintf(hdr, "} // namespace close\n");
		fprintf(hdr, "#endif\n");
		fprintf(src, "#ifndef NO_NAMESPACE\n");
		fprintf(src, "} // namespace close\n");
		fprintf(src, "#endif\n");
	}

	PrintConditionalIncludeClose (hdr, m->cxxHdrFileName);

} /* PrintCxxCode */


void PrintCxxEncodeContaining(Type *t, CxxRules *r, FILE *src)
{
	fprintf(src, "    l += %s", t->cxxTypeRefInfo->fieldName);
	if (t->cxxTypeRefInfo->isPtr)
		fprintf(src, "->");
	else
		fprintf(src, ".");

	fprintf(src, "B%s(_b);\n", r->encodeBaseName);

	/* If this is a BITSTRING CONTAINING encode a NULL octet for the unused
	* bits
	*/
	if (t->basicType->choiceId == BASICTYPE_BITCONTAINING)
	{
		fprintf(src,"    _b.PutByteRvs((char) 0 ); //encode 0 for unused bits\n");
		fprintf(src,"    l++;\n");
	}
}

void PrintCxxDecodeContaining(Type *t, CxxRules *r, FILE *src)
{
	NamedType *defByNamedType;

	/* Encode Content of contained type */
	if (t->basicType->a.stringContaining->basicType->choiceId ==
		BASICTYPE_ANYDEFINEDBY)
	{
		defByNamedType =
			t->basicType->a.stringContaining->basicType->a.anyDefinedBy->link;
		PrintCxxSetTypeByCode(defByNamedType, t->cxxTypeRefInfo, src);
	}

	if (t->basicType->choiceId == BASICTYPE_BITCONTAINING)
	{
		fprintf(src,"\n");
		fprintf(src,"    // Decode unused bits and make sure it's 0\n");
		fprintf(src,"    char unusedBits;\n");
		fprintf(src,"    unusedBits = _b.GetByte();\n");
		fprintf(src,"    seqBytesDecoded++;\n");
		fprintf(src,"    if (unusedBits != '0x0')\n");
		fprintf(src,"      throw DecodeException(STACK_ENTRY);\n");
		fprintf(src,"\n");
	}

	fprintf (src, "    %s", t->cxxTypeRefInfo->fieldName);
	if (t->cxxTypeRefInfo->isPtr)
		fprintf (src, "->");
	else
		fprintf (src, ".");

	fprintf (src, "B%s (_b, seqBytesDecoded);\n",  r->decodeBaseName);
}


void PrintCxxPEREncodeContaining(Type *t, CxxRules *r, FILE *src)
{
	fprintf(src, "    l += %s", t->cxxTypeRefInfo->fieldName);
	if (t->cxxTypeRefInfo->isPtr)
		fprintf(src, "->");
	else
		fprintf(src, ".");

	fprintf(src, "P%s(_b);\n", r->encodeBaseName);

	/* If this is a BITSTRING CONTAINING encode a NULL octet for the unused
	* bits
	*/
	if (t->basicType->choiceId == BASICTYPE_BITCONTAINING)
	{
		fprintf(src,"    unsigned char _tmp[] = {0x00};\n");
		fprintf(src,"    _b.PutBits(tmp , 8); //encode 0 for unused bits\n");
		fprintf(src,"    l++;\n");
	}
}

void PrintCxxPERDecodeContaining(Type *t, CxxRules *r, FILE *src)
{
	NamedType *defByNamedType;

	/* Encode Content of contained type */
	if (t->basicType->a.stringContaining->basicType->choiceId ==
		BASICTYPE_ANYDEFINEDBY)
	{
		defByNamedType =
			t->basicType->a.stringContaining->basicType->a.anyDefinedBy->link;
		PrintCxxSetTypeByCode(defByNamedType, t->cxxTypeRefInfo, src);
	}

	if (t->basicType->choiceId == BASICTYPE_BITCONTAINING)
	{
		fprintf(src,"\n");
		fprintf(src,"    // Decode unused bits and make sure it's 0\n");
		fprintf(src,"    unsigned char* unusedBits;\n");
		fprintf(src,"    unusedBits = _b.GetBits(8);\n");
		fprintf(src,"    bitsDecoded++;\n");
		fprintf(src,"    if (unusedBits[0] != '0x0')\n");
		fprintf(src,"      throw DecodeException(STACK_ENTRY);\n");
		fprintf(src,"\n");
	}

	fprintf (src, "    %s", t->cxxTypeRefInfo->fieldName);
	if (t->cxxTypeRefInfo->isPtr)
		fprintf (src, "->");
	else
		fprintf (src, ".");

	fprintf (src, "P%s (_b, bitsDecoded);\n",  r->decodeBaseName);
}


void PrintCxxSetTypeByCode(NamedType *defByNamedType, CxxTRI *cxxtri, FILE *src)
{
	char *varName = cxxtri->fieldName;

	if (GetBuiltinType (defByNamedType->type) == BASICTYPE_OID)
	{
		fprintf (src, "    %s", varName);
		if (cxxtri->isPtr)
			fprintf (src, "->");
		else
			fprintf (src, ".");

		fprintf (src, "SetTypeByOid (");
		if (defByNamedType->type->cxxTypeRefInfo->isPtr)
			fprintf (src, " *");
		fprintf (src, "%s);\n", defByNamedType->type->cxxTypeRefInfo->fieldName);
	}
	else if (GetBuiltinType (defByNamedType->type) == BASICTYPE_INTEGER)
	{
		fprintf (src, "    %s", varName);
		if (cxxtri->isPtr)
			fprintf (src, "->");
		else
			fprintf (src, ".");

		fprintf (src, "SetTypeByInt (");
		if (defByNamedType->type->cxxTypeRefInfo->isPtr)
			fprintf (src, " *");
		fprintf (src, "%s);\n", defByNamedType->type->cxxTypeRefInfo->fieldName);
	}
	else if (GetBuiltinType (defByNamedType->type) == BASICTYPE_CHOICE)
	{
		NamedType *nt;
		Type      *t = ResolveImportedType(defByNamedType->type);

		fprintf(src, "  switch (%s->choiceId)\n", defByNamedType->type->cxxTypeRefInfo->fieldName);
		fprintf(src, "  {\n");

		FOR_EACH_LIST_ELMT(nt, t->basicType->a.choice)
		{
			fprintf(src, "   case %s::%sCid:\n", defByNamedType->type->cxxTypeRefInfo->className, nt->fieldName);
			if (nt->type->basicType->choiceId == BASICTYPE_INTEGER ||
				nt->type->basicType->choiceId == BASICTYPE_ENUMERATED)
			{
				fprintf (src, "      %s", varName);
				if (cxxtri->isPtr)
					fprintf (src, "->");
				else
					fprintf (src, ".");

				fprintf(src, "SetTypeByInt(*%s->%s);\n", defByNamedType->type->cxxTypeRefInfo->fieldName, nt->fieldName);
			}
			else
			{
				fprintf (src, "      %s", varName);
				if (cxxtri->isPtr)
					fprintf (src, "->");
				else
					fprintf (src, ".");

				fprintf(src, "SetTypeByOid(*%s->%s);\n", defByNamedType->type->cxxTypeRefInfo->fieldName, nt->fieldName);
			}
			fprintf(src, "      break;\n");

		}
		fprintf(src, "  }\n");
	}
}


char * LookupNamespace(Type *t, ModuleList *mods)
{
	char *pszNamespace=NULL;
	Module *mTmp=NULL;
	TypeDef *ptTmp=NULL;
	BasicType *pbtTmp2=NULL;

	//RWC; HANDLE namespace designations of specific modules on declarations,
	//      if necessary.  (May have to lookup module name to get namespace).
	pbtTmp2 = t->basicType;
	if (pbtTmp2->choiceId == BASICTYPE_SEQUENCEOF ||
		pbtTmp2->choiceId == BASICTYPE_SETOF)
		pbtTmp2 = pbtTmp2->a.sequenceOf->basicType;  // Burrow 1 more layer down for SequenceOf/SetOf
	if (pbtTmp2->choiceId == BASICTYPE_IMPORTTYPEREF)
	{                     // RWC; IF IMPORTED, then we need to check for
		//       optional namespace designation (only in .h)
		FOR_EACH_LIST_ELMT (mTmp, mods)
		{
			ptTmp = LookupType(mTmp->typeDefs,
				pbtTmp2->a.importTypeRef->typeName); //WHAT we are looking for...
			if (ptTmp != NULL)
				break;      //FOUND the MODULE that contains our defninition...
		}       // END FOR each module.
		if (ptTmp != NULL && mTmp != NULL && mTmp->namespaceToUse)  // FOUND our MODULE...
		{
			pszNamespace = mTmp->namespaceToUse;    // DO NOT DELETE...
		}
		//LookupType PARAMS ((typeDefList, typeName),
	}           // IF BASICTYPE_IMPORTTYPEREF

	return(pszNamespace);
}       /* END LookupNamespace(...)*/


static void PrintCxxSeqSetPrintFunction(FILE* src, FILE* hdr, MyString className,
								 BasicType *pBasicType)
{
	int allOpt;
	int inTailOptElmts;
	NamedTypeList* pElmtList=NULL;
	NamedType *e;

	fprintf(hdr, "  void Print(std::ostream& os, unsigned short indent = 0) const;\n");

	fprintf(src, "void %s::Print(std::ostream& os, unsigned short indent) const\n",
		className);
	fprintf(src, "{\n");

	if (pBasicType->choiceId == BASICTYPE_SEQUENCE)
	{
		fprintf(src, "\tos << std::endl;\n");
		fprintf(src, "\tIndent(os, indent);\n");
		fprintf(src, "\tos << \"{ -- SEQUENCE --\" << std::endl;\n");
		pElmtList = pBasicType->a.sequence;
	}
	else if (pBasicType->choiceId == BASICTYPE_SET)
	{
		fprintf(src, "\tos << std::endl;\n");
		fprintf(src, "\tIndent(os, indent);\n");
		fprintf(src, "\tos << \"{ -- SET --\" << std::endl;\n");
		pElmtList = pBasicType->a.set;
	}
	else
		abort();

	allOpt = AllElmtsOptional(pElmtList);
	if (allOpt)
		fprintf(src, "\tint nonePrinted = true;\n");

	fprintf(src, "\t++indent;\n\n");

	FOR_EACH_LIST_ELMT (e, pElmtList)
	{
		inTailOptElmts = IsTailOptional(pElmtList);

		if (e->type->cxxTypeRefInfo->isPtr)
		{
			fprintf(src, "\tif (%s (%s))\n",
				e->type->cxxTypeRefInfo->optTestRoutineName,
				e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "\t{\n");

			if (allOpt)
			{
				if (e != FIRST_LIST_ELMT (pElmtList))
				{
					//fprintf(src, "\t\tif (!nonePrinted) {\n");
					//fprintf(src, "\t\tIndent(os, indent);\n");
					//fprintf(src, "\t\t\tos << \",\" << std::endl;\n");
					//fprintf(src, "\t\t\t}\n");
				}
				fprintf(src, "\t\tnonePrinted = false;\n");
			}
			/* cannot be first elmt ow allOpt is true */
			else if (inTailOptElmts)
			{
				//fprintf(src, "\t\tIndent(os, indent);\n");
				//fprintf (src, "\t\tos << \",\"<< std::endl;\n");
			}

			fprintf(src, "\t\tIndent(os, indent);\n");

			if (e && e->fieldName != NULL)
				fprintf(src, "\t\tos << \"%s \";\n", e->fieldName);

			if(e)
				fprintf(src, "\t\t%s->Print(os, indent);\n", e->type->cxxTypeRefInfo->fieldName);

			fprintf(src, "\t\tos << std::endl;\n");
			fprintf(src, "\t}\n");
		}
		else
		{
			fprintf(src, "\tIndent(os, indent);\n");

			if (e->fieldName != NULL)
				fprintf(src, "\tos << \"%s \";\n", e->fieldName);

			fprintf(src, "\t%s.Print(os, indent);\n",
				e->type->cxxTypeRefInfo->fieldName);

			fprintf(src, "\tos << std::endl;\n");
			//if (e != LAST_LIST_ELMT (pElmtList))
			//	fprintf(src, "\tos << ',' << std::endl;\n");
		}

		fprintf (src, "\n");

		//if (e == LAST_LIST_ELMT (pElmtList))
		//	fprintf(src, "\tos << std::endl;\n");
	}

	fprintf(src, "\t--indent;\n");
	fprintf(src, "\tIndent(os, indent);\n");
	fprintf(src, "\tos << \"}\\n\";\n");
	fprintf (src, "} // end of %s::Print()\n\n", className);
} /* end of PrintCxxSeqSetPrintFunction() */


/*
* RWC;   */
static void
PrintCxxDefCode_SetSeqPEREncode (FILE *src, FILE *hdr, CxxRules *r, TypeDef *td,
								 NamedType **pSetElementNamedType,
								 int iElementCount)      /* IN, ELEMENT Count to process in array*/
{
	NamedType *e;
	char *varName;
	CxxTRI *cxxtri=NULL;
	enum BasicTypeChoiceId tmpTypeId;
	NamedType *defByNamedType;
	int extensionsExist = FALSE;

	// DEFINE PER encode/decode tmp vars.
	int ii=0;
	long lOptional_Default_ElmtCount=0;
	const char* tabAndlenVar = "\tl";

	fprintf(hdr, "\t%s\t\tP%s(AsnBufBits &_b) const;\n", lenTypeNameG,
		r->encodeBaseName);
	/*RWC; {AsnLen len; len = 1;return len;};\n");
	RWC; MUST sort the results by tag; usually explicit except for
	RWC;  untagged Choices which can be nested.  We must determine which
	RWC;  tag can go 1st from any Choice, potentially nested.
	RWC;  (FORWARD ENCODING FOR PER!)*/

	fprintf(src, "%s %s::P%s(AsnBufBits &_b) const\n", lenTypeNameG,
		td->cxxTypeDefInfo->className, r->encodeBaseName);
	fprintf(src, "{\n\t%s l = 0;\n", lenTypeNameG);

	/* SECOND, determine ahead of time the bit count for OPTIONAL/DEFAULT values. */
	for (ii=0; ii < iElementCount; ii++)
	{
		e = pSetElementNamedType[ii];

		//RWC; ALSO, count any OPTIONAL/DEFAULT ASN.1 elements.
		if ( (e->type->optional || e->type->defaultVal != NULL) && (!e->type->extensionAddition))
			lOptional_Default_ElmtCount++;
	}

	/* NEXT, decode this number of bits, if any, to determine the
	presence/absence of OPTIONAL/DEFAULT elements.*/
	if (lOptional_Default_ElmtCount)
	{	/* NOW, load PER encoding flag to indicate what OPTIONAL/DEFAULT
		fields are actually present.*/
		/* FOR PER ENCODING OF Set, we must load a bitfield of length
		"lOptional_Default_ElmtCount", indicating presence of optional
		or default field data. */
		int iOptional_Default_ElementIndex=0;
		fprintf(src, "\n\t// Build and encode preamble");
		fprintf(src, "\n\tAsnBits SnaccOptionalDefaultBits(%ld);\n", lOptional_Default_ElmtCount);
		for (ii=0; ii < iElementCount; ii++)
		{
			e = pSetElementNamedType[ii];
			if ( (e->type->optional || e->type->defaultVal != NULL) && (!e->type->extensionAddition))
			{
				fprintf (src, "\tif (%s != NULL)\n",
					e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "\t\tSnaccOptionalDefaultBits.SetBit(%d);\n",
					iOptional_Default_ElementIndex++);
			}		/* END IF OPTIONAL/DEFAULT */
		}	/* END FOR each element. */
		fprintf (src, "\t_b.PutBits(SnaccOptionalDefaultBits.data(), %ld);\n",
			lOptional_Default_ElmtCount);
		fprintf (src, "\tl += %ld;\n", lOptional_Default_ElmtCount);

	}		/* END IF lOptional_Default_ElmtCount */

	/* NEXT, process each element of the Set/Sequence for decoding. */
	fprintf(src, "\n\t// Encode the elements of the SEQUENCE\n");
	for (ii=0; ii < iElementCount; ii++)
	{
		e = pSetElementNamedType[ii];
		if(!e->type->extensionAddition)
		{


			if ( (e->type->optional || e->type->defaultVal != NULL) && (!e->type->extensionAddition))
			{
				fprintf(src, "\tif (%s != NULL)\t// Optional OR Default\n",
					e->type->cxxTypeRefInfo->fieldName);
				tabAndlenVar = "\t\tl";
			}

			cxxtri =  e->type->cxxTypeRefInfo;
			varName = cxxtri->fieldName;

			/* encode tag(s), not UNIV but APL, CNTX or PRIV tags ONLY for PER */

			/*RWC;TBD; UPDATE to reflect PER encoding rules for encoding length, no tags
			RWC;TBD;  unless explicit (probably need to write a PER version, checking type).*/

			//RWC;NOT FOR PER;PrintCxxTagAndLenEncodingCode (src, td, e->type, "l", "(*iBuf)");

			/* encode content */
			tmpTypeId = GetBuiltinType (e->type);
			if (tmpTypeId == BASICTYPE_ANYDEFINEDBY)
			{
				//RWC;TBD; we may have to investigate individual types here to
				//RWC;TBD;	restrict which codes are printed for PER...
				defByNamedType = e->type->basicType->a.anyDefinedBy->link;
				PrintCxxSetTypeByCode(defByNamedType, cxxtri, src);

				fprintf(src, "%s += %s", tabAndlenVar, varName);
				if (cxxtri->isPtr)
					fprintf (src, "->");
				else
					fprintf (src, ".");
				fprintf (src, "P%s(_b);\n", r->encodeBaseName);
			}
			else if (tmpTypeId == BASICTYPE_ANY)
			{
				//RWC;NOTE:  we will assume here that the ANY buffer is already
				//RWC;NOTE:    properly PER encoder; we have no way of checking.
				fprintf(src, "%s += %s", tabAndlenVar, varName);
				if (cxxtri->isPtr)
					fprintf (src, "->");
				else
					fprintf (src, ".");

				fprintf (src, "P%s(_b);\n", r->encodeBaseName);
			}
			else if (tmpTypeId == BASICTYPE_BITCONTAINING)
			{
				PrintCxxPEREncodeContaining(e->type, r, src);
				//RWC;TBD; THIS CALL WILL NOT UPDATE THE COUNT VALUE PROPERLY; must reflect
				//RWC;TBD;	PER encoding forward, l+=, instead of l=
			}
			else
			{
				fprintf(src, "%s += %s", tabAndlenVar, varName);
				if (cxxtri->isPtr)
					fprintf (src, "->");
				else
					fprintf (src, ".");

				fprintf (src, "P%s(_b);\n", r->encodeBaseName);/*RWC;r->encodeContentBaseName);*/
			}
		}
		else
		{
			extensionsExist = TRUE;
		}
	}			/* END FOR iElementCount */


	if(extensionsExist)
	{
		fprintf (src, " \t/*   WARNING:  PER does not yet support extensibility */\n");
	}

	fprintf(src, "\n\treturn l;\n");
	fprintf(src, "}\t// %s::P%s\n\n\n", td->cxxTypeDefInfo->className,
		r->encodeBaseName);
} /* END PrintCxxDefCode_SetSeqPEREncode(...) */


/*
* RWC;  This method only handles the entire routine decode operations for both Set and Sequence PER Decode.
* element decodes, not the wrapping logic. */
static void
PrintCxxDefCode_SetSeqPERDecode (FILE *src, FILE *hdr, CxxRules *r, TypeDef *td,
								 NamedType **pSetElementNamedType,
								 int iElementCount)      /* IN, ELEMENT Count to process in arrays */
{
	NamedType *e;
	char *varName;
	CxxTRI *cxxtri=NULL;
	int varCount, tmpVarCount;
	enum BasicTypeChoiceId tmpTypeId;
	NamedType *defByNamedType;

	// DEFINE PER encode/decode tmp vars.
	int ii=0;
	int iOptional_Default_ElementIndex=0;
	long lOptional_Default_ElmtCount=0;

	if (pSetElementNamedType == NULL)
	{
		printf("****** PrintCxxDefCode_SetSeqPERDecode: MUST HAVE PER Encoders as well as PER Decoders! *****\n");
		return;
	}

	/***RWC; PERFORM PDec operations first... */
	fprintf(hdr, "\tvoid\t\tP%s(AsnBufBits& _b, %s& bitsDecoded);\n\n",
		r->decodeBaseName, lenTypeNameG);

	fprintf(src, "void %s::P%s(AsnBufBits& _b, %s& bitsDecoded)\n",
		td->cxxTypeDefInfo->className, r->decodeBaseName, lenTypeNameG);
	fprintf(src, "{\n");
	fprintf(src, "\tClear();\n");

	/* count max number of extra length var nec */
	varCount = 0;

	for (ii=0; ii < iElementCount; ii++)
		//FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
	{
		e = pSetElementNamedType[ii];
		tmpVarCount = CxxCountVariableLevels (e->type);
		if (tmpVarCount > varCount)
			varCount = tmpVarCount;
		if ( (e->type->optional || e->type->defaultVal != NULL) && (!e->type->extensionAddition))
		{
			lOptional_Default_ElmtCount++;
		}
	}

	/* NEXT, decode this number of bits, if any, to determine the
	presence/absence of OPTIONAL/DEFAULT elements.	MUST BE DONE BEFORE
	TAGs.*/
	if (lOptional_Default_ElmtCount)
	{
		fprintf(src, "\n\t// Decode the preamble\n");
		fprintf(src, "\tAsnBits SnaccOptionalDefaultBits;\n");
		fprintf(src, "\tbitsDecoded += _b.GetBits(SnaccOptionalDefaultBits, %ld);\n",
			lOptional_Default_ElmtCount);
	}

	//******************
	/****RWC; PERFORM PDecContent operations here... ***/
	/* print content local vars */
	//	fprintf (src, "  unsigned int mandatoryElmtsDecoded = 0;\n");

	//******************
	/* write extra length vars */
	//	fprintf (src, "{\n");		// RWC; Temporary until I figure out the local var names from combining PDec with PDecContent
	//	for (i = 1; i <= varCount; i++)
	//		fprintf (src, "  %s elmtLen%d = 0; //RWC;default to infinite for now.\n", lenTypeNameG, i);
	//	fprintf (src, "\n");

	/* handle empty set */
	//RWC;if ((set->basicType->a.set == NULL) || LIST_EMPTY (set->basicType->a.set))
	if (iElementCount == 0)
	{
		// RWC; Allow for "{" editing...
		/*fprintf (src, "    throw EXCEPT(\"Expected an empty sequence\", DECODE_ERROR);\n");
		fprintf (src, "  }\n");*/
	}
	else
	{
		fprintf(src, "\n\t// Decode each of the elements\n");
		for (ii=0; ii < iElementCount; ii++)
		{
			const char* tabStr = "\t";
			e = pSetElementNamedType[ii];

			if(!e->type->extensionAddition)
			{
				cxxtri =  e->type->cxxTypeRefInfo;
				if (e->type->optional || (e->type->defaultVal != NULL))
				{
					tabStr = "\t\t";
					fprintf(src, "\tif (SnaccOptionalDefaultBits.GetBit(%d))\n",
						iOptional_Default_ElementIndex++);
					fprintf(src, "\t{\n");
				}

				varName = cxxtri->fieldName;

				/* decode content */
				if (cxxtri->isPtr)
				{
					//fprintf(src, "%sif(%s)\n", tabStr, varName);
					//fprintf(src, "%s%sdelete %s;\n", tabStr, tabStr, varName);

					fprintf(src, "%s%s = new %s;\n", tabStr, varName,
						cxxtri->className);
					/* END IF subtypes, PER-Visible */
				}

				/* decode content */
				tmpTypeId = GetBuiltinType (e->type);
				if (tmpTypeId == BASICTYPE_BITCONTAINING)
				{
					PrintCxxPERDecodeContaining(e->type, r, src);
				}
				else
				{
					if (tmpTypeId == BASICTYPE_ANYDEFINEDBY)
					{
						defByNamedType = e->type->basicType->a.anyDefinedBy->link;
						PrintCxxSetTypeByCode(defByNamedType, cxxtri, src);
					}
					else if (tmpTypeId == BASICTYPE_ANY)
					{
					}

					if (cxxtri->isPtr)
						fprintf(src, "%s%s->", tabStr, varName);
					else
						fprintf(src, "%s%s.", tabStr, varName);
					fprintf(src, "P%s(_b, bitsDecoded);\n",	r->decodeBaseName);
				}

				if (e->type->optional || (e->type->defaultVal != NULL))
					fprintf (src, "\t}\n\n");
			}
		} /* for each elmt */
	} /* if not empty set clause */

	fprintf (src, "} // %s::P%s()\n\n", td->cxxTypeDefInfo->className,
		r->decodeBaseName);

}		/* END PrintCxxDefCode_SetSeqPERDecode(...) */

/*** This routine handles sorting of groups of NameType element(s) based on the
*   Set and Choice sorting rules.
*/
static void
PrintCxxDefCode_PERSort (
						 NamedType ***pppElementNamedType, /* OUT, array of sorted NameType(s) */
						 int **ppElementTag,      /* OUT, actual tag for sorted. */
						 AsnList *pElementList)   /* IN, actual eSNACC defs for NameType(s). */
{
	NamedType **pElementNamedType;
	int *pElementTag;
	NamedType *e;
	NamedType *pnamedTypeTmp;
	Tag *tag;
	TagList *tags;
	int tagTmp;
	int stoleChoiceTags;
	int ii=0, iii;

	/*
	* FIRST, determine encode order by looking at each element tag/type.
	*  (careful with untagged Choice elements, may be nested).
	*  If not tagged in the ASN.1 syntax, then we sort based on the IMPLICIT
	*  tag, even though it may not be encoded for PER.
	* pElementList->count total elements for PER encode sorting.*/
	pElementTag = *ppElementTag = (int *)calloc(pElementList->count, sizeof(int));
	pElementNamedType = *pppElementNamedType =
		(NamedType **)calloc(pElementList->count, sizeof(NamedType *));
	FOR_EACH_LIST_ELMT (e, pElementList)
	{
		/*RWC;SEE tag-utils.c, line 175 for example of looking at nested
		*RWC;  untagged Choice(s).  For PER, NEED to return lowest tag
		*RWC;  value in nested untagged Choice for sorting.
		*RWC;  The call to GetTags will only return tags with non-tagged
		*RWC;  "Choice" elements if present (flagged by "stoleChoiceTags").*/
		tags = GetTags (e->type, &stoleChoiceTags);

		if (LIST_EMPTY (tags))
		{
			pElementTag[ii] = 0;
			/* RWC; IGNORE; for now */
		}       /* END IF (LIST_EMPTY (tags))*/
		else if (stoleChoiceTags)
		{
			/* FOR untagged Choice, determine lowest possible tag for
			*  PER sorting order.*/
			pElementTag[ii] = 9999;
			FOR_EACH_LIST_ELMT (tag, tags)
			{
				if (tag->code < pElementTag[ii])
					pElementTag[ii] = tag->code;  /* ONLY 1st element for sorting.*/
			}
		}
		else
		{
			tag = (Tag*)FIRST_LIST_ELMT (tags);
			if (!tag)
				exit(3);
			pElementTag[ii] = tag->code;  // ONLY 1st element for sorting.
		}
		pElementNamedType[ii] = e;
		ii++;
	}       // END FOR each element.

	// SECOND, sort this group of elements based on these tags.
	if (!pElementList)
		exit(3);

	for (ii=0; ii < pElementList->count-1; ii++)
	{
		for (iii=ii+1; iii < pElementList->count; iii++)
		{   // LOCATE smallest tag value
			if (pElementTag[iii] < pElementTag[ii])
			{   // THEN switch them.
				tagTmp = pElementTag[ii];
				pnamedTypeTmp = pElementNamedType[ii];
				pElementTag[ii] = pElementTag[iii];
				pElementNamedType[ii] = pElementNamedType[iii];
				pElementTag[iii] = tagTmp;
				pElementNamedType[iii] = pnamedTypeTmp;
			}
		}   // END for remaining elements (for sorting)
	}       // END FOR each element
}       /* END PrintCxxDefCode_PERSort(...) */

void PrintCxxSimpleDefMeta_1(FILE * hdr, FILE* src, TypeDef* td, int hasNamedElmts, CNamedElmt *n, Module* m)
{
#if META
	const char	*T, *t;
	int a3;

	fprintf (hdr, "\n");
	fprintf (hdr, "#if META\n");
	fprintf (src, "#if META\n\n");

	fprintf (src, "static AsnType *create%s()\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  return new %s;\n", td->cxxTypeDefInfo->className);
	fprintf (src, "}\n\n");

	if ((hasNamedElmts = HasNamedElmts (td->type)))
	{
		fprintf (hdr, "  static const AsnNameDesc	_nmdescs[];\n");

		fprintf (src, "const AsnNameDesc %s::_nmdescs[] =\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");
		FOR_EACH_LIST_ELMT (n, td->type->cxxTypeRefInfo->namedElmts)
#if 0 /* (no asn1 names available!) */
			if (printMetaG == META_backend_names)
			else /* META_asn1_names */
#endif
			fprintf (src, "  \"%s\", %s, // %d\n", n->name, n->name, n->value);
		fprintf (src, "  NULL, -1\n");
		fprintf (src, "};\n\n");
	}

	switch (GetBuiltinType (td->type))
	{
	case BASICTYPE_BOOLEAN:
		T = "BOOLEAN";
		t = "Bool";
		a3 = FALSE;
		break;
	case BASICTYPE_ENUMERATED:
		T = "ENUMERATED";
		t = "Enum";
		a3 = TRUE;
		break;
	case BASICTYPE_INTEGER:
		T = "INTEGER";
		t = "Int";
		a3 = TRUE;
		break;
	case BASICTYPE_REAL:
		T = "REAL";
		t = "Real";
		a3 = FALSE;
		break;
	case BASICTYPE_OCTETSTRING:
		T = "OCTET_STRING";
		t = "Octs";
		a3 = FALSE;
		break;
	case BASICTYPE_BITSTRING:
		T = "BIT_STRING";
		t = "Bits";
		a3 = TRUE;
		break;
	case BASICTYPE_OID:
		T = "OID";
		t = "Oid";
		a3 = FALSE;
	case BASICTYPE_RELATIVE_OID:
		T = "RELATIVE_OID";
		t = "RelativeOid";
		a3 = FALSE;
	default:
		T =
			t = "?";
		a3 = FALSE;
	}

	fprintf (hdr, "  static const Asn%sTypeDesc	_desc;\n", t);
	fprintf (hdr, "  const AsnTypeDesc	*_getdesc() const;\n");

	fprintf (src, "const Asn%sTypeDesc %s::_desc\n", t, td->cxxTypeDefInfo->className);
	fprintf (src, "(\n");
	fprintf (src, "  &%sModuleDesc,\n", m->cxxname);
	if (printMetaG == META_backend_names)
		fprintf (src, "  \"%s\", // `%s'\n", td->cxxTypeDefInfo->className, td->definedName);
	else /* META_asn1_names */
		fprintf (src, "  \"%s\", // `%s'\n", td->definedName, td->cxxTypeDefInfo->className);
	fprintf (src, "  %s,\n", isMetaPDU (m->modId->name, td->definedName, meta_pdus_G) ? "true" : "false");
	fprintf (src, "  AsnTypeDesc::%s,\n", T);
	fprintf (src, "  create%s", td->cxxTypeDefInfo->className);
	if (a3)
		fprintf (src, ",\n  %s", hasNamedElmts ? "_nmdescs" : "NULL");
	fprintf (src, "\n);\n\n");

	fprintf (src, "const AsnTypeDesc *%s::_getdesc() const\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  return &_desc;\n");
	fprintf (src, "}\n\n");

	fprintf (hdr, "#endif // META\n");
	fprintf (src, "#endif // META\n\n");

#if TCL
#endif

#endif //META
}

void PrintCxxSimpleDefMeta_2(FILE * hdr, FILE* src, TypeDef* td, int hasNamedElmts, CNamedElmt *n, Module* m, CxxRules *r)
{
#if META
	fprintf (hdr, "#if META\n");
	fprintf (src, "#if META\n\n");

	fprintf (src, "static AsnType *create%s()\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  return new %s;\n", td->cxxTypeDefInfo->className);
	fprintf (src, "}\n\n");

	fprintf (hdr, "struct %s: public %s\n", td->cxxTypeDefInfo->className, td->type->cxxTypeRefInfo->className);
	fprintf (hdr, "{\n");

	PrintDerivedConstructors (hdr, r, td);
	PrintCloneMethod (hdr, src, td);

	fprintf (hdr, "  static const AsnAliasTypeDesc	_desc;\n");
	fprintf (hdr, "  const AsnTypeDesc	*_getdesc() const;\n");

	fprintf (src, "const AsnAliasTypeDesc %s::_desc\n", td->cxxTypeDefInfo->className);
	fprintf (src, "(\n");
	fprintf (src, "  &%sModuleDesc,\n", m->cxxname);
	if (printMetaG == META_backend_names)
		fprintf (src, "  \"%s\", // `%s'\n", td->cxxTypeDefInfo->className, td->definedName);
	else /* META_asn1_names */
		fprintf (src, "  \"%s\", // `%s'\n", td->definedName, td->cxxTypeDefInfo->className);
	fprintf (src, "  %s,\n", isMetaPDU (m->modId->name, td->definedName, meta_pdus_G) ? "true" : "false");
	fprintf (src, "  AsnTypeDesc::ALIAS,\n");
	fprintf (src, "  create%s,\n", td->cxxTypeDefInfo->className);
	fprintf (src, "  &%s::_desc\n);\n\n", td->type->cxxTypeRefInfo->className);

	fprintf (src, "const AsnTypeDesc *%s::_getdesc() const\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  return &_desc;\n");
	fprintf (src, "}\n\n");

	fprintf (hdr, "};\n\n");

	fprintf (hdr, "#else // META\n\n");
	fprintf (src, "#endif // META\n\n");
#endif //META
}

void PrintCxxChoiceDefCodeMeta_1(FILE* hdr, FILE* src, TypeDef* td, Type* choice, Module* m, NamedType* e)
{
#if META
	fprintf (hdr, "#if META\n");
	fprintf (src, "#if META\n\n");

	fprintf (hdr, "  static const AsnChoiceTypeDesc	_desc;\n");
	fprintf (hdr, "  static const AsnChoiceMemberDesc	_mdescs[];\n\n");
	fprintf (hdr, "  const AsnTypeDesc		*_getdesc() const;\n");
	fprintf (hdr, "  AsnType			*_getref (const char *membername, bool create = false);\n\n");

	fprintf (src, "static AsnType *create%s()\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  return new %s;\n", td->cxxTypeDefInfo->className);
	fprintf (src, "}\n\n");

	fprintf (src, "const AsnChoiceMemberDesc %s::_mdescs[] =\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
		if (printMetaG == META_backend_names)
			fprintf (src, "  AsnChoiceMemberDesc (\"%s\", &%s::_desc), // `%s'\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className, e->fieldName ? e->fieldName : "");
		else /* META_asn1_names */
			fprintf (src, "  AsnChoiceMemberDesc (\"%s\", &%s::_desc), // `%s'\n", e->fieldName ? e->fieldName : e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className, e->type->cxxTypeRefInfo->fieldName);
	fprintf (src, "  AsnChoiceMemberDesc()\n");
	fprintf (src, "};\n\n");

	fprintf (src, "const AsnChoiceTypeDesc %s::_desc\n", td->cxxTypeDefInfo->className);
	fprintf (src, "(\n");
	fprintf (src, "  &%sModuleDesc,\n", m->cxxname);
	if (printMetaG == META_backend_names)
		fprintf (src, "  \"%s\", // `%s'\n", td->cxxTypeDefInfo->className, td->definedName);
	else /* META_asn1_names */
		fprintf (src, "  \"%s\", // `%s'\n", td->definedName, td->cxxTypeDefInfo->className);
	fprintf (src, "  %s,\n", isMetaPDU (m->modId->name, td->definedName, meta_pdus_G) ? "true" : "false");
	fprintf (src, "  AsnTypeDesc::CHOICE,\n");
	fprintf (src, "  create%s,\n", td->cxxTypeDefInfo->className);
	fprintf (src, "  _mdescs\n");
	fprintf (src, ");\n\n");

	fprintf (src, "const AsnTypeDesc *%s::_getdesc() const\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  return &_desc;\n");
	fprintf (src, "}\n\n");

	fprintf (src, "AsnType *%s::_getref (const char *membername, bool create)\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  ChoiceIdEnum newCid = (ChoiceIdEnum)_desc.choicebyname (membername);\n");
	fprintf (src, "  if (newCid == -1)\n");
	fprintf (src, "    return NULL;\n");
	fprintf (src, "  if (newCid == choiceId)\n");
	fprintf (src, "  {\n");
	fprintf (src, "    switch (choiceId)\n");
	fprintf (src, "    {\n");
	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		fprintf (src, "      case %sCid:\n", e->type->cxxTypeRefInfo->fieldName);
		fprintf (src, "        return %s;\n", e->type->cxxTypeRefInfo->fieldName);
	}
	fprintf (src, "      default:\n");
	fprintf (src, "        return NULL;\n");
	fprintf (src, "    }\n");
	fprintf (src, "  }\n");
	fprintf (src, "  else\n");
	fprintf (src, "  {\n");
	fprintf (src, "    if (create)\n");
	fprintf (src, "    {\n");
	fprintf (src, "//      switch (choiceId)\n");
	fprintf (src, "//      {\n");
	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		fprintf (src, "//        case %sCid:\n", e->type->cxxTypeRefInfo->fieldName);
		fprintf (src, "//          delete %s;\n", e->type->cxxTypeRefInfo->fieldName);
		fprintf (src, "//          %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
		fprintf (src, "//          break;\n");
	}
	fprintf (src, "//        default:\n");
	fprintf (src, "//          return NULL;\n");
	fprintf (src, "//      }\n");
	e = FIRST_LIST_ELMT (choice->basicType->a.choice);
	fprintf (src, "      // simply delete any member, the virtual function table takes care of the rest:\n");
	fprintf (src, "      delete %s;\n", e->type->cxxTypeRefInfo->fieldName);
	fprintf (src, "      %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
	fprintf (src, "      switch (choiceId = newCid)\n");
	fprintf (src, "      {\n");
	FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
	{
		fprintf (src, "        case %sCid:\n", e->type->cxxTypeRefInfo->fieldName);
		fprintf (src, "          return %s = new %s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className);
	}
	fprintf (src, "        default: // internal error!\n");
	fprintf (src, "          return NULL;\n");
	fprintf (src, "      }\n");
	fprintf (src, "    }\n");
	fprintf (src, "    else\n");
	fprintf (src, "      return NULL;\n");
	fprintf (src, "  }\n");
	fprintf (src, "}\n\n");

#if TCL
	if (printTclG)
	{
		fprintf (hdr, "#if TCL\n");
		fprintf (src, "#if TCL\n\n");

		fprintf (hdr, "  int			TclGetDesc (Tcl_DString *) const;\n");
		fprintf (hdr, "  int			TclGetVal (Tcl_Interp *) const;\n");
		fprintf (hdr, "  int			TclSetVal (Tcl_Interp *, const char *valstr);\n\n");

		fprintf (src, "int %s::TclGetDesc (Tcl_DString *valstr) const\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");
		fprintf (src, "  Tcl_DStringAppendElement (valstr, (char*)_desc.choicebyvalue (choiceId));\n");
		fprintf (src, "  // hack: since all members are pointers, we don't have to check for its type via choiceId, because all we want to know is whether it's NULL or not:\n");
		e = FIRST_LIST_ELMT (choice->basicType->a.choice);
		fprintf (src, "  Tcl_DStringAppendElement (valstr, %s ? \"valid\" : \"void\");\n", e->type->cxxTypeRefInfo->fieldName);
		fprintf (src, "  return TCL_OK;\n");
		fprintf (src, "}\n\n");

		fprintf (src, "int %s::TclGetVal (Tcl_Interp *interp) const\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");
		fprintf (src, "  switch (choiceId)\n");
		fprintf (src, "  {\n");
		FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
		{
			fprintf (src, "    case %sCid:\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "      if (%s)\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "        if (%s->TclGetVal (interp) != TCL_OK)\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "          return TCL_ERROR;\n");
			fprintf (src, "      break;\n");
		}
		fprintf (src, "    default:\n");
		fprintf (src, "      Tcl_SetResult (interp, \"illegal choiceId in %s\", TCL_STATIC);\n", td->cxxTypeDefInfo->className);
		fprintf (src, "      Tcl_SetErrorCode (interp, \"SNACC\", \"ILLCHOICE\", NULL);\n");
		fprintf (src, "      return TCL_ERROR;\n");
		fprintf (src, "  }\n");
		fprintf (src, "  Tcl_DString valstr;\n");
		fprintf (src, "  Tcl_DStringInit (&valstr);\n");
		fprintf (src, "  Tcl_DStringAppendElement (&valstr, (char*)_desc.choicebyvalue (choiceId));\n");
		fprintf (src, "  Tcl_DStringAppendElement (&valstr, interp->result);\n");
		fprintf (src, "  Tcl_ResetResult (interp);\n");
		fprintf (src, "  Tcl_DStringResult (interp, &valstr);\n");
		fprintf (src, "  return TCL_OK;\n");
		fprintf (src, "}\n\n");

		fprintf (src, "int %s::TclSetVal (Tcl_Interp *interp, const char *valstr)\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");
		fprintf (src, "  Args elem;\n");
		fprintf (src, "  if (Tcl_SplitList (interp, (char*)valstr, &elem.c, &elem.v) != TCL_OK)\n");
		fprintf (src, "    return TCL_ERROR;\n");
		fprintf (src, "  if (elem.c != 2)\n");
		fprintf (src, "  {\n");
		fprintf (src, "    sprintf (interp->result, \"syntax error: expected a pair, but it's got %%d element(s)\", elem.c);\n");
		fprintf (src, "    return TCL_ERROR;\n");
		fprintf (src, "  }\n");
		fprintf (src, "  AsnType *member = _getref (elem.v[0], true);\n");
		fprintf (src, "  if (!member)\n");
		fprintf (src, "  {\n");
		fprintf (src, "    Tcl_AppendResult (interp, \"illegal choice \", elem.v[0], \" for %s\", NULL);\n", td->cxxTypeDefInfo->className);
		fprintf (src, "    Tcl_SetErrorCode (interp, \"SNACC\", \"ILLCHOICE\", NULL);\n");
		fprintf (src, "    return TCL_ERROR;\n");
		fprintf (src, "  }\n");
		fprintf (src, "  return member->TclSetVal (interp, elem.v[1]);\n");
		fprintf (src, "}\n\n");

		fprintf (hdr, "#endif // TCL\n");
		fprintf (src, "#endif // TCL\n\n");
	}
#endif /* TCL */

	fprintf (hdr, "#endif // META\n");
	fprintf (src, "#endif // META\n\n");
#endif /* META */
}

void PrintCxxSeqDefCodeMeta_1(FILE* hdr, FILE* src, TypeDef* td, Type* seq, Module* m, NamedType* e)
{
#if META
	fprintf (hdr, "#if META\n");
	fprintf (src, "#if META\n\n");

	fprintf (hdr, "  static const AsnSequenceTypeDesc	_desc;\n");
	fprintf (hdr, "  static const AsnSequenceMemberDesc	_mdescs[];\n");
	fprintf (hdr, "  const AsnTypeDesc		*_getdesc() const;\n");
	fprintf (hdr, "  AsnType			*_getref (const char *membername, bool create = false);\n\n");

	fprintf (src, "static AsnType *create%s()\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  return new %s;\n", td->cxxTypeDefInfo->className);
	fprintf (src, "}\n\n");

	fprintf (src, "const AsnSequenceMemberDesc %s::_mdescs[] =\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
		if (printMetaG == META_backend_names)
			fprintf (src, "  AsnSequenceMemberDesc (\"%s\", &%s::_desc, %s), // `%s'\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className, e->type->optional || e->type->defaultVal ? "true" : "false", e->fieldName ? e->fieldName : "");
		else /* META_asn1_names */
			fprintf (src, "  AsnSequenceMemberDesc (\"%s\", &%s::_desc, %s), // `%s'\n", e->fieldName ? e->fieldName : e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className, e->type->optional || e->type->defaultVal ? "true" : "false", e->type->cxxTypeRefInfo->fieldName);
	fprintf (src, "  AsnSequenceMemberDesc()\n");
	fprintf (src, "};\n\n");

	fprintf (src, "const AsnSequenceTypeDesc %s::_desc\n", td->cxxTypeDefInfo->className);
	fprintf (src, "(\n");
	fprintf (src, "  &%sModuleDesc,\n", m->cxxname);
	if (printMetaG == META_backend_names)
		fprintf (src, "  \"%s\", // `%s'\n", td->cxxTypeDefInfo->className, td->definedName);
	else /* META_asn1_names */
		fprintf (src, "  \"%s\", // `%s'\n", td->definedName, td->cxxTypeDefInfo->className);
	fprintf (src, "  %s,\n", isMetaPDU (m->modId->name, td->definedName, meta_pdus_G) ? "true" : "false");
	fprintf (src, "  AsnTypeDesc::SEQUENCE,\n");
	fprintf (src, "  create%s,\n", td->cxxTypeDefInfo->className);
	fprintf (src, "  _mdescs\n");
	fprintf (src, ");\n\n");

	fprintf (src, "const AsnTypeDesc *%s::_getdesc() const\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  return &_desc;\n");
	fprintf (src, "}\n\n");

	fprintf (src, "AsnType *%s::_getref (const char *membername, bool create)\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
	{
		fprintf (src, "  if (!strcmp (membername, \"%s\"))\n", e->type->cxxTypeRefInfo->fieldName);
		if (e->type->cxxTypeRefInfo->isPtr)
		{
			fprintf (src, "  {\n");
			fprintf (src, "    if (!%s && create)\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "      %s = new %s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className);
			fprintf (src, "    return %s;\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "  }\n");
		}
		else
			fprintf (src, "    return &%s;\n", e->type->cxxTypeRefInfo->fieldName);
	}
	fprintf (src, "  return NULL;\n");
	fprintf (src, "}\n\n");

#if TCL
	if (printTclG)
	{
		fprintf (hdr, "#if TCL\n");
		fprintf (src, "#if TCL\n\n");

		fprintf (hdr, "  int			TclGetDesc (Tcl_DString *) const;\n");
		fprintf (hdr, "  int			TclGetVal (Tcl_Interp *) const;\n");
		fprintf (hdr, "  int			TclSetVal (Tcl_Interp *, const char *valstr);\n");
		fprintf (hdr, "  int			TclUnsetVal (Tcl_Interp *, const char *membname);\n\n");

		fprintf (src, "int %s::TclGetDesc (Tcl_DString *valstr) const\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");
		fprintf (src, "  Tcl_DStringStartSublist (valstr);\n\n");
		FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
		{
			fprintf (src, "  Tcl_DStringStartSublist (valstr);\n");
			fprintf (src, "  Tcl_DStringAppendElement (valstr, \"%s\");\n", e->type->cxxTypeRefInfo->fieldName);
			if (e->type->cxxTypeRefInfo->isPtr)
				fprintf (src, "  Tcl_DStringAppendElement (valstr, %s ? \"valid\" : \"void\");\n", e->type->cxxTypeRefInfo->fieldName);
			else
				fprintf (src, "  Tcl_DStringAppendElement (valstr, %s \"valid\");\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "  Tcl_DStringEndSublist (valstr);\n\n");
		}
		fprintf (src, "  Tcl_DStringEndSublist (valstr);\n\n");
		fprintf (src, "  return TCL_OK;\n");
		fprintf (src, "}\n\n");

		fprintf (src, "int %s::TclGetVal (Tcl_Interp *interp) const\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");
		fprintf (src, "  Tcl_DString valstr;\n\n");
		fprintf (src, "  Tcl_DStringInit (&valstr);\n\n");
		FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
		{
			if (e->type->cxxTypeRefInfo->isPtr)
			{
				fprintf (src, "  if (%s)\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "  {\n");
				fprintf (src, "    Tcl_DStringStartSublist (&valstr);\n");
				fprintf (src, "    Tcl_DStringAppendElement (&valstr, \"%s\");\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "    if (%s->TclGetVal (interp) != TCL_OK)\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "      goto Error;\n");
				fprintf (src, "    Tcl_DStringAppendElement (&valstr, interp->result);\n");
				fprintf (src, "    Tcl_ResetResult (interp);\n");
				fprintf (src, "    Tcl_DStringEndSublist (&valstr);\n");
				fprintf (src, "  }\n\n");
			}
			else
			{
				fprintf (src, "  Tcl_DStringStartSublist (&valstr);\n");
				fprintf (src, "  Tcl_DStringAppendElement (&valstr, \"%s\");\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "  if (%s.TclGetVal (interp) != TCL_OK)\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "    goto Error;\n");
				fprintf (src, "  Tcl_DStringAppendElement (&valstr, interp->result);\n");
				fprintf (src, "  Tcl_ResetResult (interp);\n");
				fprintf (src, "  Tcl_DStringEndSublist (&valstr);\n\n");
			}
		}
		fprintf (src, "  Tcl_DStringResult (interp, &valstr);\n");
		fprintf (src, "  return TCL_OK;\n\n");
		fprintf (src, "Error:\n");
		fprintf (src, "  Tcl_DStringFree (&valstr);\n");
		fprintf (src, "  return TCL_ERROR;\n");
		fprintf (src, "}\n\n");

		fprintf (src, "int %s::TclSetVal (Tcl_Interp *interp, const char *valstr)\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");
		fprintf (src, "  int i;\n");
		fprintf (src, "  Args elems;\n");
		fprintf (src, "  if (Tcl_SplitList (interp, (char*)valstr, &elems.c, &elems.v) != TCL_OK)\n");
		fprintf (src, "    return TCL_ERROR;\n");
		fprintf (src, "  Args* elempairs = new Args[elems.c];\n");
		fprintf (src, "  for (i=0; i<elems.c; i++)\n");
		fprintf (src, "  {\n");
		fprintf (src, "    if (Tcl_SplitList (interp, elems.v[i], &elempairs[i].c, &elempairs[i].v) != TCL_OK)\n");
		fprintf (src, "    {\n");
		fprintf (src, "      delete elempairs;\n");
		fprintf (src, "      return TCL_ERROR;\n");
		fprintf (src, "    }\n");
		fprintf (src, "    if (elempairs[i].c != 2)\n");
		fprintf (src, "    {\n");
		fprintf (src, "      sprintf (interp->result, \"syntax error in element #%%d: expected a pair, but it's got %%d element(s)\", i, elempairs[i].c);\n");
		fprintf (src, "      delete elempairs;\n");
		fprintf (src, "      return TCL_ERROR;\n");
		fprintf (src, "    }\n");
		fprintf (src, "  }\n");
		fprintf (src, "\n");
		fprintf (src, "  for (const AsnSequenceMemberDesc *m=_mdescs; m->name; m++)\n");
		fprintf (src, "  {\n");
		fprintf (src, "    int count = 0;\n");
		fprintf (src, "    for (i=0; i<elems.c; i++)\n");
		fprintf (src, "      if (!strcmp (elempairs[i].v[0], m->name))\n");
		fprintf (src, "        count++;\n");
		fprintf (src, "    if (count > 1)\n");
		fprintf (src, "    {\n");
		fprintf (src, "      sprintf (interp->result, \"duplicate value for member \\\"%%s\\\" in list\", m->name);\n");
		fprintf (src, "      Tcl_SetErrorCode (interp, \"SNACC\", \"DUPMEMB\", NULL);\n");
		fprintf (src, "      delete elempairs;\n");
		fprintf (src, "      return TCL_ERROR;\n");
		fprintf (src, "    }\n");
		fprintf (src, "    if (!m->optional && count < 1)\n");
		fprintf (src, "    {\n");
		fprintf (src, "      sprintf (interp->result, \"mandatory member \\\"%%s\\\" is missing in list\", m->name);\n");
		fprintf (src, "      Tcl_SetErrorCode (interp, \"SNACC\", \"MISSMAND\", NULL);\n");
		fprintf (src, "      delete elempairs;\n");
		fprintf (src, "      return TCL_ERROR;\n");
		fprintf (src, "    }\n");
		fprintf (src, "    \n");
		fprintf (src, "  }\n");
		fprintf (src, "\n");
		fprintf (src, "  for (i=0; i<elems.c; i++)\n");
		fprintf (src, "  {\n");
		FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
		{
			fprintf (src, "    if (!strcmp (elempairs[i].v[0], \"%s\"))\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "    {\n");
			if (e->type->cxxTypeRefInfo->isPtr)
			{
				fprintf (src, "      if (!%s)\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "        %s = new %s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className);
				fprintf (src, "      if (%s->TclSetVal (interp, elempairs[i].v[1]))\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "      {\n");
				fprintf (src, "        delete elempairs;\n");
				fprintf (src, "        return TCL_ERROR;\n");
				fprintf (src, "      }\n");
			}
			else
			{
				fprintf (src, "      if (%s.TclSetVal (interp, elempairs[i].v[1]))\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "      {\n");
				fprintf (src, "        delete elempairs;\n");
				fprintf (src, "        return TCL_ERROR;\n");
				fprintf (src, "      }\n");
			}
			fprintf (src, "    }\n");
		}
		fprintf (src, "  }\n");
		fprintf (src, "\n");
		fprintf (src, "  // look for unmentioned optional members and delete them:\n");
		FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
		{
			if (e->type->optional || e->type->defaultVal)
			{
				fprintf (src, "  {\n");
				fprintf (src, "    bool present = false;\n");
				fprintf (src, "    for (i=0; i<elems.c; i++)\n");
				fprintf (src, "      if (!strcmp (elempairs[i].v[0], \"%s\"))\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "        present = true;\n");
				fprintf (src, "    if (!present)\n");
				fprintf (src, "    {\n");
				fprintf (src, "      delete %s;\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "      %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "    }\n");
				fprintf (src, "  }\n");
			}
		}
		fprintf (src, "\n");
		fprintf (src, "  delete elempairs;\n");
		fprintf (src, "  return TCL_OK;\n");
		fprintf (src, "}\n\n");

		fprintf (src, "int %s::TclUnsetVal (Tcl_Interp *interp, const char *membernames)\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");
		fprintf (src, "  Args elems;\n");
		fprintf (src, "  if (Tcl_SplitList (interp, (char*)membernames, &elems.c, &elems.v) != TCL_OK)\n");
		fprintf (src, "    return TCL_ERROR;\n");
		fprintf (src, "\n");
		fprintf (src, "  for (int i=0; i<elems.c; i++)\n");
		fprintf (src, "  {\n");
		FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
		{
			fprintf (src, "    if (!strcmp (elems.v[i], \"%s\"))\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "    {\n");
			if (e->type->optional || e->type->defaultVal)
			{
				fprintf (src, "        delete %s;\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "        %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
			}
			else
			{
				fprintf (src, "        return _desc.mandatmemberr (interp, elems.v[i]);\n");
			}
			fprintf (src, "    }\n");
		}
		fprintf (src, "  }\n");
		fprintf (src, "  return TCL_OK;\n");
		fprintf (src, "}\n\n");

		fprintf (hdr, "#endif // TCL\n");
		fprintf (src, "#endif // TCL\n\n");
	}
#endif /* TCL */

	fprintf (hdr, "#endif // META\n\n");
	fprintf (src, "#endif // META\n\n");

#endif /* META*/
}

void PrintCxxSetDefCodeMeta_1(FILE* hdr, FILE* src, TypeDef* td, Type* set, Module* m, NamedType* e)
{
#if META
	fprintf (hdr, "#if META\n");
	fprintf (src, "#if META\n\n");

	fprintf (hdr, "  static const AsnSetTypeDesc	_desc;\n");
	fprintf (hdr, "  static const AsnSetMemberDesc	_mdescs[];\n");
	fprintf (hdr, "  const AsnTypeDesc		*_getdesc() const;\n");
	fprintf (hdr, "  AsnType			*_getref (const char *membername, bool create = false);\n\n");

	fprintf (src, "static AsnType *create%s()\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  return new %s;\n", td->cxxTypeDefInfo->className);
	fprintf (src, "}\n\n");

	fprintf (src, "const AsnSetMemberDesc %s::_mdescs[] =\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
		if (printMetaG == META_backend_names)
			fprintf (src, "  AsnSetMemberDesc (\"%s\", &%s::_desc, %s), // `%s'\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className, e->type->optional || e->type->defaultVal ? "true" : "false", e->fieldName ? e->fieldName : "");
		else /* META_asn1_names */
			fprintf (src, "  AsnSetMemberDesc (\"%s\", &%s::_desc, %s), // `%s'\n", e->fieldName ? e->fieldName : e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className, e->type->optional || e->type->defaultVal ? "true" : "false", e->type->cxxTypeRefInfo->fieldName);
	fprintf (src, "  AsnSetMemberDesc()\n");
	fprintf (src, "};\n\n");

	fprintf (src, "const AsnSetTypeDesc %s::_desc\n", td->cxxTypeDefInfo->className);
	fprintf (src, "(\n");
	fprintf (src, "  &%sModuleDesc,\n", m->cxxname);
	if (printMetaG == META_backend_names)
		fprintf (src, "  \"%s\", // `%s'\n", td->cxxTypeDefInfo->className, td->definedName);
	else /* META_asn1_names */
		fprintf (src, "  \"%s\", // `%s'\n", td->definedName, td->cxxTypeDefInfo->className);
	fprintf (src, "  %s,\n", isMetaPDU (m->modId->name, td->definedName, meta_pdus_G) ? "true" : "false");
	fprintf (src, "  AsnTypeDesc::SET,\n");
	fprintf (src, "  create%s,\n", td->cxxTypeDefInfo->className);
	fprintf (src, "  _mdescs\n");
	fprintf (src, ");\n\n");

	fprintf (src, "const AsnTypeDesc *%s::_getdesc() const\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	fprintf (src, "  return &_desc;\n");
	fprintf (src, "}\n\n");

	fprintf (src, "AsnType *%s::_getref (const char *membername, bool create)\n", td->cxxTypeDefInfo->className);
	fprintf (src, "{\n");
	FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
	{
		fprintf (src, "  if (!strcmp (membername, \"%s\"))\n", e->type->cxxTypeRefInfo->fieldName);
		if (e->type->cxxTypeRefInfo->isPtr)
		{
			fprintf (src, "  {\n");
			fprintf (src, "    if (!%s && create)\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "      %s = new %s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className);
			fprintf (src, "    return %s;\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "  }\n");
		}
		else
			fprintf (src, "    return &%s;\n", e->type->cxxTypeRefInfo->fieldName);
	}
	fprintf (src, "  return NULL;\n");
	fprintf (src, "}\n\n");

#if TCL
	if (printTclG)
	{
		fprintf (hdr, "#if TCL\n");
		fprintf (src, "#if TCL\n\n");

		fprintf (hdr, "  int			TclGetDesc (Tcl_DString *) const;\n");
		fprintf (hdr, "  int			TclGetVal (Tcl_Interp *) const;\n");
		fprintf (hdr, "  int			TclSetVal (Tcl_Interp *, const char *valstr);\n");
		fprintf (hdr, "  int			TclUnsetVal (Tcl_Interp *, const char *membernames);\n\n");

		fprintf (src, "int %s::TclGetDesc (Tcl_DString *valstr) const\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");
		fprintf (src, "  Tcl_DStringStartSublist (valstr);\n\n");
		FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
		{
			fprintf (src, "  Tcl_DStringStartSublist (valstr);\n");
			fprintf (src, "  Tcl_DStringAppendElement (valstr, \"%s\");\n", e->type->cxxTypeRefInfo->fieldName);
			if (e->type->cxxTypeRefInfo->isPtr)
				fprintf (src, "  Tcl_DStringAppendElement (valstr, %s ? \"valid\" : \"void\");\n", e->type->cxxTypeRefInfo->fieldName);
			else
				fprintf (src, "  Tcl_DStringAppendElement (valstr, \"valid\");\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "  Tcl_DStringEndSublist (valstr);\n\n");
		}
		fprintf (src, "  Tcl_DStringEndSublist (valstr);\n\n");
		fprintf (src, "  return TCL_OK;\n");
		fprintf (src, "}\n\n");

		fprintf (src, "int %s::TclGetVal (Tcl_Interp *interp) const\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");
		fprintf (src, "  Tcl_DString valstr;\n\n");
		fprintf (src, "  Tcl_DStringInit (&valstr);\n\n");
		FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
		{
			if (e->type->cxxTypeRefInfo->isPtr)
			{
				fprintf (src, "  if (%s)\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "  {\n");
				fprintf (src, "    Tcl_DStringStartSublist (&valstr);\n");
				fprintf (src, "    Tcl_DStringAppendElement (&valstr, \"%s\");\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "    if (%s->TclGetVal (interp) != TCL_OK)\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "      goto Error;\n");
				fprintf (src, "    Tcl_DStringAppendElement (&valstr, interp->result);\n");
				fprintf (src, "    Tcl_ResetResult (interp);\n");
				fprintf (src, "    Tcl_DStringEndSublist (&valstr);\n");
				fprintf (src, "  }\n\n");
			}
			else
			{
				fprintf (src, "  Tcl_DStringStartSublist (&valstr);\n");
				fprintf (src, "  Tcl_DStringAppendElement (&valstr, \"%s\");\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "  if (%s.TclGetVal (interp) != TCL_OK)\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "    goto Error;\n");
				fprintf (src, "  Tcl_DStringAppendElement (&valstr, interp->result);\n");
				fprintf (src, "  Tcl_ResetResult (interp);\n");
				fprintf (src, "  Tcl_DStringEndSublist (&valstr);\n\n");
			}
		}
		fprintf (src, "  Tcl_DStringResult (interp, &valstr);\n");
		fprintf (src, "  return TCL_OK;\n\n");
		fprintf (src, "Error:\n");
		fprintf (src, "  Tcl_DStringFree (&valstr);\n");
		fprintf (src, "  return TCL_ERROR;\n");
		fprintf (src, "}\n\n");

		fprintf (src, "int %s::TclSetVal (Tcl_Interp *interp, const char *valstr)\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");
		fprintf (src, "  int i;\n");
		fprintf (src, "  Args elems;\n");
		fprintf (src, "  if (Tcl_SplitList (interp, (char*)valstr, &elems.c, &elems.v) != TCL_OK)\n");
		fprintf (src, "    return TCL_ERROR;\n");
		fprintf (src, "  Args* elempairs = new Args[elems.c];\n");
		fprintf (src, "  for (i=0; i<elems.c; i++)\n");
		fprintf (src, "  {\n");
		fprintf (src, "    if (Tcl_SplitList (interp, elems.v[i], &elempairs[i].c, &elempairs[i].v) != TCL_OK)\n");
		fprintf (src, "    {\n");
		fprintf (src, "      delete elempairs;\n");
		fprintf (src, "      return TCL_ERROR;\n");
		fprintf (src, "    }\n");
		fprintf (src, "    if (elempairs[i].c != 2)\n");
		fprintf (src, "    {\n");
		fprintf (src, "      sprintf (interp->result, \"syntax error in element #%%d: expected a pair, but it's got %%d element(s)\", i, elempairs[i].c);\n");
		fprintf (src, "      delete elempairs;\n");
		fprintf (src, "      return TCL_ERROR;\n");
		fprintf (src, "    }\n");
		fprintf (src, "  }\n");
		fprintf (src, "\n");
		fprintf (src, "  for (const AsnSetMemberDesc *m=_mdescs; m->name; m++)\n");
		fprintf (src, "  {\n");
		fprintf (src, "    int count = 0;\n");
		fprintf (src, "    for (i=0; i<elems.c; i++)\n");
		fprintf (src, "      if (!strcmp (elempairs[i].v[0], m->name))\n");
		fprintf (src, "        count++;\n");
		fprintf (src, "    if (count > 1)\n");
		fprintf (src, "    {\n");
		fprintf (src, "      sprintf (interp->result, \"duplicate value for member \\\"%%s\\\" in list\", m->name);\n");
		fprintf (src, "      Tcl_SetErrorCode (interp, \"SNACC\", \"DUPMEMB\", NULL);\n");
		fprintf (src, "      delete elempairs;\n");
		fprintf (src, "      return TCL_ERROR;\n");
		fprintf (src, "    }\n");
		fprintf (src, "    if (!m->optional && count < 1)\n");
		fprintf (src, "    {\n");
		fprintf (src, "      sprintf (interp->result, \"mandatory member \\\"%%s\\\" is missing in list\", m->name);\n");
		fprintf (src, "      Tcl_SetErrorCode (interp, \"SNACC\", \"MISSMAND\", NULL);\n");
		fprintf (src, "      delete elempairs;\n");
		fprintf (src, "      return TCL_ERROR;\n");
		fprintf (src, "    }\n");
		fprintf (src, "    \n");
		fprintf (src, "  }\n");
		fprintf (src, "\n");
		fprintf (src, "  for (i=0; i<elems.c; i++)\n");
		fprintf (src, "  {\n");
		FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
		{
			fprintf (src, "    if (!strcmp (elempairs[i].v[0], \"%s\"))\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "    {\n");
			if (e->type->cxxTypeRefInfo->isPtr)
			{
				fprintf (src, "      if (!%s)\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "        %s = new %s;\n", e->type->cxxTypeRefInfo->fieldName, e->type->cxxTypeRefInfo->className);
				fprintf (src, "      if (%s->TclSetVal (interp, elempairs[i].v[1]))\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "      {\n");
				fprintf (src, "        delete elempairs;\n");
				fprintf (src, "        return TCL_ERROR;\n");
				fprintf (src, "      }\n");
			}
			else
			{
				fprintf (src, "      if (%s.TclSetVal (interp, elempairs[i].v[1]))\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "      {\n");
				fprintf (src, "        return TCL_ERROR;\n");
				fprintf (src, "        delete elempairs;\n");
				fprintf (src, "      }\n");
			}
			fprintf (src, "    }\n");
		}
		fprintf (src, "  }\n");
		fprintf (src, "\n");
		fprintf (src, "  // look for unmentioned optional members and delete them:\n");
		FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
		{
			if (e->type->optional || e->type->defaultVal)
			{
				fprintf (src, "  {\n");
				fprintf (src, "    bool present = false;\n");
				fprintf (src, "    for (i=0; i<elems.c; i++)\n");
				fprintf (src, "      if (!strcmp (elempairs[i].v[0], \"%s\"))\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "        present = true;\n");
				fprintf (src, "    if (!present)\n");
				fprintf (src, "    {\n");
				fprintf (src, "      delete %s;\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "      %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "    }\n");
				fprintf (src, "  }\n");
			}
		}
		fprintf (src, "\n");
		fprintf (src, "  delete elempairs;\n");
		fprintf (src, "  return TCL_OK;\n");
		fprintf (src, "}\n\n");

		fprintf (src, "int %s::TclUnsetVal (Tcl_Interp *interp, const char *membernames)\n", td->cxxTypeDefInfo->className);
		fprintf (src, "{\n");
		fprintf (src, "  Args elems;\n");
		fprintf (src, "  if (Tcl_SplitList (interp, (char*)membernames, &elems.c, &elems.v) != TCL_OK)\n");
		fprintf (src, "    return TCL_ERROR;\n");
		fprintf (src, "\n");
		fprintf (src, "  for (int i=0; i<elems.c; i++)\n");
		fprintf (src, "  {\n");
		FOR_EACH_LIST_ELMT (e, set->basicType->a.set)
		{
			fprintf (src, "    if (!strcmp (elems.v[i], \"%s\"))\n", e->type->cxxTypeRefInfo->fieldName);
			fprintf (src, "    {\n");
			if (e->type->optional || e->type->defaultVal)
			{
				fprintf (src, "      delete %s;\n", e->type->cxxTypeRefInfo->fieldName);
				fprintf (src, "      %s = NULL;\n", e->type->cxxTypeRefInfo->fieldName);
			}
			else
			{
				fprintf (src, "      return _desc.mandatmemberr (interp, elems.v[i]);\n");
			}
			fprintf (src, "    }\n");
		}
		fprintf (src, "  }\n");
		fprintf (src, "  return TCL_OK;\n");
		fprintf (src, "}\n\n");

		fprintf (hdr, "#endif // TCL\n");
		fprintf (src, "#endif // TCL\n\n");
	}
#endif /* TCL */
	fprintf (hdr, "#endif // META\n\n");
	fprintf (src, "#endif // META\n\n");
#endif /* META */
}
/* EOF gen-code.c (for back-ends/c++-gen) */

