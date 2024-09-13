/*
 * compiler/core/snacc.c---Compiles ASN.1 src files into an internal type tree.
 *        Imported type/value references are resolved if possible.
 *        Produces C or C++ encoder/decoder/print/free code and .h for
 *        data struct and prototypes.
 *        Generated C can be either ANSI or old style via macros.
 *        Produces values for OBJECT IDENTIFIERs, INTEGERs and BOOLEANs
 *
 * Copyright (C) 1991, 1992 Michael Sample
 *            and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program and the associated libraries are distributed in the hope
 * that they will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License and GNU Library General
 * Public License for more details.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/core/snacc.c,v 1.3 2005/11/09 15:47:42 \ste Exp $
 *
 */

int gNO_NAMESPACE = 0;
const char* gAlternateNamespaceString = 0;

/*  DEFAULT no DLL Export of SNACC  built classes.
 */
char* bVDAGlobalDLLExport = (char*)0;

#include <ctype.h>

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#endif // _WIN32

#include "../../c-lib/include/asn-incl.h"
#include <string.h>

#include "mem.h"
#include "asn1module.h"
#include "exports.h"
#include "enc-rules.h"
#include "print.h"
#include "define.h"
#include "snacc-util.h"
#include "snacc-validators.h"
#include "filetype.h"
#include "../back-ends/structure-util.h"
#include "../back-ends/str-util.h"
#include "efileressources.h"
#include "cpp_c_helper.h"
#include "time_helpers.h"
#if META
#include "meta.h"
#endif

#include "../back-ends/c-gen/rules.h" /* for c file generation */
#include "../back-ends/c-gen/type-info.h"

#include "../back-ends/c++-gen/rules.h" /* for c++ file generation */

#if IDL
#include "../back-ends/idl-gen/rules.h" /* for idl file generation */
#endif

#include "../back-ends/c++-gen/gen-code.h"
#include "../back-ends/java-gen/gen-java-code.h"
#include "../back-ends/cs-gen/gen-code.h"
#include "../back-ends/swift-gen/gen-swift-code.h"
#include "../back-ends/js-gen/gen-js-code.h"
#include "../back-ends/ts-gen/gen-ts-code.h"
#include "../back-ends/delphi-gen/gen-delphi-code.h"
#include "../back-ends/jsondoc-gen/gen-jsondoc-code.h"
#include "../back-ends/openapi-gen/gen-openapi-code.h"
#include "asn_comments.h"

/* ******************* */
/* Function Prototypes */
/* ******************* */

void ErrChkModule PROTO((Module * m));
void FillCxxTypeInfo PROTO((CxxRules * r, ModuleList* m));
void FillIDLTypeInfo PROTO((IDLRules * r, ModuleList* modList));
void GenTypeTbls PROTO((ModuleList * mods, const char* fileName, int tableFileVersion));
int InitAsn1Parser PROTO((Module * mod, const char* fileName, FILE* fPtr));
int LinkTypeRefs PROTO((ModuleList * m));
int LinkValueRefs PROTO((ModuleList * m));
void MarkRecursiveTypes PROTO((Module * m));
void NormalizeModule PROTO((Module * m));
void NormalizeValue PROTO((Module * m, ValueDef* vd, Value* v, int quiet));
int ParseValues PROTO((ModuleList * mods, Module* m));
void PrintCCode PROTO((FILE * src, FILE* hdr, ModuleList* mods, Module* m, CRules* r, long int longJmpVal, int printTypes, int printValues, int printEncoders, int printDecoders, int printPrinters, int printFree));
void PrintIDLCode PROTO((FILE * idl, ModuleList* mods, Module* m, IDLRules* r, long int longJmpVal, int printValues));
void ProcessMacros PROTO((Module * m));
void SortAllDependencies PROTO((ModuleList * m));
int yyparse();
int copy_file(const char* source, const char* destination);

/* Internal routines */
void CreateNames(ModuleList* allMods);
static int GenCCode(ModuleList* allMods, long longJmpVal, int genTypes, int genEncoders, int genDecoders, int genPrinters, int genValues, int genFree);

static void GenCxxCode(ModuleList* allMods, long longJmpVal, int genTypes, int genEncoders, int genDecoders, int genJSONEncDec, int genPrinters, int genPrintersXML, int genValues, int genFree, const char* szCppHeaderIncludePath, if_META(MetaNameStyle genMeta COMMA MetaPDU* meta_pdus COMMA) if_TCL(int genTcl COMMA) int novolatilefuncs, int genROSEDecoders, const int genCxxCode_EnumClasses);

static void GenSwiftCode(ModuleList* allMods, long longJmpVal, int genTypes, int genEncoders, int genDecoders, int genJSONEncDec, int genPrinters, int genPrintersXML, int genValues, int genFree, int novolatilefuncs, int genROSEDecoders);

static void GenJSCode(ModuleList* allMods, long longJmpVal, int genTypes, int genEncoders, int genDecoders, int genJSONEncDec, int genPrinters, int genPrintersXML, int genValues, int genFree, int novolatilefuncs, int genROSEDecoders);

static void GenOpenApiCode(ModuleList* allMods, long longJmpVal, int genTypes, int genEncoders, int genDecoders, int genJSONEncDec, int genPrinters, int genPrintersXML, int genValues, int genFree, int novolatilefuncs, int genROSEDecoders);

static void GenTSCode(ModuleList* allMods, long longJmpVal, int genTypes, int genJSONEncDec, int genTSROSEStubs, int genPrinters, int genPrintersXML, int genValues, int genFree, int novolatilefuncs, int genROSEDecoders);

static void GenJsonDocCode(ModuleList* allMods);

static void GenCSCode(ModuleList* allMods, int genROSECSDecoders);

static void GenJAVACode(ModuleList* allMods, int genROSEJAVADecoders);
static void GenDelphiCode(ModuleList* allMods, long longJmpVal, int genTypes, int genEncoders, int genDecoders, int genJSONEncDec, int genPrinters, int genPrintersXML, int genValues, int genFree, int novolatilefuncs, int genROSEDecoders);

static void GenIDLCode(ModuleList* allMods, long longJmpVal, int genTypes, int genPrinters, int genValues, int genFree);
static int ModNamesUnique(ModuleList* m);
static Module* ParseAsn1File(const char* fileName, short ImportFlag, int parseComments);

#if META
static MetaPDU* parse_type_list PROTO(char* arg));
#endif /* META */

/* **************** */
/* Global Variables */
/* **************** */
extern int anyEnumValG;

extern int smallErrG; /* can continue processing but don't produce code - see more errs */
/*RWC;extern int	yydebug; / * set to 1 to enable debugging */
extern int yydebug;

int maxFileNameLenG = -1; /* values > 2 are considered valid */
/* this is used in back_ends/c_gen/str_util.c */

FILE* errFileG = NULL; /* Pointer to file for reporting errors */

int genPERCode = FALSE;

int isSyntax1997 = 1;			  /* Deepak: 24/Mar/2003	Syntax Checking for 1990/1997,
									  defaults to 1997 */
int isTableConstraintAllowed = 1; /* Deepak: 25/Mar/2003 */
int isInWithSyntax = 0;			  /* Deepak: 26/Mar/2003 */

// ste 9.9.2016 - allow private declared symbols to be suppressed
int gPrivateSymbols = 1;

// Defines the node version the typescript stub will be generated for
int gNodeVersion = 22;

// Defines the major interface version the interfaces are built with
// The version is read from a file interfaceversion.txt when the first asn1 is read
int gMajorInterfaceVersion = -1;

// jan 10.1.2023 - if set deprecated symbols are removed from the generated code
//  The value contains a timestamp, either specified by the command line or set to 1 (if nodeprecated has been set!)
//  The comment parser reads @deprecated flags in association to sequences, attributes and operations and stores them with the comment content
//  When we parse the @deprecated flags in the asn1 we also search for a timestamp next to it -> the value is converted to unix time and used for deprecated comparison
//  If no timestamp is found next to the deprecate a warning is generated to add a timestamp -> the unix time stamp value expressing the deprecation is set to 1 and used for deprecated comparison

// If the command line contains nodeprecated you may add a timestamp to the command line parameter: e.g. nodeprecated:31.05.2023
// This value is converted to unix timestamp and stored in gi64NoDeprecatedSymbols
// If no value has been specified all deprecated objects shall get remove from the output and therefore gi64NoDeprecatedSymbols is set to 1

// Any deprecated information from a sequence, attribute or operation comment lower or equal than the gi64NoDeprecatedSymbols will get removed
long long gi64NoDeprecatedSymbols = 0;

// jan 11.1.2023 - Default level for validating the content of the asn1 files is all checks that the tool knows
int giValidationLevel = 0xffffffff;

// Write comments to the target files on true (parsing is always enabled)
int giWriteComments = 0;

// Write a combined version file
int genVersionFile = FALSE;

// Defines whether we write commonsJS or ESM typescript code
int genTSESMCode = FALSE;

int gFilterASN1Files = FALSE;
char gszOutputPath[_MAX_PATH] = {0};

#ifdef WIN_SNACC /* Deepak: 14/Feb/2003 */
#define main Win_Snacc_Main
#endif

void Usage PARAMS((prgName, fp), char* prgName _AND_ FILE* fp)
{
	fprintf(fp, "\nUsage: %s ", prgName);
	fprintf(fp, "[-h] [-P] [-t] [-v] [-e] [-d] [-p] [-f] [-y] [-M] [-b] [-R] \n");
#if IDL
	fprintf(fp, "            [-c | -C | -T <table output file> | -idl ]\n");
#else
	fprintf(fp, "            [-c | -C | -T <table output file>]\n");
#endif
	fprintf(fp, "            [-I <include directory>]\n");
	fprintf(fp, "            [-mm] [-mf <max file name length>]\n");
	fprintf(fp, "            [-l <neg number>]\n");
	fprintf(fp, "            [-VDAexport=DEFINE_NAME] to designate export of SNACC generated classes\n");
	fprintf(fp, "            [-E BER|DER select encoding rules to generate (C only)]\n");
	fprintf(fp, "            [-a <start number>] select starting number for ANYs\n");

#if META
	fprintf(fp, "            [-meta <type list>] [-mA | -mC]\n");
#if TCL
	fprintf(fp, "            [-tcl <type list>]\n");
#endif
#endif
	fprintf(fp, "            <ASN.1 file list>\n\n");
	fprintf(fp, "  -c   generate C encoders and decoders (default)\n");
	fprintf(fp, "  -C   generate C++ encoders and decoders\n");
	fprintf(fp, "  -Ce  Uses dedicated enum classes for enums instead of burried enums inside a class\n");
	fprintf(fp, "  -Ch  <path> header prefix for the generated cpp files (defaults to cpp-lib/include/ but you may e.g. define libs/snacclib5/cpp-lib/include/)\n");
	fprintf(fp, "  -S   generate Swift code\n");
	fprintf(fp, "  -j   generate JSON encoders/decoders. Use with -J or -JT.\n");
	fprintf(fp, "  -J   generate plain JavaScript code. For Java see -RJ.\n");
	fprintf(fp, "  -JT  generate Typescript CommonJS code.\n");
	fprintf(fp, "  -JD  generate JSON Documentation files.\n");
	fprintf(fp, "  -JO  generate OpenApi Documentation files.\n");
	fprintf(fp, "  -JTE  generate Typescript ES Module code.\n");
	fprintf(fp, "  -T   <filename> write a type table file for the ASN.1 modules to file filename\n");
	fprintf(fp, "  -O   <filename> writes the type table file in the original (<1.3b2) format\n");
	fprintf(fp, "  -O   <filename> writes the type table file in the original (<1.3b2) format\n");
	fprintf(fp, "  -o   write output to a different folder, default is the current folder\n");
	fprintf(fp, "  -b   turns on generation of PER support\n");
	fprintf(fp, "  -R   generate c++ ROSE stub code\n");
	fprintf(fp, "  -RCS generate c# ROSE stub code\n");
	fprintf(fp, "  -RJ  generate JAVA ROSE stub code. For JavaScript see -J.\n");
	fprintf(fp, "  -RTS_SERVER   generate Typescript ROSE server stub code.\n");
	fprintf(fp, "  -RTS_CLIENT_NODE   generate Typescript ROSE client stub code for node.\n");
	fprintf(fp, "  -RTS_CLIENT_BROWSER   generate Typescript ROSE client stub code for a browser.\n");
	fprintf(fp, "  -D   generate Delphi code\n");
#if IDL
	fprintf(fp, "  -idl generate CORBA IDL\n");
#endif
	fprintf(fp, "  -filter Filters the asn1 files (needs no private and or nodeprecated flags beeign set)\n");
	fprintf(fp, "  -node:21   Defines the node version the stub will be generated for. Defaults to 22\n");
	fprintf(fp, "  -noprivate   do not generate code that is marked as private\n");
	fprintf(fp, "  -nodeprecated   do not generate code that is marked as deprecated (any date)\n");
	fprintf(fp, "  -nodeprecated:Day.Month.Year  do not generate code that has been marked deprecated prior to this date\n");
	fprintf(fp, "  -ValidationLevel n - Sets a specific validation rule set for the asn1 files. Default is that all of the following checks are applied\n");
	fprintf(fp, "   0 no validation\n");
	fprintf(fp, "   1 Validates that operationIDs are not used twice\n");
	fprintf(fp, "   2 Validates that operation arguments, results and errors are sequences or choices (only types are extendable) (@deprecated are not validated)\n");
	fprintf(fp, "   4 Validates that errors are of the same type to generalize error handling (@deprecated are not validated)\n");
	fprintf(fp, "   8 Validates that all sequences contain ... to allow extending them (@deprecated are not validated)\n");
	fprintf(fp, "   16 Validates that only allow types from the esnacc_whiteliste.txt are used (@deprecated are not validated)\n");
	fprintf(fp, "   32 Ensure that invokes are specified with argument, result and error where events only consists of an argument (@deprecated are not validated)\n");
	fprintf(fp, "   64 Ensure that optional parameters are not encoded implicit (context specific, with number) and explicit (without) in the same object (@deprecated are not validated)\n");
	fprintf(fp, "   128 Ensure that optional parameters are always encoded implicit and never explicit (@deprecated are not validated)\n");
	fprintf(fp, "  -versionfile - the compiler writes a version file for the highest version found (requires interfaceversion.txt)\n");
	fprintf(fp, "  -h   prints this msg\n");
	fprintf(fp, "  -P   print the parsed ASN.1 modules to stdout from their parse trees\n");
	fprintf(fp, "       (helpful debugging)\n");
	fprintf(fp, "  -t   generate type definitions\n");
	fprintf(fp, "  -v   generate value definitions (limited)\n");
	fprintf(fp, "  -e   generate encode routines\n");
	fprintf(fp, "  -d   generate decode routines\n");
	fprintf(fp, "  -p   generate print routines\n");
	fprintf(fp, "  -x   generate XML print routines\n");
	fprintf(fp, "  -f   generate hierarchical free routines (C only)\n");
	fprintf(fp, "    Note: if none of -t -v -e -d -p -f are given, all are generated.\n");
	fprintf(fp, "    These do not affect type tables.\n");
	fprintf(fp, "  -y   enable bison debugging\n");
	fprintf(fp, "  -M   uses the 1990 Syntax, default is the 1997 Syntax\n");

	fprintf(fp, "  -mm  mangle output file name into module name (by default, the output file\n");
	fprintf(fp, "       inherits the input file's name, with only the suffix replaced)\n");
	fprintf(fp, "  -mf <num> num is maximum file name length for the generated source files\n");
	fprintf(fp, "  -comments  parse asn Files for comments and print to code\n");

	fprintf(fp, "  -l <neg num> where to start error longjmp values decending from (obscure).\n");
	fprintf(fp, "  -L <error log file> print syntax errors to the specified error log file\n");
	fprintf(fp, "                      (default is stderr)\n");

#if META
	fprintf(fp, "  -meta <type list> generate meta code that describes the generated types. Implies -C.\n");
	fprintf(fp, "  -mA  metacode: use names as defined in the ASN.1 files.\n");
	fprintf(fp, "  -mC  metacode: use names as used in the generated C++ files.\n");
#if TCL
	fprintf(fp, "  -tcl <type list> generate code for a Tcl interpreter. Implies -meta.\n");
#endif
	fprintf(fp, "    <type list> has the following syntax: <module>.<type>[,<module>.<type>[...]]\n");
	fprintf(fp, "    the types listed are the top level PDUs.\n");
#endif

	fprintf(fp, "\nUse `-' as the ASN.1 source file name to parse stdin.\n\n");

	fprintf(fp, "This ASN.1 compiler produces C or C++ BER encoders and decoders or type tables.\n");

	fprintf(fp, "\nVersion: %s\n", VERSION);
	fprintf(fp, "Release Date: %s\n", RELDATE);
	// fprintf (fp, "Please see %s for new versions and where to send bug reports and comments.\n\n", bugreportaddressG);

	fprintf(fp, "Copyright (C) 1993 Michael Sample and UBC\n");
	fprintf(fp, "Copyright (C) 1994, 1995 by Robert Joop and GMD FOKUS\n");
	fprintf(fp, "Improvements 2004-2016 estos GmbH, www.estos.com\n\n");

	fprintf(fp, "This program is free software; you can redistribute it and/or modify\n");
	fprintf(fp, "it under the terms of the GNU General Public License as published by\n");
	fprintf(fp, "the Free Software Foundation; either version 2 of the License, or\n");
	fprintf(fp, "(at your option) any later version.\n\n");

	fprintf(fp, "This program is distributed in the hope that it will be useful,\n");
	fprintf(fp, "but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	fprintf(fp, "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
	fprintf(fp, "GNU General Public License for more details.\n\n");

	fprintf(fp, "You should have received a copy of the GNU General Public License\n");
}

/****************/
/* main routine */
/****************/
int main PARAMS((argc, argv), int argc _AND_ char** argv)
{
	int semErr;
	int numSrcFiles;
	ModuleList* allMods = NULL;
	Module* currMod;
	Module** tmpModHndl;
	int currArg;
	int printModuleFlag = FALSE; /* default: Don't print */
	int genTypeTbls = 0;		 /* default: Don't gen tbls */
	const char* tblFileName = NULL;
	int encRulesSet = FALSE;
	int genTypeCode = FALSE;
	int genEncodeCode = FALSE;
	int genDecodeCode = FALSE;
	int genJSONEncDec = FALSE;
	int genPrintCode = FALSE;
	int genPrintCodeXML = FALSE;
	int genValueCode = FALSE;
	int genFreeCode = FALSE;
#if META
	MetaNameStyle genMetaCode = META_off;
	MetaPDU* meta_pdus = NULL;
#if TCL
	int genTclCode = FALSE;
#endif
#endif
	int genCCode = FALSE; /* defaults to C if neither specified */
	int genCxxCode = FALSE;
	int genCxxCode_EnumClasses = FALSE;
	int genIDLCode = FALSE;
	int genJAVACode = FALSE;   // JAVA
	int genDelphiCode = FALSE; // Delphi
	int genCSCode = FALSE;	   // c#
	int genSwiftCode = FALSE;
	int genJSCode = FALSE;
	int genOpenApiCode = FALSE;
	int genTSCommonJSCode = FALSE;
	int genJsonDocCode = FALSE;
	long longJmpVal = -100;
	int novolatilefuncs = FALSE;
	int genTSRoseStubs = FALSE;
	int genROSEDecoders = FALSE; /* ste -- 13.04.054 --added */
	const char* dirName;		 /* REN -- 6/2/03 -- added */
	const char* errFileName;	 /* REN -- 7/7/03 -- added */
	const char* szCppHeaderIncludePath = "cpp-lib/include/";

	if (argc <= 1)
	{
		Usage(argv[0], stderr);
		return 1;
	}

#ifndef _WIN32
	/**
	 * Ensure that time values are parsed as UTC and not using any other time zone
	 */
	setenv("TZ", "UTC", 1);
	tzset();
#endif

	/*
	 * parse cmd line args
	 */
	numSrcFiles = 0;
	for (currArg = 1; (currArg < argc);)
	{
		const char* argument = argv[currArg];
		if ((argument[0] == '-') && (argument[1] != '\0'))
		{
			switch (argument[1])
			{
				case 'h':
					Usage(argv[0], stdout);
					return 0;
					break;

				case 'M': // Deepak: 24/Mar/2003
					isSyntax1997 = 0;
					isTableConstraintAllowed = 0;
					currArg++;
					break;

				case 'a':					 /* AnyID start value */
					if (argument[2] != '\0') /* no space after -a */
					{
						anyEnumValG = atoi(&argument[2]);
						currArg++;
					}
					else
					{
						anyEnumValG = atoi(argv[currArg + 1]);
						currArg += 2;
					}
					break;

				case 'I':
					if (argument[2] != '\0')
						dirName = &argument[2];
					else
						dirName = argv[++currArg];
					if (!findFiles(dirName, true))
					{
						fprintf(stderr, "%s: ERROR---Unknown ASN Import Directory -I, found no files", dirName);
						Usage(argv[0], stderr);
						return 1;
					}
					currArg++;
					break;

				case 'P':
					printModuleFlag = TRUE;
					currArg++;
					break;

				case 'R':
					if (strcmp(argument + 1, "RCS") == 0)
					{
						/* asn --added */
						genCxxCode = FALSE;
						genCSCode = TRUE;
						genROSEDecoders = TRUE;
						currArg++;
					}
					else if (strcmp(argument + 1, "RJ") == 0)
					{
						/* stm --added */
						genCxxCode = FALSE;
						genJAVACode = TRUE;
						genROSEDecoders = TRUE;
						currArg++;
					}
					else if (strcmp(argument + 1, "RTS_SERVER") == 0)
					{
						genTSRoseStubs |= 0x01;
						currArg++;
					}
					else if (strcmp(argument + 1, "RTS_CLIENT_NODE") == 0)
					{
						genTSRoseStubs |= 0x02;
						currArg++;
					}
					else if (strcmp(argument + 1, "RTS_CLIENT_BROWSER") == 0)
					{
						genTSRoseStubs |= 0x04;
						currArg++;
					}
					else
					{
						/* ste --added */
						genROSEDecoders = TRUE;
						currArg++;
					}
					break;
				case 's':
					if (strcmp(argument + 1, "stdafx") == 0)
					{
						/* ste --added */
						genCodeCPPPrintStdAfxInclude = 1;
						currArg++;
					}
					break;
				case 'v':
					if (strcmp(argument + 1, "versionfile") == 0)
					{
						genVersionFile = 1;
						currArg++;
					}
					else if (strcmp(argument + 1, "v") == 0)
					{
						genValueCode = TRUE;
						currArg++;
					}
					else
						goto error;
					break;
#if IDL
				case 'i':
					if (strcmp(argument + 1, "idl") == 0)
					{
						genIDLCode = TRUE;
						currArg++;
					}
					else
						goto error;
					break;
#endif

				case 't':
					if (strcmp(argument + 1, "tcl") == 0)
					{
#if TCL
						meta_pdus = parse_type_list(argv[++currArg]);
						genTclCode = TRUE;
						if (!genMetaCode)
							genMetaCode = META_backend_names;
						genCxxCode = TRUE;
#else
						goto error;
#endif
					}
					else
						genTypeCode = TRUE;
					currArg++;
					break;
				case 'e':
					genEncodeCode = TRUE;
					currArg++;
					break;
				case 'd':
					genDecodeCode = TRUE;
					currArg++;
					break;
				case 'j':
					genJSONEncDec = TRUE; // Generate JSON Encoders / decoders
					currArg++;
					break;
				case 'p':
					genPrintCode = TRUE;
					currArg++;
					break;
				case 'x':
					genPrintCodeXML = TRUE;
					currArg++;
					break;
				case 'f':
					if (strcmp(argument + 1, "filter") == 0)
					{
						gFilterASN1Files = TRUE;
						currArg++;
					}
					else
					{
						genFreeCode = TRUE;
						currArg++;
					}
					break;
				case 'C': /* produce C++ code */
					if (strcmp(argument + 1, "Ch") == 0)
					{
						currArg++;
						szCppHeaderIncludePath = argv[currArg];
						if (szCppHeaderIncludePath == 0 || strlen(szCppHeaderIncludePath) == 0)
							goto error;
					}
					else if (strcmp(argument + 1, "Ce") == 0)
						genCxxCode_EnumClasses = TRUE;
					else if (strcmp(argument + 1, "C") == 0)
						genCxxCode = TRUE;
					else
						goto error;
					currArg++;
					break;
				case 'S': /* produce Swift code */
					genSwiftCode = TRUE;
					currArg++;
					break;
				case 'J': /* produce Javascript Objects */
					if (strcmp(argument + 1, "JD") == 0)
						genJsonDocCode = TRUE;
					else if (strcmp(argument + 1, "JT") == 0)
						genTSCommonJSCode = TRUE;
					else if (strcmp(argument + 1, "JTE") == 0)
						genTSESMCode = TRUE;
					else if (strcmp(argument + 1, "JO") == 0)
						genOpenApiCode = TRUE;
					else
						genJSCode = TRUE;
					currArg++;
					break;
				case 'D': /* produce Delphi Objects */
					genDelphiCode = TRUE;
					currArg++;
					break;
				case 'b': /* produce C++ code */
					genPERCode = TRUE;
					currArg++;
					break;
				case 'n':
					if (strcmp(argument + 1, "nons") == 0)
					{
						currArg++;
						gNO_NAMESPACE = 1;
					}
					else if (strcmp(argument + 1, "ns") == 0)
					{
						gAlternateNamespaceString = &argument[4];
						currArg += 2;
					}
					else if (strcmp(argument + 1, "noprivate") == 0)
					{
						gPrivateSymbols = 0;
						currArg++;
					}
					else if (strncmp(argument + 1, "nodeprecated", 12) == 0)
					{
						// Shortest time would be -nodeprecated:1.1.2000 = + 9 charachters
						size_t len = strlen(argument + 1) - 12;
						if (len > 0)
						{
							if (len < 9)
							{
								// The string but not long enough....
								Usage(argv[0], stderr);
								return 1;
							}
							const char* szFollowing = argument + 14;
							time_t i64Result = ConvertDateToUnixTime(szFollowing);
							if (i64Result < 0)
							{
								// Invalid time, could not parse the time
								Usage(argv[0], stderr);
								return 1;
							}
							gi64NoDeprecatedSymbols = i64Result;
						}
						else
						{
							gi64NoDeprecatedSymbols = 1;
						}
						currArg++;
					}
					else if (strncmp(argument + 1, "node", 4) == 0)
					{
						gNodeVersion = atoi(argument + 6);
						if (gNodeVersion == 0)
							goto error;
						currArg++;
					}
					else if (strcmp(argument + 1, "novolat") == 0)
					{
						novolatilefuncs = TRUE;
						currArg++;
					}
					else
						goto error;
					break;
				case 'c':
					if (strcmp(argument + 1, "comments") == 0)
						giWriteComments = TRUE;
					else
						genCCode = TRUE;
					currArg++;
					break;
				case 'l':
					if (argument[2] != '\0') /* no space after -l */
					{
						longJmpVal = atoi(&argument[2]);
						currArg++;
					}
					else
					{
						longJmpVal = atoi(argv[currArg + 1]);
						currArg += 2;
					}
					break;
				case 'T':
				case 'O':
					genTypeTbls = argument[1] == 'T' ? 2 : 1;
					if (argument[2] != '\0') /* no space after -T */
					{
						tblFileName = &argument[2];
						currArg++;
					}
					else
					{
						tblFileName = argv[currArg + 1];
						currArg += 2;
					}
					break;
				case 'o':
					if (argv[currArg + 1] != NULL)
					{
						strcpy_s(gszOutputPath, _MAX_PATH - 1, argv[currArg + 1]);
						getDirectoryWithDelimiterFromPath(gszOutputPath, _MAX_PATH - 1);
						if (!createDirectories(gszOutputPath))
							snacc_exit("Failed to create directory %s", gszOutputPath);
						currArg++;
					}
					currArg++;
					break;
				case 'E':
					if (currArg + 1 == argc)
					{
						fprintf(errFileG, "%s: ERROR---encoding rule missing after -E\n", argv[0]);
						Usage(argv[0], stdout);
						return 1;
					}
					/* Select encoding rules */
					if (strcmp(argv[currArg + 1], "BER") == 0)
					{
						AddEncRules(BER);
						encRulesSet = TRUE;
						currArg += 2;
					}
					else if (strcmp(argv[currArg + 1], "DER") == 0)
					{
						AddEncRules(DER);
						encRulesSet = TRUE;
						currArg += 2;
					}
					else
					{
						fprintf(errFileG, "%s: ERROR---no such encoding rule \"%s\". Try BER or DER\n", argv[0], argv[currArg + 1]);
						Usage(argv[0], stdout);
						return 1;
					}
					break;
				case 'V':
					if (!strncmp(argument + 1, "VDAexport", strlen("VDAexport")))
					{
						if (strlen(argument + 1) > strlen("VDAexport"))
							bVDAGlobalDLLExport = _strdup(argument + 1 + strlen("VDAexport") + 1); // TRUE
						else																	   // Default a definition for SFL.
							bVDAGlobalDLLExport = "VDASNACCDLL_API";
						currArg++;
						break;
					}
					else if (strcmp(argument + 1, "ValidationLevel") == 0)
					{
						currArg++;
						giValidationLevel = atoi(argv[currArg]);
						currArg++;
						break;
					}
					else
						currArg++; // IGNORE the "-V" option.
					break;
				case 'L':
					if (errFileG != NULL)
					{
						fprintf(stderr, "ERROR---Multiple occurrences of error log file option -L");
						Usage(argv[0], stderr);
						return 1;
					}
					if (argument[2] != '\0')
						errFileName = &argument[2];
					else
						errFileName = argv[++currArg];
					/* Open the error log file */
					if (fopen_s(&errFileG, errFileName, "wt") != 0 || errFileG == NULL)
					{
						fprintf(stderr, "ERROR---Unable to open error log file: \'%s\'\n", errFileName);
						return 1;
					}
					currArg++;
					break;
				case 'y':
					/*RWC;yydebug = 1;*/
					yydebug = 1;
					currArg++;
					break;
				case 'm':
					if (argument[2] == 'f')
					{
						if (argument[3] != '\0') /* no space after -mf */
						{
							maxFileNameLenG = atoi(&argument[3]);
							currArg++;
						}
						else
						{
							maxFileNameLenG = atoi(argv[currArg + 1]);
							currArg += 2;
						}
						break;
					}
#if META
					else if (strcmp(argument + 1, "meta") == 0)
					{
						meta_pdus = parse_type_list(argv[++currArg]);
						if (!genMetaCode)
							genMetaCode = META_backend_names;
						genCxxCode = TRUE;
						currArg++;
						break;
					}
					else if (strcmp(argument + 1, "mA") == 0)
					{
						genMetaCode = META_asn1_names;
						genCxxCode = TRUE;
						currArg++;
						break;
					}
					else if (strcmp(argument + 1, "mC") == 0)
					{
						genMetaCode = META_backend_names;
						genCxxCode = TRUE;
						currArg++;
						break;
					}
#endif
				error:
				default:
					fprintf(stderr, "%s: ERROR---unknown cmd line option `%s'\n\n", argv[0], argument);
					Usage(argv[0], stderr);
					return 1;
			}
		}
		else /* asn1srcFileName */
		{
			// no -Argument on the command line, this is a file or a wildcard to a file...
			const char* szFileName = argument;
			numSrcFiles += findFiles(szFileName, false);
			currArg++;
		}
	} /* end of for loop */

	if (numSrcFiles == 0)
	{
		fprintf(stderr, "%s: ERROR---no ASN.1 source files were specified\n", argv[0]);
		Usage(argv[0], stderr);
		return 1;
	}

	/*
	 * set default options
	 */
	if (!(genTypeCode || genValueCode || genEncodeCode || genDecodeCode || genFreeCode || genPrintCode || genPrintCodeXML))
	{
		genTypeCode = TRUE;
		genValueCode = TRUE;
		genEncodeCode = TRUE;
		genDecodeCode = TRUE;
		genFreeCode = TRUE;
		genPrintCode = TRUE;
		genPrintCodeXML = FALSE;
	}
	else if (genCCode + genCxxCode + genTypeTbls + genIDLCode + genJAVACode + genCSCode + genJSCode + genSwiftCode + genTSCommonJSCode + genTSESMCode + genDelphiCode + genOpenApiCode + gFilterASN1Files > 1 + genJsonDocCode)
	{
		fprintf(stderr, "%s: ERROR---Choose only one of the -c -C or -D or -T or -RCS or -RJ or -OA or -J or -JD or -filter options\n", argv[0]);
		Usage(argv[0], stderr);
		return 1;
	}

	if (!genCCode && !genCxxCode && !genJAVACode && !genCSCode && !genTypeTbls && !genIDLCode && !genSwiftCode && !genJSCode && !genDelphiCode && !genTSCommonJSCode && !genTSESMCode && !genJsonDocCode && !genOpenApiCode && !gFilterASN1Files)
		genCCode = TRUE; /* default to C if neither specified */

	/* Set the encoding rules to BER if not set */
	if (!encRulesSet)
		AddEncRules(BER);

	/* Set the error log file to stderr if not specified */
	if (errFileG == NULL)
		errFileG = stderr;

	/*
	 * STEP 1---parse each ASN.1 src file
	 */
	allMods = (ModuleList*)AsnListNew(sizeof(void*));

	bool bReadVersionFile = true;

	SASN1File file;
	while (getNextFile(&file, 0))
	{
		if (bReadVersionFile)
		{
			bReadVersionFile = false;
			char* szDirectory = getFilePath(file.filePath);
			if (szDirectory)
			{
				char szPath[_MAX_PATH] = {0};
				strcpy_s(szPath, _MAX_PATH - 1, szDirectory);
				strcat_s(szPath, _MAX_PATH - 1, "interfaceversion.txt");
				FILE* pFile = NULL;
				if (fopen_s(&pFile, szPath, "r") == 0 && pFile)
				{
					while (true)
					{
						char szLine[128] = {0};
						if (fgets(szLine, sizeof(szLine), pFile))
						{
							// Trim leading whitespaces or empty lines (just \r \n)
							int start = 0;
							while (isspace((unsigned char)szLine[start]))
								start++;

							if (strlen(&szLine[start]) && szLine[start] != '#' && szLine[start] != '/')
							{
								gMajorInterfaceVersion = atoi(&szLine[start]);
								break;
							}
						}
						else
							break;
					}
					fclose(pFile);
				}
				if (gFilterASN1Files)
				{
					// We have the folder path. We need to copy the esnacc_whitelist.txt nd the interfaceversion.txt
					{
						char szTarget[_MAX_PATH] = {0};
						strcat_s(szTarget, _MAX_PATH - 1, gszOutputPath);
						strcat_s(szTarget, _MAX_PATH - 1, "interfaceversion.txt");
						copy_file(szPath, szTarget);
					}
					{
						char szTarget[_MAX_PATH] = {0};
						strcat_s(szTarget, _MAX_PATH - 1, gszOutputPath);
						strcat_s(szTarget, _MAX_PATH - 1, "esnacc_whitelist.txt");
						strcpy_s(szPath, _MAX_PATH - 1, szDirectory);
						strcat_s(szPath, _MAX_PATH - 1, "esnacc_whitelist.txt");
						copy_file(szPath, szTarget);
					}
				}
			}
		}

		currMod = ParseAsn1File(file.filePath, file.bIsImportedFile, 1);

		if (currMod == NULL)
			return 1;

		/*
		 * insert this module at the head of the list
		 * of already parsed (if any) modules
		 */
		tmpModHndl = (Module**)AsnListAppend(allMods);
		*tmpModHndl = currMod;
	}

	// If we are filtering we now just need to write the contents of the file parser
	if (gFilterASN1Files)
	{
		FilterFiles();
		free(allMods);
		printf("Now exiting...\n");
		return 0;
	}

	/*
	 * Check that the module names/oids are unique
	 */
	if (!ModNamesUnique(allMods))
	{
		fprintf(errFileG, "\nConflicting module names, cannot proceed.\n");
		return 1;
	}

	/*
	 * STEP 2
	 * Now that all files have been parsed,
	 * link local and locatable import type refs
	 */
	if (LinkTypeRefs(allMods) < 0)
	{
		fprintf(errFileG, "\nType linking errors---cannot proceed\n");
		return 2;
	}

	/*
	 * STEP 3
	 * Parse constructed values now that types are all parsed
	 * and have been linked.  Need type info to be able to
	 * parse values easily (elimitate ambiguity).
	 */
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (ParseValues(allMods, currMod) != 0)
			fprintf(errFileG, "WARNING: Value parsing error (s), attempting to continue\n");
	}

	/*
	 * STEP 4
	 * Value parsing may have defined some new values
	 * so can link local and locatable import value refs now.
	 */
	if (LinkValueRefs(allMods) < 0)
	{
		fprintf(errFileG, "\nValue linking errors---cannot proceed\n");
		return 4;
	}

	/*
	 * STEP 5
	 * process macros
	 *   - adding type/value defs as nec
	 *   - mark type defs with ANY DEFINED BY id if nec
	 *     so they are put in the id to ANY type hash tbl.
	 */
	semErr = 0;
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{ // For Macors, New TypeDefs are added here, if required
		ProcessMacros(currMod);
		if (currMod->status == MOD_ERROR)
			semErr = 1;
	}
	if (semErr)
		return 5;

	/*
	 * STEP 6
	 * convert silly type constructs into
	 * a normal format, leaving behind pure type/value info
	 * eg: expand COMPONENTS OF refs, SELECTION types.
	 * boil down values into simplest rep. (eg OID -> ENC_OID)
	 */
	semErr = 0;
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{ // New TypeDefs are added here, if required
		NormalizeModule(currMod);
		if (currMod->status == MOD_ERROR)
			semErr = 1;
	}
	if (semErr)
		return 6;

	/*
	 * STEP 7
	 * Mark recusive types.  Currently the recursive information is
	 * not used elsewhere.
	 */
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		MarkRecursiveTypes(currMod);
	}

	/*
	 * STEP 8
	 * Check for errors in the ASN.1 modules.
	 * Check all modules and exit if errors were found
	 */
	semErr = 0;
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		ErrChkModule(currMod);
		if (currMod->status == MOD_ERROR)
			semErr = 1;
	}
	if (semErr)
		return 8;

	/*
	 * exit if any sundry errors occurred at any point.
	 * smallErrG is set upon finding small errors that prevent code
	 * production but should not affect the other processing/error
	 * checking steps.  This allows full display of errors.
	 */
	if (smallErrG)
	{
		/*
		 * for debugging show "parsed" version of ASN.1 module if
		 * the print flag is set.
		 * Dumps each module to stdout. Printed from Module data struct
		 * print here before exiting otherwise print after sorting
		 */
		if (printModuleFlag)
		{
			FOR_EACH_LIST_ELMT(currMod, allMods)
			{
				printf("\n\n");
				PrintModule(stdout, currMod);
			}
		}

		return 8;
	}

	/*
	 * STEP 9
	 * Make C/C++ typenames/routine names for enc/decode.
	 * Type/Value renaming will occur if name conflicts
	 * arise between modules.
	 *
	 * NOTE: this is done before sorting the types because
	 *       the type sorting routine may use the 'isPtr'
	 *       information to help order knots of recursive types.
	 */
	if (genCCode)
		FillCTypeInfo(&cRulesG, allMods);

	else if (genCxxCode || genJAVACode || genCSCode || genSwiftCode || genJSCode || genDelphiCode || genTSCommonJSCode || genTSESMCode || genJsonDocCode || genOpenApiCode)
		FillCxxTypeInfo(&cxxRulesG, allMods);

#if IDL
	else if (genIDLCode)
		FillIDLTypeInfo(&idlRulesG, allMods);
#endif

	/*
	 * STEP 10
	 * Sort each typedef list such that independent types are
	 * before the types that depend on them
	 *
	 *  modules remain in same order as given on command line
	 *  (cmd line file order should be
	 *      least dependent module-> most dependent module
	 *      so that include file order in generated src is correct)
	 */
	SortAllDependencies(allMods);

	/*
	 * Creates the different names we need to write the output files
	 */
	CreateNames(allMods);

	/*
	 * STEP 11
	 * Validates that the structures written are okay
	 * Checks for certain things we do not want to see :)
	 */
	ValidateASN1Data(allMods);

	/*
	 * STEP 12
	 * for debugging show "parsed" version of ASN.1 module.
	 * dumps each module to stdout. Printed from Module data struct
	 * Shows the results of normalization and sorting.
	 */
	if (printModuleFlag)
	{
		FOR_EACH_LIST_ELMT(currMod, allMods)
		{
			printf("\n\n");
			PrintModule(stdout, currMod);
		}
	}

	/*
	 * Step 13
	 * Final Step: Code/Type Table generation
	 */
	if (genCCode)
		GenCCode(allMods, longJmpVal, genTypeCode, genValueCode, genEncodeCode, genDecodeCode, genPrintCode, genFreeCode);

	if (genCxxCode)
		GenCxxCode(allMods, longJmpVal, genTypeCode, genValueCode, genEncodeCode, genDecodeCode, genJSONEncDec, genPrintCode, genPrintCodeXML, genFreeCode, szCppHeaderIncludePath, if_META(genMetaCode COMMA meta_pdus COMMA) if_TCL(genTclCode COMMA) novolatilefuncs, genROSEDecoders, genCxxCode_EnumClasses);

	if (genSwiftCode)
		GenSwiftCode(allMods, longJmpVal, genTypeCode, genValueCode, genEncodeCode, genDecodeCode, genJSONEncDec, genPrintCode, genPrintCodeXML, genFreeCode, if_META(genMetaCode COMMA meta_pdus COMMA) if_TCL(genTclCode COMMA) novolatilefuncs, genROSEDecoders);

	if (genJSCode)
		GenJSCode(allMods, longJmpVal, genTypeCode, genValueCode, genEncodeCode, genDecodeCode, genJSONEncDec, genPrintCode, genPrintCodeXML, genFreeCode, if_META(genMetaCode COMMA meta_pdus COMMA) if_TCL(genTclCode COMMA) novolatilefuncs, genROSEDecoders);

	if (genOpenApiCode)
		GenOpenApiCode(allMods, longJmpVal, genTypeCode, genValueCode, genEncodeCode, genDecodeCode, genJSONEncDec, genPrintCode, genPrintCodeXML, genFreeCode, if_META(genMetaCode COMMA meta_pdus COMMA) if_TCL(genTclCode COMMA) novolatilefuncs, genROSEDecoders);

	if (genTSCommonJSCode || genTSESMCode)
		GenTSCode(allMods, longJmpVal, genTypeCode, genValueCode, genJSONEncDec, genTSRoseStubs, genPrintCode, genPrintCodeXML, genFreeCode, if_META(genMetaCode COMMA meta_pdus COMMA) if_TCL(genTclCode COMMA) novolatilefuncs, genROSEDecoders);

	if (genJsonDocCode)
		GenJsonDocCode(allMods);

	if (genCSCode)
		GenCSCode(allMods, genROSEDecoders);

	if (genJAVACode)
		GenJAVACode(allMods, genROSEDecoders);

	if (genDelphiCode)
		GenDelphiCode(allMods, longJmpVal, genTypeCode, genValueCode, genEncodeCode, genDecodeCode, genJSONEncDec, genPrintCode, genPrintCodeXML, genFreeCode, if_META(genMetaCode COMMA meta_pdus COMMA) if_TCL(genTclCode COMMA) novolatilefuncs, genROSEDecoders);

	if (genTypeTbls)
		GenTypeTbls(allMods, tblFileName, genTypeTbls);

#if IDL
	if (genIDLCode)
		GenIDLCode(allMods, longJmpVal, genTypeCode, genValueCode, genPrintCode, genFreeCode);
#endif

	free(allMods);

	printf("Now exiting...\n");

	// getchar();
	return 0;
} /* end main */

#if META
MetaPDU* parse_type_list PARAMS((arg), char* arg)
{
	MetaPDU* meta_pdus = NULL;
	char* module;
	for (module = strtok(arg, ".:"); module; module = strtok(NULL, ".:"))
	{
		MetaPDU* pdu = MT(MetaPDU);
		char* type = strtok(NULL, " /,;");
		if (!type)
		{
			fprintf(errFileG, "usage: {-meta|-tcl} module.type[,module.type[...]]\n");
			return (1);
		}
		pdu->module = module;
		pdu->type = type;
		pdu->used = FALSE;

		pdu->next = meta_pdus;
		meta_pdus = pdu;
	}
	return meta_pdus;
}
#endif /* META */

/*
 * Calls the yacc/lex parser given a the ASN.1 src file's filename.
 * Returns a Module *for the given ASN.1 module. If the filename is
 * "-" stdin is used.
 */
Module* ParseAsn1File(const char* fileName, short ImportFlag, int parseComments)
{
	FILE* fPtr;
	Module* retVal;
	int parseResult;

	enum EFILETYPE fileType = ASCII;

	/*
	 *  Open input file for lexical analyzer/parser
	 *  Use stdin if the filename is "-"
	 */
	if (strcmp(fileName, "-") == 0)
	{
		fPtr = stdin;
	}
	else
	{
		if (fopen_s(&fPtr, fileName, "r") != 0 || fPtr == NULL)
		{
			perror("fopen: ");
			fprintf(errFileG, "ERROR---asn1 src file `%s' cannot be opened for reading\n", fileName);
			return NULL;
		}
		unsigned char szFileType[3] = {0};
		size_t readBytes = 0;
#ifdef _WIN32
#ifdef _MSC_VER
		readBytes = fread_s(szFileType, 3, sizeof(char), 3, fPtr);
#else
		// MingW, Clang, GCC
		readBytes = fread(szFileType, sizeof(char), 3, fPtr);
#endif
#else
		readBytes = fread(szFileType, sizeof(char), 3, fPtr);
#endif
		if (readBytes == 3 && szFileType[0] == 0xef && szFileType[1] == 0xbb && szFileType[2] == 0xbf) // UTF8
			fileType = UTF8WITHBOM;
		else
		{
			fseek(fPtr, 0, SEEK_SET);
			char szLine[5000] = {0};
			while (fileType == ASCII && fgets(szLine, 5000, fPtr))
			{
				char* szPos = szLine;
				while (*szPos != 0)
				{
					if ((unsigned char)*szPos > 127)
					{
						fileType = UTF8;
						break;
					}
					szPos++;
				}
			}
			fseek(fPtr, 0, SEEK_SET);
		}
	}

	retVal = (Module*)Malloc(sizeof(Module));

	/*
	 * Init Parser by giving it a ptr to the Module data struct
	 * to initialize/use, and the file name associtated with
	 * the given FILE *, fPtr (for error reporting).
	 * fPtr should be an opened FILE *to an ASN.1 source FILE
	 */
	InitAsn1Parser(retVal, fileName, fPtr);

	/*
	 * parse the current asn1 src file into the
	 * Module data struct
	 */
	parseResult = yyparse();

	if (parseResult != 0 || retVal->status == MOD_ERROR)
	{
		/* parser will print exact err msg */
		fprintf(errFileG, "\nParsing errors---cannot proceed\n");
		return NULL;
	}

	if (parseComments)
	{
		// Parse the comments in this file...
		char* szModuleName = MakeModuleName(fileName);
		parseResult = ParseFileForComments(fPtr, szModuleName, fileType);
		free(szModuleName);
		if (parseResult != 0)
		{
			/* parser will print exact err msg */
			fprintf(errFileG, "\nParsing errors---cannot proceed\n");
			return NULL;
		}
	}

	if (fPtr != stdin)
		fclose(fPtr);

	retVal->ImportedFlag = (unsigned char)ImportFlag;

	return retVal;

} /* ParseAsn1File */

/*
 * Given the list of parsed, linked, normalized, error-checked and sorted
 * modules, and some code generation flags, generates C code and
 * writes it to files derived from each modules name.  Each module
 * gets 2 source files, one .h for data struct and prototypes, the other .c
 * for the enc/dec/print/free routine code.
 */
int GenCCode PARAMS((allMods, longJmpVal, genTypes, genValues, genEncoders, genDecoders, genPrinters, genFree), ModuleList* allMods _AND_ long longJmpVal _AND_ int genTypes _AND_ int genValues _AND_ int genEncoders _AND_ int genDecoders _AND_ int genPrinters _AND_ int genFree)
{
	Module* currMod;
	FILE* cHdrFilePtr = NULL;
	FILE* cSrcFilePtr = NULL;
	DefinedObj* fNames;
	int fNameConflict = FALSE;

	/*
	 * Make names for each module's encoder/decoder src and hdr files
	 * so import references can be made via include files.
	 * If file names conflict, print error msg & exit.
	 */
	fNames = NewObjList();
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (ObjIsDefined(fNames, currMod->cHdrFileName, StrObjCmp) || ObjIsDefined(fNames, currMod->cSrcFileName, StrObjCmp))
		{
			fprintf(errFileG, "Ack! ERROR---file name conflict for generated source files with names `%s' and `%s'.\n\n", currMod->cHdrFileName, currMod->cSrcFileName);
			fprintf(errFileG, "This usually means the max file name length is truncating the file names.\n");
			fprintf(errFileG, "Try re-naming the modules with shorter names or increasing the argument to -mf option (if you are using it).\n");
			fprintf(errFileG, "This error can also be caused by 2 modules having the same name but different OBJECT IDENTIFIERs.");
			fprintf(errFileG, "  Try renaming the modules to correct this.\n");
			fNameConflict = TRUE;
		}
		else
		{
			DefineObj(&fNames, currMod->cHdrFileName);
			DefineObj(&fNames, currMod->cSrcFileName);
		}
	}
	if (fNameConflict)
		return (1);

	FreeDefinedObjs(&fNames);
	/*
	 * make c files
	 */
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			fopen_s(&cHdrFilePtr, currMod->cHdrFileName, "w");
			fopen_s(&cSrcFilePtr, currMod->cSrcFileName, "w");
		}
		if ((currMod->ImportedFlag == FALSE) && ((cSrcFilePtr == NULL) || (cHdrFilePtr == NULL)))
			perror("fopen");

		else if (currMod->ImportedFlag == FALSE)
		{
			PrintCCode(cSrcFilePtr, cHdrFilePtr, allMods, currMod, &cRulesG, longJmpVal, genTypes, genValues, genEncoders, genDecoders, genPrinters, genFree);
			fclose(cHdrFilePtr);
			fclose(cSrcFilePtr);
		}
	}
	return 0;
} /* GenCCode */

/*
 * Given the list of parsed, linked, normalized, error-checked and sorted
 * modules, and some code generation flags, generates C++ code and
 * writes it to files derived from each modules name.  Each module
 * gets 2 source files, one .h for data struct and prototypes, the other .C
 * for the enc/dec/print/free routine code.
 */
void GenJAVACode(ModuleList* allMods, int genROSEJAVADecoders)
{
	if (genROSEJAVADecoders)
		PrintJAVACode(allMods);
} /* GenJAVACode */

/*
 * Given the list of parsed, linked, normalized, error-checked and sorted
 * modules, and some code generation flags, generates C++ code and
 * writes it to files derived from each modules name.  Each module
 * gets 2 source files, one .h for data struct and prototypes, the other .C
 * for the enc/dec/print/free routine code.
 */
void GenCSCode(ModuleList* allMods, int genROSECSDecoders)
{
	Module* currMod;
	AsnListNode* saveMods;
	FILE* srcFilePtr;
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
		if (ObjIsDefined(fNames, currMod->ROSESrcCSFileName, StrObjCmp))
		{
			fprintf(errFileG, "Ack! ERROR---file name conflict for generated source files with names `%s'.\n\n", currMod->ROSESrcCSFileName);
			fprintf(errFileG, "This usually means the max file name length is truncating the file names.\n");
			fprintf(errFileG, "Try re-naming the modules with shorter names or increasing the argument to -mf option (if you are using it).\n");
			fprintf(errFileG, "This error can also be caused by 2 modules having the same name but different OBJECT IDENTIFIERs.");
			fprintf(errFileG, "  Try renaming the modules to correct this.\n");
			fNameConflict = TRUE;
		}
		else
		{
			DefineObj(&fNames, currMod->ROSESrcCSFileName);
		}

		if (fNameConflict)
			return;

		FreeDefinedObjs(&fNames);
	}
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			if (genROSECSDecoders) // CS
			{
				if (fopen_s(&srcFilePtr, currMod->ROSESrcCSFileName, "wt") != 0 || srcFilePtr == NULL)
				{
					perror("fopen ROSE");
				}
				else
				{
					saveMods = allMods->curr;

					PrintROSECSCode(srcFilePtr, allMods, currMod);
					allMods->curr = saveMods;
					fclose(srcFilePtr);
				}
			}
		}
	}
} /* GenCSCode */

/*
 * Given the list of parsed, linked, normalized, error-checked and sorted
 * modules, and some code generation flags, generates C++ code and
 * writes it to files derived from each modules name.  Each module
 * gets 2 source files, one .h for data struct and prototypes, the other .C
 * for the enc/dec/print/free routine code.
 */
void GenCxxCode(ModuleList* allMods, long longJmpVal, int genTypes, int genValues, int genEncoders, int genDecoders, int genJSONEncDec, int genPrinters, int genPrintersXML, int genFree, const char* szCppHeaderIncludePath, if_META(MetaNameStyle genMeta _AND_) if_META(MetaPDU* meta_pdus _AND_) if_TCL(int genTcl _AND_) int novolatilefuncs, int genROSEDecoders, const int genCxxCode_EnumClasses)
{
	Module* currMod;
	AsnListNode* saveMods;
	FILE* hdrFilePtr;
	FILE* srcFilePtr;
	FILE* hdrInterfaceFilePtr;
	FILE* hdrForwardDecl;
	DefinedObj* fNames;
	int fNameConflict = FALSE;

#if META
	static const char metabasefn[] = "modules";
	Meta meta;
#if TCL
	const MetaPDU* pdu;
#endif
#endif

	/*
	 * Make names for each module's encoder/decoder src and hdr files
	 * so import references can be made via include files
	 * check for truncation --> name conflicts & exit if nec
	 */
	fNames = NewObjList();
#if META
	if (genMeta)
		DefineObj(&fNames, meta.srcfn = MakeCxxSrcFileName(metabasefn));
#endif
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
#if META
		{
			char *in, *out;

			out = currMod->cxxname = (char*)malloc(strlen(in = currMod->modId->name) + 1);
			do
				*out++ = *in == '-' ? '_' : *in;
			while (*in++);
		}
#endif

		if (ObjIsDefined(fNames, currMod->cxxHdrFileName, StrObjCmp) || ObjIsDefined(fNames, currMod->cxxSrcFileName, StrObjCmp))
		{
			fprintf(errFileG, "Ack! ERROR---file name conflict for generated source files with names `%s' and `%s'.\n\n", currMod->cxxHdrFileName, currMod->cxxSrcFileName);
			fprintf(errFileG, "This usually means the max file name length is truncating the file names.\n");
			fprintf(errFileG, "Try re-naming the modules with shorter names or increasing the argument to -mf option (if you are using it).\n");
			fprintf(errFileG, "This error can also be caused by 2 modules having the same name but different OBJECT IDENTIFIERs.");
			fprintf(errFileG, "  Try renaming the modules to correct this.\n");
			fNameConflict = TRUE;
		}
		else
		{
			DefineObj(&fNames, currMod->cxxHdrFileName);
			DefineObj(&fNames, currMod->cxxSrcFileName);
		}

		if (fNameConflict)
			return;

		FreeDefinedObjs(&fNames);

		/*
		 * make C++ files
		 */
#if META
		if (genMeta)
		{
			time_t now = time(NULL);

			if (!(meta.srcfp = fopen(meta.srcfn, "w")))
			{
				perror("fopen");
				exit(1);
			}
			fprintf(meta.srcfp, "// modules.C - reference to all modules and their types\n");
			fprintf(meta.srcfp, "//\n");
			write_snacc_header(meta.srcfp, "// ");
			fprintf(meta.srcfp, "//\n");
		}
#endif
	}

	if (gMajorInterfaceVersion >= 0 && genVersionFile)
	{
		FILE* versionFile = NULL;
		char* szVersionFile = MakeFileName("Asn1InterfaceVersion.h", "");
		if (fopen_s(&versionFile, szVersionFile, "wt") != 0 || versionFile == NULL)
			perror("fopen");
		else
		{
			long long lMaxMinorVersion = GetMaxModuleMinorVersion();
			write_snacc_header(versionFile, "// ");
			fprintf(versionFile, "\n");
			fprintf(versionFile, "// clang-format off\n\n");
			fprintf(versionFile, "#ifndef ASN1_INTERFACE_VERSION_H\n");
			fprintf(versionFile, "#define ASN1_INTERFACE_VERSION_H\n\n");
			fprintf(versionFile, "#ifndef NO_NAMESPACE\n");
			fprintf(versionFile, "namespace SNACC\n");
			fprintf(versionFile, "{\n");
			fprintf(versionFile, "#endif\n\n");

			fprintf(versionFile, "struct Asn1InterfaceVersion\n");
			fprintf(versionFile, "{\n");
			fprintf(versionFile, "\tstatic constexpr const char* lastChange = \"%s\";\n", ConvertUnixTimeToISO(lMaxMinorVersion));
			fprintf(versionFile, "\tstatic constexpr int majorVersion = %i;\n", gMajorInterfaceVersion);
			fprintf(versionFile, "\tstatic constexpr long long minorVersion = %lld;\n", lMaxMinorVersion);
			fprintf(versionFile, "\tstatic constexpr const char* version = \"%i.%lld.0\";\n", gMajorInterfaceVersion, lMaxMinorVersion);
			fprintf(versionFile, "};\n\n");

			fprintf(versionFile, "#ifndef NO_NAMESPACE\n");
			fprintf(versionFile, "}\n");
			fprintf(versionFile, "#endif\n\n");

			fprintf(versionFile, "#endif");
			fclose(versionFile);
		}
		free(szVersionFile);
	}

	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			/*
			 * create and fill .h file for module's data structs
			 */
			fopen_s(&hdrFilePtr, currMod->cxxHdrFileName, "wt");
			fopen_s(&srcFilePtr, currMod->cxxSrcFileName, "wt");

			if ((hdrFilePtr == NULL) || (srcFilePtr == NULL))
			{
				perror("fopen");
			}
			else
			{
				saveMods = allMods->curr;
				PrintCxxCode(srcFilePtr, hdrFilePtr, if_META(genMeta COMMA & meta COMMA meta_pdus COMMA) allMods, currMod, &cxxRulesG, longJmpVal, genTypes, genValues, genEncoders, genDecoders, genJSONEncDec, genPrinters, genPrintersXML, genFree, if_TCL(genTcl COMMA) novolatilefuncs, szCppHeaderIncludePath, genCxxCode_EnumClasses);
				allMods->curr = saveMods;
				fclose(hdrFilePtr);
				fclose(srcFilePtr);
			}

			if (genROSEDecoders)
			{
				// Print Forward Declarations

				if (fopen_s(&hdrForwardDecl, currMod->ROSEHdrForwardDeclFileName, "wt") != 0 || hdrForwardDecl == NULL)
				{
					perror("fopen ROSE");
				}
				else
				{
					saveMods = allMods->curr;

					PrintForwardDeclarationsCode(hdrForwardDecl, allMods, currMod);
					allMods->curr = saveMods;

					fclose(hdrForwardDecl);
				}

				if (HasROSEOperations(currMod))
				{
					fopen_s(&hdrFilePtr, currMod->ROSEHdrFileName, "wt");
					fopen_s(&srcFilePtr, currMod->ROSESrcFileName, "wt");
					fopen_s(&hdrInterfaceFilePtr, currMod->ROSEHdrInterfaceFileName, "wt");

					if ((hdrFilePtr == NULL) || (srcFilePtr == NULL) || (hdrInterfaceFilePtr == NULL))
					{
						perror("fopen ROSE");
					}
					else
					{
						saveMods = allMods->curr;

						PrintROSECode(srcFilePtr, hdrFilePtr, hdrInterfaceFilePtr, allMods, currMod, &cxxRulesG, szCppHeaderIncludePath);
						allMods->curr = saveMods;

						fclose(hdrFilePtr);
						fclose(hdrInterfaceFilePtr);
						fclose(srcFilePtr);
					}
				}
			}

#if META
			if (genMeta)
			{
				fprintf(meta.srcfp, "\n");
				fprintf(meta.srcfp, "#ifndef META\n");
				fprintf(meta.srcfp, "#define META	1\n");
				fprintf(meta.srcfp, "#endif\n");
				if (meta_pdus)
				{
					for (pdu = meta_pdus; pdu; pdu = pdu->next)
						if (!pdu->used)
							fprintf(errFileG, "warning: PDU %s.%s couldn't be found\n", pdu->module, pdu->type);
				}
#if TCL
				fprintf(meta.srcfp, "#ifndef TCL\n");
				fprintf(meta.srcfp, "#define TCL	META\n");
				fprintf(meta.srcfp, "#endif\n");
#endif
			}

			fprintf(meta.srcfp, "\n");

			fprintf(meta.srcfp, "#include <cpp-lib\\include\\asn-incl.h>\n");
			FOR_EACH_LIST_ELMT(currMod, allMods)
			fprintf(meta.srcfp, "#include \"%s\"\n", currMod->cxxHdrFileName);
			fprintf(meta.srcfp, "\n");

			fprintf(meta.srcfp, "#if META\n\n");

			fprintf(meta.srcfp, "const AsnModuleDesc *asnModuleDescs[] =\n");
			fprintf(meta.srcfp, "{\n");
			FOR_EACH_LIST_ELMT(currMod, allMods)
			fprintf(meta.srcfp, "  &%sModuleDesc,\n", currMod->cxxname);
			fprintf(meta.srcfp, "  NULL\n");
			fprintf(meta.srcfp, "};\n\n");

			if (genTcl)
			{
				fprintf(meta.srcfp, "#if TCL\n\n");

				fprintf(meta.srcfp, "// hack to avoid the neccessity to list -ltk -ltcl both before and after -lasn1tcl:\n");
				fprintf(meta.srcfp, "static int (*dummy)(Tcl_Interp *) = Tcl_AppInit;\n\n");

				fprintf(meta.srcfp, "#endif // TCL\n\n");
			}

			fprintf(meta.srcfp, "#endif // META\n");

			fclose(meta.srcfp);
#endif
		}
	}
} /* GenCxxCode */

/*
 * Given the list of parsed, linked, normalized, error-checked and sorted
 * modules, and some code generation flags, generates Swift code and
 * writes it to files derived from each modules name.
 */
void GenSwiftCode(ModuleList* allMods, long longJmpVal, int genTypes, int genValues, int genEncoders, int genDecoders, int genJSONEncDec, int genPrinters, int genPrintersXML, int genFree, int novolatilefuncs, int genROSEDecoders)
{
	PrintSwiftCode(allMods, longJmpVal, genTypes, genValues, genEncoders, genDecoders, genJSONEncDec, genPrinters, genPrintersXML, genFree, novolatilefuncs, genROSEDecoders);
} /* GenSwiftCode */

/*
 * Given the list of parsed, linked, normalized, error-checked and sorted
 * modules, and some code generation flags, generates Swift code and
 * writes it to files derived from each modules name.
 */
void GenJSCode(ModuleList* allMods, long longJmpVal, int genTypes, int genValues, int genEncoders, int genDecoders, int genJSONEncDec, int genPrinters, int genPrintersXML, int genFree, int novolatilefuncs, int genROSEDecoders)
{
	Module* currMod;
	AsnListNode* saveMods;
	FILE* srcFilePtr;
	// FILE		*hdrInterfaceFilePtr;
	// FILE		*hdrForwardDecl;
	DefinedObj* fNames;
	int fNameConflict = FALSE;

	genROSEDecoders = 1;

	/*
	 * Make names for each module's encoder/decoder src and hdr files
	 * so import references can be made via include files
	 * check for truncation --> name conflicts & exit if nec
	 */
	fNames = NewObjList();
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (ObjIsDefined(fNames, currMod->jsFileName, StrObjCmp))
		{
			fprintf(errFileG, "Ack! ERROR---file name conflict for generated swift file with name `%s'.\n\n", currMod->jsFileName);
			fprintf(errFileG, "This usually means the max file name length is truncating the file names.\n");
			fprintf(errFileG, "Try re-naming the modules with shorter names or increasing the argument to -mf option (if you are using it).\n");
			fprintf(errFileG, "This error can also be caused by 2 modules have the same names but different OBJECT IDENTIFIERs.");
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
				PrintJSCode(srcFilePtr, allMods, currMod, longJmpVal, genTypes, genValues, genEncoders, genDecoders, genJSONEncDec, novolatilefuncs);
				allMods->curr = saveMods;
				fclose(srcFilePtr);
			}
		}
	}
} /* GenJSCode */

/*
 * Given the list of parsed, linked, normalized, error-checked and sorted
 * modules, and some code generation flags, generates OpenApi code and
 * writes it to files derived from each modules name.
 */
void GenOpenApiCode(ModuleList* allMods, long longJmpVal, int genTypes, int genValues, int genEncoders, int genDecoders, int genJSONEncDec, int genPrinters, int genPrintersXML, int genFree, int novolatilefuncs, int genROSEDecoders)
{
	Module* currMod;
	AsnListNode* saveMods;
	FILE* srcFilePtr;
	// FILE		*hdrInterfaceFilePtr;
	// FILE		*hdrForwardDecl;
	DefinedObj* fNames;
	int fNameConflict = FALSE;

	genROSEDecoders = 1;

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
				PrintOpenApiCode(srcFilePtr, allMods, currMod, longJmpVal, genTypes, genValues, genEncoders, genDecoders, genJSONEncDec, novolatilefuncs);
				allMods->curr = saveMods;
				fclose(srcFilePtr);
			}
		}
	}
} /* GenOpenAPICode */

/*
 * Given the list of parsed, linked, normalized, error-checked and sorted
 * modules, and some code generation flags, generates Swift code and
 * writes it to files derived from each modules name.
 */
#if defined(_MSC_VER)
#pragma warning(disable : 6262)
#endif
void GenTSCode(ModuleList* allMods, long longJmpVal, int genTypes, int genValues, int genJSONEncDec, int genTSROSEStubs, int genPrinters, int genPrintersXML, int genFree, int novolatilefuncs, int genROSEDecoders)
{
	PrintTSCode(allMods, longJmpVal, genTypes, genValues, genJSONEncDec, genTSROSEStubs, genPrinters, genPrintersXML, genFree, novolatilefuncs, genROSEDecoders);
} /* GenTSCode */
#if defined(_MSC_VER)
#pragma warning(default : 6262)
#endif

/*
 * Given the list of parsed, linked, normalized, error-checked and sorted
 * modules, and some code generation flags, generates Swift code and
 * writes it to files derived from each modules name.
 */
void GenJsonDocCode(ModuleList* allMods)
{
	PrintJsonDocCode(allMods);
} /* GenJsonDocCode */

/*
 * Given the list of parsed, linked, normalized, error-checked and sorted
 * modules, and some code generation flags, generates Delphi code and
 * writes it to files derived from each modules name.
 */
void GenDelphiCode(ModuleList* allMods, long longJmpVal, int genTypes, int genValues, int genEncoders, int genDecoders, int genJSONEncDec, int genPrinters, int genPrintersXML, int genFree, int novolatilefuncs, int genROSEDecoders)
{
	Module* currMod;
	AsnListNode* saveMods;
	FILE* srcFilePtr;
	// FILE		*hdrInterfaceFilePtr;
	// FILE		*hdrForwardDecl;
	DefinedObj* fNames;
	int fNameConflict = FALSE;

	genROSEDecoders = 1;

	// Save the gluecode to the output directory
	SaveResourceToFile(EDELPHI_ASN1_TYPES, "DelphiAsn1Types.pas");

	/*
	 * Make names for each module's encoder/decoder src and hdr files
	 * so import references can be made via include files
	 * check for truncation --> name conflicts & exit if nec
	 */
	fNames = NewObjList();
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (ObjIsDefined(fNames, currMod->delphiFileName, StrObjCmp)) // todo: change message or remove it
		{
			fprintf(errFileG, "Ack! ERROR---file name conflict for generated delphi file with name `%s'.\n\n", currMod->delphiFileName);
			fprintf(errFileG, "This usually means the max file name length is truncating the file names.\n");
			fprintf(errFileG, "Try re-naming the modules with shorter names or increasing the argument to -mf option (if you are using it).\n");
			fprintf(errFileG, "This error can also be caused by 2 modules having the same name but different OBJECT IDENTIFIERs.");
			fprintf(errFileG, "Try renaming the modules to correct this.\n");
			fNameConflict = TRUE;
		}
		else
		{
			DefineObj(&fNames, currMod->delphiFileName);
		}

		if (fNameConflict)
			return;

		FreeDefinedObjs(&fNames);
	}
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			if (fopen_s(&srcFilePtr, currMod->delphiFileName, "wt") != 0 || srcFilePtr == NULL)
			{
				perror("fopen");
			}
			else
			{
				saveMods = allMods->curr;
				PrintDelphiCode(srcFilePtr, allMods, currMod);
				allMods->curr = saveMods;
				fclose(srcFilePtr);
			}
		}
	}
} /* GenDelphiCode */

#if IDL
/*
 * Given the list of parsed, linked, normalized, error-checked and sorted
 * modules, and some code generation flags, generates C++ code and
 * writes it to files derived from each modules name.  Each module
 * gets 2 source files, one .h for data struct and prototypes, the other .C
 * for the enc/dec/print/free routine code.
 */
void GenIDLCode PARAMS((allMods, longJmpVal, genTypes, genValues, genPrinters, genFree), ModuleList* allMods _AND_ long longJmpVal _AND_ int genTypes _AND_ int genValues _AND_ int genPrinters _AND_ int genFree)
{
	Module* currMod;
	FILE* idlFilePtr;
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
		char *in, *out;
#if defined(_MSC_VER)
#pragma warning(disable : 6011)
#endif
		out = currMod->idlname = (char*)malloc(strlen(in = currMod->modId->name) + 1);
		do
			*out++ = (char)(*in == '-' ? '_' : *in);
		while (*in++);
#if defined(_MSC_VER)
#pragma warning(default : 6011)
#endif

		if (ObjIsDefined(fNames, currMod->idlFileName, StrObjCmp))
		{
			fprintf(errFileG, "Ack! ERROR---file name conflict for generated source file with name `%s'.\n\n", currMod->idlFileName);
			fprintf(errFileG, "This usually means the max file name length is truncating the file names.\n");
			fprintf(errFileG, "Try re-naming the modules with shorter names or increasing the argument to -mf option (if you are using it).\n");
			fprintf(errFileG, "This error can also be caused by 2 modules having the same name but different OBJECT IDENTIFIERs.");
			fprintf(errFileG, "Try renaming the modules to correct this.\n");
			fNameConflict = TRUE;
		}
		else
		{
			DefineObj(&fNames, currMod->idlFileName);
		}
	}
	if (fNameConflict)
		return;

	FreeDefinedObjs(&fNames);

	/*
	 * make C++ files
	 */
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		/*
		 * create and fill .h file for module's data structs
		 */
		if (fopen_s(&idlFilePtr, currMod->idlFileName, "w") != 0 || idlFilePtr == NULL)
			perror("fopen");
		else
		{
			PrintIDLCode(idlFilePtr, allMods, currMod, &idlRulesG, longJmpVal, genValues);

			fclose(idlFilePtr);
		}
	}
} /* GenIDLCode */
#endif /* IDL */

/*
 * returns 1 if the module names and oid's are unique.
 * otherwise returns 0
 */
int ModNamesUnique PARAMS((mods), ModuleList* mods)
{
	DefinedObj* names;
	DefinedObj* oids;
	Module* m;
	int retVal = 1;

	names = NewObjList();
	oids = NewObjList();

	FOR_EACH_LIST_ELMT(m, mods)
	{
		m->ImportUsed = FALSE;
		if (((m->modId->oid != NULL) && ObjIsDefined(oids, m->modId->oid, OidObjCmp)))
		{
			/* oops, 2 modules have the same oid */
			PrintErrLoc(m->asn1SrcFileName, (long)1);
			fprintf(errFileG, "ERROR---2 modules have the OBJECT IDENTIFIER `");
			PrintOid(stdout, m->modId->oid);
			fprintf(errFileG, "'.\n");
			retVal = 0;
		}
		/* name is only signficant if oid is empty */
		else if ((m->modId->oid == NULL) && (ObjIsDefined(names, m->modId->name, StrObjCmp)))
		{
			/* oops, 2 modules have the same name */
			PrintErrLoc(m->asn1SrcFileName, (long)1);
			fprintf(errFileG, "ERROR---2 modules have the name `%s'\n", m->modId->name);
			retVal = 0;
		}
		else
		{
			// Add in the Imported File
			DefineObj(&names, m->modId->name);
			if (m->modId->oid != NULL)
				DefineObj(&oids, m->modId->oid);
		}
	}
	FreeDefinedObjs(&names);
	FreeDefinedObjs(&oids);
	return retVal;
} /* ModNamesUnique */

void CreateNames(ModuleList* allMods)
{
	Module* currMod;
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		currMod->baseFilePath = MakeBaseFileName(currMod->asn1SrcFileName);
		currMod->baseFileName = RemovePathNonConst(currMod->baseFilePath);
		currMod->moduleName = MakeModuleName(currMod->baseFilePath);
		currMod->ROSEClassName = MakeROSEClassName(currMod->baseFilePath);
		currMod->cHdrFileName = MakeCHdrFileName(currMod->baseFilePath);
		currMod->cSrcFileName = MakeCSrcFileName(currMod->baseFilePath);
		currMod->cxxHdrFileName = MakeCxxHdrFileName(currMod->baseFilePath);
		currMod->cxxSrcFileName = MakeCxxSrcFileName(currMod->baseFilePath);
		currMod->swiftFileName = MakeSwiftFileName(currMod->baseFilePath);
		currMod->jsFileName = MakeJSFileName(currMod->baseFilePath);
		currMod->tsFileName = MakeTSFileName(currMod->baseFilePath);
		currMod->tsConverterFileName = MakeTSEncDecFileName(currMod->baseFilePath);
		currMod->idlFileName = MakeIDLFileName(currMod->baseFilePath);
		currMod->delphiFileName = MakeDelphiFileName(currMod->baseFilePath);
		currMod->ROSEHdrFileName = MakeROSEHdrFileName(currMod->baseFilePath);
		currMod->ROSESrcFileName = MakeROSESrcFileName(currMod->baseFilePath);
		currMod->ROSESrcCSFileName = MakeROSESrcCSFileName(currMod->baseFilePath);
		currMod->ROSEHdrInterfaceFileName = MakeROSEHdrInterfaceFileName(currMod->baseFilePath);
		currMod->ROSEHdrForwardDeclFileName = MakeROSEHdrForwardDeclFileName(currMod->baseFilePath);
		currMod->ROSESrcJAVAFileName = MakeROSESrcJAVAFileName(currMod->baseFilePath);
		currMod->ROSESwiftInterfaceFileName = MakeROSESwiftInterfaceFileName(currMod->baseFilePath);
	}
}

void snacc_exit_now(const char* szMethod, const char* szMessage, ...)
{
	va_list argptr;
	fprintf(stderr, "\nFatal error in %s\n", szMethod);
	va_start(argptr, szMessage);
	vfprintf(stderr, szMessage, argptr);
	va_end(argptr);
	fprintf(stderr, "\n");
	assert(false);
	exit(200);
}

int copy_file(const char* source, const char* destination)
{
	// Open the source file in binary read mode
	FILE* src = fopen(source, "rb");
	if (src == NULL)
	{
		perror("Error opening source file");
		return 1;
	}

	// Open the destination file in binary write mode
	FILE* dest = fopen(destination, "wb");
	if (dest == NULL)
	{
		perror("Error opening destination file");
		fclose(src);
		return 1;
	}

	// Buffer to hold file data during copying
	char buffer[4096];
	size_t bytesRead;

	// Copy the contents of the source file to the destination file
	while ((bytesRead = fread(buffer, 1, sizeof(buffer), src)) > 0)
	{
		size_t bytesWritten = fwrite(buffer, 1, bytesRead, dest);
		if (bytesWritten != bytesRead)
		{
			perror("Error writing to destination file");
			fclose(src);
			fclose(dest);
			return 1;
		}
	}

	// Check if reading from the source file failed
	if (ferror(src))
	{
		perror("Error reading source file");
		fclose(src);
		fclose(dest);
		return 1;
	}

	// Close both files
	fclose(src);
	fclose(dest);

	return 0;
}
