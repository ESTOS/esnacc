/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison implementation for Yacc-like parsers in C

	  Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if defined(_MSC_VER)
#pragma warning(disable : 28182)
#endif

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.7"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
#line 28 "core\\parse-asn1.y"

#include "../../snacc.h"

#include <string.h>
// #include <stdio.h>

#include "../../c-lib/include/asn-incl.h"
#include "mem.h"
#include "asn1module.h"
#include "lib-types.h"
#include "snacc-util.h"
#include "exports.h"
#include "lex-stuff.h"

int OidArcNameToNum PROTO((char* name));
int yylex();
static void yyerror(char* s);

extern FILE* errFileG; // Defined in snacc.c

/*
 * smallErrG
 *    used for small errors that should prevent code generation but not
 *    prevent the later error checking passes
 */
int smallErrG = FALSE;

#ifdef FLEX_IN_USE
/*
 * firstTimeThroughG
 *    used incase the asn1.lex was compiled with flex in which
 *    case the lexical analyzer must be reset for every ASN.1 file
 *    parsed, except the first
 */
static int firstTimeThroughG = TRUE;
#endif

/*
 *  modulePtrG
 *    used to hold the parsed value.  The root of the parse tree.
 */
Module* modulePtrG;

/*
 * oidElmtValDefsG
 *    used to hold integer values that are defined as arc numbers
 *    the modules object identifiers.
 * eg. FOO-MODULE { joint-iso-ccitt dod (2) foo (2) 3 2 } DEFINITIONS ::=
 *     would put dod/2 and foo/2 in the oidElmtValDefsG list
 * Note: only some oid's (modules name/import list module names)
 *       are parsed by the yacc code.  The rest are parsed later
 *       due to ambiguities that arise without type info.
// REN -- 9/23/02
ValueDefList *oidElmtValDefsG = NULL;
 */

/*
 * ApplTag
 *   used to hold APPLICATION tags that have been defined in
 *   a module.  This permits checking for the the error of
 *   using the same APPLICATION tag in 1 module.  The
 *   ApplTags list (appTagsG) is emptied for each module.
 */
typedef struct ApplTag
{
	unsigned long lineNo;
	unsigned long tagCode;
	struct ApplTag* next;
} ApplTag;

ApplTag* applTagsG = NULL;

/*
 * Protos for ApplTag related stuff. These are defined at the
 * end  of this file
 */
void PushApplTag PROTO((unsigned long tagCode, unsigned long lineNo));
void FreeApplTags();

/*
 * the following are globals to simplify disparity between
 * productions and produced data structure
 */

/*
 * used to set exports flag in Type/value defs
 * exportListG holds the explicitly exported elements.
 * see SetExports routine in export.c
 */
ExportElmt* exportListG = NULL;
int exportsParsedG;

/*
 * globals for the APPLICATION-CONTEXT macro productions
 */
static ValueList* rosAcSymmetricAsesG;
static ValueList* rosAcResponderConsumerOfG;
static ValueList* rosAcInitiatorConsumerOfG;

/*
 * used with MTSAS Extension macro
 * set to NULL for the initial parse.
 */
static AsnBool* mtsasCriticalForSubmissionG = NULL;
static AsnBool* mtsasCriticalForTransferG = NULL;
static AsnBool* mtsasCriticalForDeliveryG = NULL;

/*
 * Asn PORT macro globals
 */
static TypeOrValueList* asnConsumerG;
static TypeOrValueList* asnSupplierG;

/*
 * parseErrCountG
 *   used to prevent too many cascade errors
 */
int parseErrCountG = 0;
#define MAX_ERR 50
#define PARSE_ERROR()                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
	parseErrCountG++;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
	modulePtrG->status = MOD_ERROR;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
	if (parseErrCountG > MAX_ERR)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
	{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
		fprintf(errFileG, "Ackkkkk! too many errors - bye!\n");                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
		YYABORT;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
	}

/* Line 371 of yacc.c  */
#line 216 "core\\parse-asn1.c"

#ifndef YY_NULL
#if defined __cplusplus && 201103L <= __cplusplus
#define YY_NULL nullptr
#else
#define YY_NULL 0
#endif
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
#undef YYERROR_VERBOSE
#define YYERROR_VERBOSE 1
#else
#define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "parse-asn1.h".  */
#ifndef YY_YY_CORE_PARSE_ASN1_H_INCLUDED
#define YY_YY_CORE_PARSE_ASN1_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
#define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
#define YYTOKENTYPE
/* Put the tokens into the symbol table, so that GDB and other debuggers
   know about them.  */
enum yytokentype
{
	BSTRING_SYM = 258,
	HSTRING_SYM = 259,
	CSTRING_SYM = 260,
	UCASEFIRST_IDENT_SYM = 261,
	LCASEFIRST_IDENT_SYM = 262,
	NAMEDMACRO_SYM = 263,
	MACRODEFBODY_SYM = 264,
	BRACEBAL_SYM = 265,
	NUMBER_ERANGE = 266,
	NUMBER_SYM = 267,
	SNACC_ATTRIBUTES = 268,
	COLON_SYM = 269,
	ELLIPSIS_SYM = 270,
	DOT_SYM = 271,
	COMMA_SYM = 272,
	LEFTBRACE_SYM = 273,
	RIGHTBRACE_SYM = 274,
	LEFTPAREN_SYM = 275,
	RIGHTPAREN_SYM = 276,
	LEFTBRACKET_SYM = 277,
	RIGHTBRACKET_SYM = 278,
	LESSTHAN_SYM = 279,
	MINUS_SYM = 280,
	GETS_SYM = 281,
	BAR_SYM = 282,
	TAGS_SYM = 283,
	BOOLEAN_SYM = 284,
	INTEGER_SYM = 285,
	BIT_SYM = 286,
	STRING_SYM = 287,
	OCTET_SYM = 288,
	CONTAINING_SYM = 289,
	ENCODED_SYM = 290,
	NULL_SYM = 291,
	SEQUENCE_SYM = 292,
	OF_SYM = 293,
	SET_SYM = 294,
	IMPLICIT_SYM = 295,
	CHOICE_SYM = 296,
	ANY_SYM = 297,
	OBJECT_IDENTIFIER_SYM = 298,
	RELATIVE_OID_SYM = 299,
	OPTIONAL_SYM = 300,
	DEFAULT_SYM = 301,
	COMPONENTS_SYM = 302,
	UNIVERSAL_SYM = 303,
	APPLICATION_SYM = 304,
	PRIVATE_SYM = 305,
	TRUE_SYM = 306,
	FALSE_SYM = 307,
	BEGIN_SYM = 308,
	END_SYM = 309,
	DEFINITIONS_SYM = 310,
	EXPLICIT_SYM = 311,
	ENUMERATED_SYM = 312,
	EXPORTS_SYM = 313,
	IMPORTS_SYM = 314,
	REAL_SYM = 315,
	INCLUDES_SYM = 316,
	MIN_SYM = 317,
	MAX_SYM = 318,
	SIZE_SYM = 319,
	FROM_SYM = 320,
	WITH_SYM = 321,
	COMPONENT_SYM = 322,
	PRESENT_SYM = 323,
	ABSENT_SYM = 324,
	DEFINED_SYM = 325,
	BY_SYM = 326,
	PLUS_INFINITY_SYM = 327,
	MINUS_INFINITY_SYM = 328,
	SEMI_COLON_SYM = 329,
	IA5STRING_SYM = 330,
	PRINTABLESTRING_SYM = 331,
	NUMERICSTRING_SYM = 332,
	TELETEXSTRING_SYM = 333,
	T61STRING_SYM = 334,
	VIDEOTEXSTRING_SYM = 335,
	VISIBLESTRING_SYM = 336,
	ISO646STRING_SYM = 337,
	GRAPHICSTRING_SYM = 338,
	GENERALSTRING_SYM = 339,
	BMPSTRING_SYM = 340,
	UNIVERSALSTRING_SYM = 341,
	UTF8STRING_SYM = 342,
	GENERALIZEDTIME_SYM = 343,
	UTCTIME_SYM = 344,
	EXTERNAL_SYM = 345,
	OBJECTDESCRIPTOR_SYM = 346,
	OPERATION_SYM = 347,
	ARGUMENT_SYM = 348,
	RESULT_SYM = 349,
	ERRORS_SYM = 350,
	LINKED_SYM = 351,
	ERROR_SYM = 352,
	PARAMETER_SYM = 353,
	BIND_SYM = 354,
	BINDERROR_SYM = 355,
	UNBIND_SYM = 356,
	UNBINDERROR_SYM = 357,
	ASE_SYM = 358,
	OPERATIONS_SYM = 359,
	CONSUMERINVOKES_SYM = 360,
	SUPPLIERINVOKES_SYM = 361,
	AC_SYM = 362,
	ASES_SYM = 363,
	REMOTE_SYM = 364,
	INITIATOR_SYM = 365,
	RESPONDER_SYM = 366,
	ABSTRACTSYNTAXES_SYM = 367,
	CONSUMER_SYM = 368,
	EXTENSIONS_SYM = 369,
	CHOSEN_SYM = 370,
	EXTENSION_SYM = 371,
	CRITICAL_SYM = 372,
	FOR_SYM = 373,
	DELIVERY_SYM = 374,
	SUBMISSION_SYM = 375,
	TRANSFER_SYM = 376,
	EXTENSIONATTRIBUTE_SYM = 377,
	TOKEN_SYM = 378,
	TOKENDATA_SYM = 379,
	SECURITYCATEGORY_SYM = 380,
	OBJECT_SYM = 381,
	PORTS_SYM = 382,
	BOXC_SYM = 383,
	BOXS_SYM = 384,
	PORT_SYM = 385,
	ABSTRACTOPS_SYM = 386,
	REFINE_SYM = 387,
	AS_SYM = 388,
	RECURRING_SYM = 389,
	VISIBLE_SYM = 390,
	PAIRED_SYM = 391,
	ABSTRACTBIND_SYM = 392,
	ABSTRACTUNBIND_SYM = 393,
	TO_SYM = 394,
	ABSTRACTERROR_SYM = 395,
	ABSTRACTOPERATION_SYM = 396,
	ALGORITHM_SYM = 397,
	ENCRYPTED_SYM = 398,
	SIGNED_SYM = 399,
	SIGNATURE_SYM = 400,
	PROTECTED_SYM = 401,
	OBJECTTYPE_SYM = 402,
	SYNTAX_SYM = 403,
	ACCESS_SYM = 404,
	STATUS_SYM = 405,
	DESCRIPTION_SYM = 406,
	REFERENCE_SYM = 407,
	INDEX_SYM = 408,
	DEFVAL_SYM = 409,
	ATTRIB_ASN1_TYPE_ID = 410,
	ATTRIB_C_TYPE_ID = 411,
	ATTRIB_C_TYPE_NAME_SYM = 412,
	ATTRIB_C_FIELD_NAME_SYM = 413,
	ATTRIB_IS_PDU_SYM = 414,
	ATTRIB_IS_PTR_SYM = 415,
	ATTRIB_IS_PTR_TYPEDEF_SYM = 416,
	ATTRIB_IS_PTR_TYPE_REF_SYM = 417,
	ATTRIB_IS_PTR_IN_CHOICE_SYM = 418,
	ATTRIB_IS_PTR_FOR_OPT_SYM = 419,
	ATTRIB_OPT_TEST_ROUTINE_SYM = 420,
	ATTRIB_DEFAULT_FIELD_SYM = 421,
	ATTRIB_PRINT_ROUTINE_SYM = 422,
	ATTRIB_ENCODE_ROUTINE_SYM = 423,
	ATTRIB_DECODE_ROUTINE_SYM = 424,
	ATTRIB_FREE_ROUTINE_SYM = 425,
	ATTRIB_IS_ENC_DEC_SYM = 426,
	ATTRIB_GEN_TYPEDEF_SYM = 427,
	ATTRIB_GEN_PRINT_ROUTINE_SYM = 428,
	ATTRIB_GEN_ENCODE_ROUTINE_SYM = 429,
	ATTRIB_GEN_DECODE_ROUTINE_SYM = 430,
	ATTRIB_GEN_FREE_ROUTINE_SYM = 431,
	ATTRIB_CHOICE_ID_SYMBOL_SYM = 432,
	ATTRIB_CHOICE_ID_VALUE_SYM = 433,
	ATTRIB_CHOICE_ID_ENUM_NAME_SYM = 434,
	ATTRIB_CHOICE_ID_ENUM_FIELD_NAME_SYM = 435,
	ATTRIB_IS_BIG_INT_SYM = 436,
	ATTRIB_NAMESPACE_SYM = 437,
	C_CHOICE_SYM = 438,
	C_LIST_SYM = 439,
	C_ANY_SYM = 440,
	C_ANYDEFBY_SYM = 441,
	C_LIB_SYM = 442,
	C_STRUCT_SYM = 443,
	C_TYPEDEF_SYM = 444,
	C_TYPEREF_SYM = 445,
	C_NO_TYPE_SYM = 446,
	UNKNOWN_SYM = 447,
	BITSTRING_SYM = 448,
	OCTETSTRING_SYM = 449,
	SEQUENCE_OF_SYM = 450,
	SET_OF_SYM = 451,
	ANY_DEFINED_BY_SYM = 452,
	LOCAL_TYPE_REF_SYM = 453,
	IMPORT_TYPE_REF_SYM = 454
};
#endif

#if !defined YYSTYPE && !defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
	/* Line 387 of yacc.c  */
#line 183 "core\\parse-asn1.y"

	int intVal;
	unsigned int uintVal;
	char* charPtr;
	Type* typePtr;
	NamedType* namedTypePtr;
	NamedTypeList* namedTypeListPtr;
	Value* valuePtr;
	NamedValue* namedValuePtr;
	SubtypeValue* subtypeValuePtr;
	Subtype* subtypePtr;
	ModuleId* moduleId;
	OID* oidPtr;
	OidList* oidListPtr;
	TypeDef* typeDefPtr;
	TypeDefList* typeDefListPtr;
	ValueDef* valueDefPtr;
	ValueDefList* valueDefListPtr;
	ExportElmt* exportList;
	ImportModule* importModulePtr;
	ImportModuleList* importModuleListPtr;
	ImportElmt* importElmtPtr;
	ImportElmtList* importElmtListPtr;
	Tag* tagPtr;
	TagList* tagListPtr;
	Constraint* constraintPtr;
	ConstraintList* constraintListPtr;
	InnerSubtype* innerSubtypePtr;
	ValueRangeEndpoint* valueRangeEndpointPtr;
	ValueList* valueListPtr;
	TypeOrValueList* typeOrValueListPtr;
	TypeOrValue* typeOrValuePtr;
	AsnPort* asnPortPtr;
	AsnPortList* asnPortListPtr;
	SnaccDirectiveList* directiveList;
	SnaccDirective* directivePtr;
	SnaccDirectiveEnum directiveEnum;
	enum BasicTypeChoiceId basicTypeChoiceIdEnum;
	CTypeId cTypeIdEnum;
	AsnBool boolVal;

	/* Line 387 of yacc.c  */
#line 501 "core\\parse-asn1.c"
} YYSTYPE;
#define YYSTYPE_IS_TRIVIAL 1
#define yystype YYSTYPE /* obsolescent; will be withdrawn */
#define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse(void* YYPARSE_PARAM);
#else
int yyparse();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse(void);
#else
int yyparse();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_CORE_PARSE_ASN1_H_INCLUDED  */

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 529 "core\\parse-asn1.c"

#ifdef short
#undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
#ifdef __SIZE_TYPE__
#define YYSIZE_T __SIZE_TYPE__
#elif defined size_t
#define YYSIZE_T size_t
#elif !defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
#include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#define YYSIZE_T size_t
#else
#define YYSIZE_T unsigned int
#endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T)-1)

#ifndef YY_
#if defined YYENABLE_NLS && YYENABLE_NLS
#if ENABLE_NLS
#include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#define YY_(Msgid) dgettext("bison-runtime", Msgid)
#endif
#endif
#ifndef YY_
#define YY_(Msgid) Msgid
#endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if !defined lint || defined __GNUC__
#define YYUSE(E) ((void)(E))
#else
#define YYUSE(E) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
#define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
static int YYID(int yyi)
#else
static int YYID(yyi)
int yyi;
#endif
{
	return yyi;
}
#endif

#if !defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

#ifdef YYSTACK_USE_ALLOCA
#if YYSTACK_USE_ALLOCA
#ifdef __GNUC__
#define YYSTACK_ALLOC __builtin_alloca
#elif defined __BUILTIN_VA_ARG_INCR
#include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#elif defined _AIX
#define YYSTACK_ALLOC __alloca
#elif defined _MSC_VER
#include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#define alloca _alloca
#else
#define YYSTACK_ALLOC alloca
#if !defined _ALLOCA_H && !defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
#include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
/* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#endif
#endif
#endif
#endif

#ifdef YYSTACK_ALLOC
/* Pacify GCC's `empty if-body' warning.  */
#define YYSTACK_FREE(Ptr)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
	do                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
	{ /* empty */                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
		;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
	} while (YYID(0))
#ifndef YYSTACK_ALLOC_MAXIMUM
/* The OS might guarantee only one guard page at the bottom of the stack,
   and a page size can be as small as 4096 bytes.  So we cannot safely
   invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
   to allow for a few compiler-allocated temporary stack slots.  */
#define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#endif
#else
#define YYSTACK_ALLOC YYMALLOC
#define YYSTACK_FREE YYFREE
#ifndef YYSTACK_ALLOC_MAXIMUM
#define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#endif
#if (defined __cplusplus && !defined EXIT_SUCCESS && !((defined YYMALLOC || defined malloc) && (defined YYFREE || defined free)))
#include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#endif
#ifndef YYMALLOC
#define YYMALLOC malloc
#if !defined malloc && !defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
void* malloc(YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#endif
#endif
#ifndef YYFREE
#define YYFREE free
#if !defined free && !defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
void free(void*);		/* INFRINGES ON USER NAME SPACE */
#endif
#endif
#endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */

#if (!defined yyoverflow && (!defined __cplusplus || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
	yytype_int16 yyss_alloc;
	YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
#define YYSTACK_GAP_MAXIMUM (sizeof(union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
#define YYSTACK_BYTES(N) ((N) * (sizeof(yytype_int16) + sizeof(YYSTYPE)) + YYSTACK_GAP_MAXIMUM)

#define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
#define YYSTACK_RELOCATE(Stack_alloc, Stack)                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
	do                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
	{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
		YYSIZE_T yynewbytes;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
		YYCOPY(&yyptr->Stack_alloc, Stack, yysize);                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
		Stack = &yyptr->Stack_alloc;                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
		yynewbytes = yystacksize * sizeof(*Stack) + YYSTACK_GAP_MAXIMUM;                                                                                                                                                                                                                                                                                                                                                                                                                                           \
		yyptr += yynewbytes / sizeof(*yyptr);                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
	} while (YYID(0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
#ifndef YYCOPY
#if defined __GNUC__ && 1 < __GNUC__
#define YYCOPY(Dst, Src, Count) __builtin_memcpy(Dst, Src, (Count) * sizeof(*(Src)))
#else
#define YYCOPY(Dst, Src, Count)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
	do                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
	{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
		YYSIZE_T yyi;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
		for (yyi = 0; yyi < (Count); yyi++)                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
			(Dst)[yyi] = (Src)[yyi];                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
	} while (YYID(0))
#endif
#endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL 5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST 3514

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS 201
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS 216
/* YYNRULES -- Number of rules.  */
#define YYNRULES 521
/* YYNRULES -- Number of states.  */
#define YYNSTATES 836

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK 2
#define YYMAXUTOK 454

#define YYTRANSLATE(YYX) ((unsigned int)(YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] = {0,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 200, 2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,
										   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,
										   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 2,	  2,   2,	2,	 1,	  2,   3,	4,	 5,	  6,   7,	8,	 9,	  10,  11,	12,	 13,  14,  15,	16,	 17,
										   18,	19,	 20,  21,  22,	23,	 24,  25,  26,	27,	 28,  29,  30,	31,	 32,  33,  34,	35,	 36,  37,  38,	39,	 40,  41,  42,	43,	 44,  45,  46,	47,	 48,  49,  50,	51,	 52,  53,  54,	55,	 56,  57,  58,	59,	 60,  61,  62,	63,	 64,  65,  66,	67,	 68,  69,  70,	71,	 72,  73,  74,	75,	 76,  77,  78,	79,	 80,  81,  82,	83,	 84,  85,  86,	87,	 88,  89,  90,	91,	 92,  93,  94,	95,	 96,  97,  98,	99,	 100, 101, 102, 103, 104, 105, 106, 107, 108,
										   109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] = {0,	 0,	   3,	 4,	   5,	 16,   19,	 22,   24,	 26,   28,	 31,   33,	 35,   39,	 41,   45,	 47,   51,	 55,   57,	 59,   61,	 63,   68,	 72,   76,	 78,   80,	 82,   85,	 87,   92,	 96,   98,	 100,  102,	 104,  107,	 109,  111,	 114,  117,	 119,  121,	 122,  129,	 133,  139,	 141,  143,	 150,  155,	 157,  159,	 161,  163,	 165,  167,	 169,  171,	 173,  175,	 177,  179,	 181,  183,	 185,  187,	 189,  191,	 193,  195,	 197,  199,
									   201,	 203,  206,	 208,  210,	 212,  215,	 217,  219,	 221,  226,	 228,  232,	 237,  242,	 244,  246,	 249,  252,	 257,  259,	 262,  268,	 270,  272,	 274,  278,	 282,  285,	 288,  290,	 296,  298,	 301,  304,	 308,  310,	 312,  316,	 318,  325,	 331,  333,	 336,  338,	 340,  342,	 344,  347,	 351,  355,	 360,  364,	 368,  372,	 375,  379,	 385,  388,	 390,  395,	 397,  400,	 403,  407,	 409,  411,	 416,  418,	 425,  427,	 431,  435,	 438,  442,
									   446,	 451,  453,	 455,  457,	 459,  461,	 463,  465,	 470,  477,	 482,  484,	 486,  488,	 490,  492,	 494,  496,	 498,  500,	 502,  504,	 506,  508,	 510,  512,	 514,  516,	 518,  521,	 526,  531,	 535,  537,	 541,  543,	 545,  547,	 549,  551,	 553,  555,	 557,  559,	 562,  567,	 569,  572,	 574,  577,	 579,  581,	 583,  585,	 588,  591,	 595,  599,	 601,  603,	 605,  609,	 615,  617,	 621,  624,	 626,  629,	 631,  633,	 635,  637,	 639,  641,
									   647,	 649,  651,	 653,  655,	 660,  662,	 664,  666,	 668,  670,	 672,  674,	 675,  679,	 681,  683,	 685,  687,	 689,  691,	 694,  698,	 701,  703,	 705,  707,	 709,  711,	 713,  718,	 723,  725,	 727,  729,	 731,  733,	 735,  737,	 739,  740,	 742,  744,	 746,  749,	 753,  757,	 761,  765,	 769,  771,	 773,  775,	 777,  779,	 781,  783,	 785,  787,	 789,  791,	 793,  795,	 797,  799,	 801,  803,	 805,  807,	 809,  811,	 813,  815,	 817,  819,
									   821,	 823,  825,	 827,  829,	 831,  833,	 835,  837,	 839,  841,	 843,  845,	 847,  849,	 851,  853,	 855,  857,	 859,  861,	 863,  865,	 867,  869,	 871,  873,	 875,  877,	 879,  881,	 883,  885,	 887,  889,	 891,  893,	 895,  897,	 899,  901,	 903,  905,	 907,  909,	 911,  913,	 915,  917,	 919,  921,	 923,  925,	 927,  929,	 931,  933,	 935,  937,	 939,  941,	 943,  945,	 947,  949,	 951,  953,	 955,  957,	 959,  961,	 963,  965,	 967,  969,
									   971,	 973,  975,	 977,  979,	 981,  983,	 985,  987,	 989,  991,	 993,  996,	 1001, 1004, 1006, 1009, 1011, 1013, 1015, 1020, 1022, 1027, 1029, 1032, 1035, 1037, 1042, 1045, 1047, 1050, 1052, 1055, 1057, 1062, 1065, 1067, 1070, 1074, 1079, 1084, 1086, 1091, 1093, 1095, 1104, 1109, 1117, 1119, 1125, 1127, 1130, 1137, 1139, 1146, 1148, 1153, 1155, 1157, 1161, 1168, 1170, 1172, 1174, 1178, 1180, 1182, 1184, 1188, 1190, 1192, 1197, 1199, 1202, 1204,
									   1208, 1210, 1212, 1216, 1218, 1220, 1222, 1224, 1227, 1229, 1232, 1234, 1237, 1239, 1242, 1245, 1250, 1252, 1254, 1258, 1261, 1263, 1265, 1267, 1270, 1272, 1277, 1279, 1281, 1284, 1287, 1292, 1297, 1302, 1304, 1308, 1311, 1313, 1316, 1318, 1322, 1326, 1328, 1332, 1334, 1338, 1340, 1343, 1347, 1352, 1354, 1357, 1361, 1366, 1368, 1371, 1374, 1378, 1381, 1384, 1387, 1390, 1402, 1404, 1406, 1409, 1410, 1413, 1414, 1419, 1420, 1425};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] = {203, 0,   -1,	-1,	 -1,  207, 55,	205, 204, 206, 26,	53,	 209, 210, 54,	-1,	 56,  28,  -1,	40,	 28,  -1,  325, -1,	 15,  -1,  325, -1,	 323, 208, -1,	312, -1,  325, -1,	182, 200, 5,   -1,	325, -1,  211, 214, 220, -1,  325, -1,	58,	 212, 74,  -1,	58,	 1,	  74,  -1,	325, -1,  213, -1,	325, -1,  219, -1,	213, 17,  202, 219, -1,	 59,  215, 74,	-1,	 59,  1,   74,	-1,	 325, -1,  216, -1,	 325, -1,  216, 217, -1,  217, -1,	218, 65,  202,
									 207, -1,  218, 17,	 219, -1,  219, -1,	 324, -1,  322, -1,	 336, -1,  220, 221, -1,  221, -1,	222, -1,  222, 74,	-1,	 1,	  74,  -1,	225, -1,  302, -1,	-1,	 8,	  26,  53,	202, 223, 9,   -1,	8,	 26,  224, -1,	8,	 26,  323, 16,	224, -1,  324, -1,	336, -1,  324, 26,	326, 202, 228, 326, -1,	 323, 16,  202, 324, -1,  226, -1,	324, -1,  335, -1,	229, -1,  227, -1,	278, -1,  231, -1,	232, -1,  273, -1,	238, -1,  240, -1,	241, -1,  243,
									 -1,  254, -1,	256, -1,  257, -1,	258, -1,  266, -1,	267, -1,  271, -1,	274, -1,  236, -1,	237, -1,  272, -1,	33,	 32,  -1,  277, -1,	 276, -1,  275, -1,	 322, 228, -1,	228, -1,  29,  -1,	30,	 -1,  30,  18,	233, 19,  -1,  234, -1,	 233, 17,  234, -1,	 322, 20,  235, 21,	 -1,  322, 20,	304, 21,  -1,  12,	-1,	 11,  -1,  25,	12,	 -1,  25,  11,	-1,	 57,  18,  233, 19,	 -1,  60,  -1,	31,	 32,  -1,  31,	32,	 18,  239, 19,	-1,	 233, -1,
									 36,  -1,  90,	-1,	 37,  202, 18,	-1,	 242, 244, 19,	-1,	 242, 19,  -1,	245, 326, -1,  246, -1,	 253, 17,  326, 202, 245, -1,  253, -1,	 15,  247, -1,	17,	 15,  -1,  17,	248, 252, -1,  325, -1,	 249, -1,  248, 17,	 249, -1,  253, -1,	 22,  22,  251, 250, 23,  23,  -1,	250, 17,  326, 202, 253, -1,  253, -1,	321, 14,  -1,  325, -1,	 15,  -1,  325, -1,	 230, -1,  230, 45,	 -1,  230, 46,	311, -1,  47,  38,	228, -1,  322, 47,	38,	 228, -1,
									 37,  38,  228, -1,	 39,  202, 18,	-1,	 255, 244, 19,	-1,	 255, 19,  -1,	39,	 38,  228, -1,	41,	 202, 18,  259, 19,	 -1,  260, 326, -1,	 230, -1,  230, 17,	 326, 260, -1,	261, -1,  15,  262, -1,	 17,  15,  -1,	17,	 263, 252, -1,	325, -1,  264, -1,	263, 17,  326, 264, -1,	 230, -1,  22,	22,	 251, 265, 23,	23,	 -1,  230, -1,	265, 17,  230, -1,	322, 24,  228, -1,	268, 228, -1,  268, 40,	 228, -1,  268, 56,	 228, -1,  22,	270, 269, 23,
									 -1,  321, -1,	304, -1,  48,  -1,	49,	 -1,  50,  -1,	325, -1,  42,  -1,	42,	 70,  71,  322, -1,	 33,  32,  20,	34,	 228, 21,  -1,	31,	 32,  34,  228, -1,	 43,  -1,  44,	-1,	 88,  -1,  89,	-1,	 91,  -1,  77,	-1,	 76,  -1,  75,	-1,	 85,  -1,  86,	-1,	 87,  -1,  78,	-1,	 79,  -1,  80,	-1,	 81,  -1,  82,	-1,	 83,  -1,  84,	-1,	 228, 279, -1,	39,	 290, 38,  228, -1,	 37,  290, 38,	228, -1,  20,  280, 21,	 -1,  281, -1,	280, 27,  281,
									 -1,  283, -1,	284, -1,  285, -1,	291, -1,  290, -1,	292, -1,  282, -1,	15,	 -1,  303, -1,	61,	 228, -1,  286, 16,	 16,  287, -1,	288, -1,  288, 24,	-1,	 289, -1,  24,	289, -1,  303, -1,	62,	 -1,  303, -1,	63,	 -1,  64,  279, -1,	 65,  279, -1,	66,	 67,  293, -1,	66,	 47,  294, -1,	279, -1,  295, -1,	296, -1,  18,  297, 19,	 -1,  18,  15,	17,	 297, 19,  -1,	298, -1,  297, 17,	298, -1,  322, 299, -1,	 299, -1,  300, 301, -1,  279,
									 -1,  325, -1,	68,	 -1,  69,  -1,	325, -1,  45,  -1,	322, 228, 26,  202, 303, -1,  306, -1,	304, -1,  305, -1,	322, -1,  323, 16,	202, 322, -1,  308, -1,	 310, -1,  309, -1,	 235, -1,  319, -1,	 318, -1,  320, -1,	 -1,  18,  307, 10,	 -1,  51,  -1,	52,	 -1,  72,  -1,	73,	 -1,  36,  -1,	303, -1,  322, 303, -1,	 18,  313, 19,	-1,	 313, 314, -1,	314, -1,  315, -1,	316, -1,  317, -1,	321, -1,  322, -1,	322, 20,  315, 21,	-1,	 322, 20,
									 304, 21,  -1,	3,	 -1,  4,   -1,	5,	 -1,  12,  -1,	11,	 -1,  7,   -1,	6,	 -1,  6,   -1,	-1,	 327, -1,  325, -1,	 328, -1,  327, 328, -1,  155, 200, 329, -1,  156, 200, 330, -1,  331, 200, 332, -1,  333, 200, 5,	 -1,  334, 200, 12,	 -1,  192, -1,	29,	 -1,  30,  -1,	193, -1,  194, -1,	36,	 -1,  43,  -1,	60,	 -1,  57,  -1,	37,	 -1,  195, -1,	39,	 -1,  196, -1,	41,	 -1,  42,  -1,	197, -1,  198, -1,	199, -1,  77,  -1,	76,	 -1,  75,
									 -1,  85,  -1,	86,	 -1,  87,  -1,	78,	 -1,  44,  -1,	183, -1,  184, -1,	185, -1,  186, -1,	187, -1,  188, -1,	189, -1,  190, -1,	191, -1,  159, -1,	160, -1,  161, -1,	162, -1,  163, -1,	164, -1,  171, -1,	172, -1,  173, -1,	174, -1,  175, -1,	176, -1,  181, -1,	51,	 -1,  52,  -1,	157, -1,  158, -1,	165, -1,  166, -1,	167, -1,  168, -1,	169, -1,  170, -1,	177, -1,  179, -1,	180, -1,  178, -1,	337, -1,  344, -1,	346, -1,  350,
									 -1,  352, -1,	357, -1,  372, -1,	366, -1,  377, -1,	378, -1,  379, -1,	380, -1,  381, -1,	386, -1,  390, -1,	399, -1,  401, -1,	403, -1,  404, -1,	405, -1,  406, -1,	409, -1,  408, -1,	407, -1,  410, -1,	92,	 -1,  97,  -1,	99,	 -1,  101, -1,	103, -1,  107, -1,	116, -1,  114, -1,	122, -1,  123, -1,	124, -1,  125, -1,	126, -1,  130, -1,	132, -1,  137, -1,	138, -1,  141, -1,	140, -1,  142, -1,	143, -1,  144, -1,	145, -1,  146,
									 -1,  147, -1,	92,	 338, -1,  339, 340, 342, 343, -1,	93,	 230, -1,  325, -1,	 94,  341, -1,	325, -1,  230, -1,	325, -1,  95,  18,	369, 19,  -1,  325, -1,	 96,  18,  369, 19,	 -1,  325, -1,	97,	 345, -1,  98,	230, -1,  325, -1,	99,	 347, 348, 349, -1,	 93,  230, -1,	325, -1,  94,  230, -1,	 325, -1,  100, 230, -1,  325, -1,	101, 347, 348, 351, -1,	 102, 230, -1,	325, -1,  103, 353, -1,	 103, 354, 355, -1,	 104, 18,  356, 19,	 -1,  105,
									 18,  356, 19,	-1,	 325, -1,  106, 18,	 356, 19,  -1,	325, -1,  368, -1,	107, 358, 99,  228, 101, 228, 359, 364, -1,	 108, 18,  368, 19,	 -1,  109, 104, 18,	 303, 19,  360, 361, -1,  325, -1,	104, 38,  18,  368, 19,	 -1,  325, -1,	362, 363, -1,  110, 113, 38,  18,  368, 19,	 -1,  325, -1,	111, 113, 38,  18,	368, 19,  -1,  325, -1,	 112, 18,  365, 19,	 -1,  325, -1,	312, -1,  365, 17,	312, -1,  114, 115, 65,	 18,  367, 19,	-1,	 368, -1,
									 325, -1,  303, -1,	 368, 17,  303, -1,	 370, -1,  325, -1,	 371, -1,  370, 17,	 371, -1,  228, -1,	 303, -1,  116, 230, 373, 374, -1,	116, -1,  46,  303, -1,	 325, -1,  117, 118, 375, -1,  325, -1,	 376, -1,  375, 17,	 376, -1,  120, -1,	 121, -1,  119, -1,	 122, -1,  122, 228, -1,  123, -1,	123, 228, -1,  124, -1,	 124, 228, -1,	125, -1,  125, 228, -1,	 126, 382, -1,	127, 18,  383, 19,	-1,	 325, -1,  384, -1,	 383, 17,  384, -1,	 303, 385,
									 -1,  128, -1,	129, -1,  325, -1,	130, 387, -1,  130, -1,	 131, 18,  370, 19,	 -1,  388, -1,	389, -1,  388, 389, -1,	 389, 388, -1,	105, 18,  370, 19,	-1,	 106, 18,  370, 19,	 -1,  132, 398, 133, 391, -1,  392, -1,	 391, 17,  392, -1,	 393, 394, -1,	398, -1,  398, 134, -1,	 395, -1,  394, 17,	 395, -1,  303, 385, 396, -1,  135, -1,	 136, 66,  397, -1,	 398, -1,  397, 17,	 398, -1,  303, -1,	 137, 400, -1,	137, 400, 228, -1,	139, 18,  383,
									 19,  -1,  325, -1,	 138, 402, -1,	138, 402, 228, -1,	65,	 18,  383, 19,	-1,	 325, -1,  141, 338, -1,  140, 345, -1,	 142, 98,  228, -1,	 143, 228, -1,	144, 228, -1,  145, 228, -1,  146, 228, -1,	 147, 148, 228, 149, 411, 150, 412, 413, 414, 415, 416, -1,	 322, -1,  322, -1,	 151, 303, -1,	-1,	 152, 303, -1,	-1,	 153, 18,  370, 19,	 -1,  -1,  154, 18,	 303, 19,  -1,	-1};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] = {0,	  481,	481,  486,	484,  526,	527,  528,	536,  540,	549,  558,	559,  563,	564,  569,	570,  574,	584,  591,	595,  596,	600,  607,	617,  618,	623,  627,	628,  632,	636,  644,	655,  665,	679,  680,	681,  695,	696,  700,	701,  702,	710,  724,	737,  737,	778,  794,	820,  821,	829,  850,	865,  866,	877,  878,	879,  880,	884,  885,	886,  887,	888,  889,	890,  891,	892,  893,	894,  895,	896,  897,	898,  899,	900,
										901,  902,	906,  907,	908,  912,	918,  928,	935,  940,	949,  954,	962,  969,	978,  982,	985,  990,	996,  1007, 1015, 1020, 1028, 1034, 1041, 1048, 1053, 1068, 1084, 1098, 1099, 1114, 1123, 1130, 1139, 1149, 1161, 1162, 1170, 1176, 1183, 1199, 1208, 1209, 1217, 1218, 1222, 1223, 1228, 1242, 1248, 1260, 1273, 1277, 1291, 1307, 1321, 1338, 1351, 1356, 1368, 1372, 1378, 1387, 1391, 1403, 1404, 1412, 1417, 1424, 1429, 1444, 1459, 1484, 1525,
										1535, 1553, 1558, 1567, 1568, 1569, 1570, 1575, 1579, 1588, 1596, 1604, 1611, 1618, 1622, 1626, 1633, 1637, 1641, 1645, 1649, 1653, 1657, 1661, 1665, 1669, 1673, 1677, 1681, 1688, 1697, 1710, 1727, 1734, 1754, 1775, 1776, 1777, 1778, 1779, 1780, 1781, 1785, 1791, 1799, 1808, 1819, 1825, 1834, 1840, 1849, 1850, 1858, 1859, 1867, 1877, 1886, 1892, 1901, 1916, 1917, 1921, 1930, 1940, 1945, 1953, 1958, 1963, 1972, 1973, 1977, 1978, 1979, 1980, 1993,
										2005, 2006, 2010, 2011, 2025, 2040, 2041, 2042, 2043, 2049, 2057, 2065, 2073, 2073, 2091, 2097, 2107, 2113, 2124, 2134, 2139, 2149, 2176, 2185, 2191, 2192, 2213, 2218, 2226, 2231, 2251, 2270, 2274, 2278, 2282, 2289, 2298, 2302, 2306, 2309, 2317, 2318, 2322, 2327, 2335, 2341, 2347, 2353, 2359, 2368, 2369, 2370, 2371, 2372, 2373, 2374, 2375, 2376, 2377, 2378, 2379, 2380, 2381, 2382, 2383, 2384, 2385, 2386, 2387, 2388, 2389, 2390, 2391, 2392, 2393,
										2397, 2398, 2399, 2400, 2401, 2402, 2403, 2404, 2405, 2409, 2410, 2411, 2412, 2413, 2414, 2415, 2416, 2417, 2418, 2419, 2420, 2421, 2425, 2426, 2430, 2431, 2432, 2433, 2434, 2435, 2436, 2437, 2438, 2439, 2440, 2444, 2454, 2455, 2456, 2457, 2458, 2459, 2460, 2461, 2462, 2463, 2464, 2465, 2466, 2467, 2468, 2469, 2470, 2471, 2472, 2473, 2474, 2475, 2476, 2477, 2478, 2482, 2483, 2484, 2485, 2486, 2487, 2488, 2489, 2490, 2491, 2492, 2493, 2494, 2495,
										2496, 2497, 2498, 2499, 2500, 2501, 2502, 2503, 2504, 2505, 2506, 2515, 2519, 2535, 2536, 2540, 2541, 2546, 2547, 2552, 2556, 2562, 2566, 2578, 2592, 2593, 2602, 2616, 2617, 2622, 2623, 2628, 2629, 2638, 2653, 2654, 2663, 2671, 2684, 2692, 2696, 2701, 2705, 2710, 2719, 2743, 2751, 2756, 2766, 2770, 2774, 2778, 2782, 2786, 2790, 2794, 2798, 2803, 2808, 2821, 2835, 2836, 2840, 2845, 2853, 2854, 2858, 2863, 2871, 2877, 2890, 2907, 2920, 2921, 2925,
										2926, 2931, 2932, 2936, 2941, 2946, 2960, 2969, 2985, 2993, 3008, 3017, 3033, 3042, 3064, 3074, 3078, 3082, 3087, 3095, 3104, 3109, 3114, 3127, 3137, 3146, 3150, 3156, 3162, 3168, 3177, 3184, 3200, 3207, 3208, 3212, 3216, 3217, 3221, 3222, 3226, 3233, 3234, 3239, 3240, 3244, 3257, 3266, 3279, 3283, 3293, 3303, 3317, 3321, 3330, 3342, 3355, 3366, 3378, 3389, 3403, 3413, 3439, 3461, 3482, 3483, 3487, 3488, 3492, 3496, 3500, 3504};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char* const yytname[] = {"$end",
									  "error",
									  "$undefined",
									  "BSTRING_SYM",
									  "HSTRING_SYM",
									  "CSTRING_SYM",
									  "UCASEFIRST_IDENT_SYM",
									  "LCASEFIRST_IDENT_SYM",
									  "NAMEDMACRO_SYM",
									  "MACRODEFBODY_SYM",
									  "BRACEBAL_SYM",
									  "NUMBER_ERANGE",
									  "NUMBER_SYM",
									  "SNACC_ATTRIBUTES",
									  "COLON_SYM",
									  "ELLIPSIS_SYM",
									  "DOT_SYM",
									  "COMMA_SYM",
									  "LEFTBRACE_SYM",
									  "RIGHTBRACE_SYM",
									  "LEFTPAREN_SYM",
									  "RIGHTPAREN_SYM",
									  "LEFTBRACKET_SYM",
									  "RIGHTBRACKET_SYM",
									  "LESSTHAN_SYM",
									  "MINUS_SYM",
									  "GETS_SYM",
									  "BAR_SYM",
									  "TAGS_SYM",
									  "BOOLEAN_SYM",
									  "INTEGER_SYM",
									  "BIT_SYM",
									  "STRING_SYM",
									  "OCTET_SYM",
									  "CONTAINING_SYM",
									  "ENCODED_SYM",
									  "NULL_SYM",
									  "SEQUENCE_SYM",
									  "OF_SYM",
									  "SET_SYM",
									  "IMPLICIT_SYM",
									  "CHOICE_SYM",
									  "ANY_SYM",
									  "OBJECT_IDENTIFIER_SYM",
									  "RELATIVE_OID_SYM",
									  "OPTIONAL_SYM",
									  "DEFAULT_SYM",
									  "COMPONENTS_SYM",
									  "UNIVERSAL_SYM",
									  "APPLICATION_SYM",
									  "PRIVATE_SYM",
									  "TRUE_SYM",
									  "FALSE_SYM",
									  "BEGIN_SYM",
									  "END_SYM",
									  "DEFINITIONS_SYM",
									  "EXPLICIT_SYM",
									  "ENUMERATED_SYM",
									  "EXPORTS_SYM",
									  "IMPORTS_SYM",
									  "REAL_SYM",
									  "INCLUDES_SYM",
									  "MIN_SYM",
									  "MAX_SYM",
									  "SIZE_SYM",
									  "FROM_SYM",
									  "WITH_SYM",
									  "COMPONENT_SYM",
									  "PRESENT_SYM",
									  "ABSENT_SYM",
									  "DEFINED_SYM",
									  "BY_SYM",
									  "PLUS_INFINITY_SYM",
									  "MINUS_INFINITY_SYM",
									  "SEMI_COLON_SYM",
									  "IA5STRING_SYM",
									  "PRINTABLESTRING_SYM",
									  "NUMERICSTRING_SYM",
									  "TELETEXSTRING_SYM",
									  "T61STRING_SYM",
									  "VIDEOTEXSTRING_SYM",
									  "VISIBLESTRING_SYM",
									  "ISO646STRING_SYM",
									  "GRAPHICSTRING_SYM",
									  "GENERALSTRING_SYM",
									  "BMPSTRING_SYM",
									  "UNIVERSALSTRING_SYM",
									  "UTF8STRING_SYM",
									  "GENERALIZEDTIME_SYM",
									  "UTCTIME_SYM",
									  "EXTERNAL_SYM",
									  "OBJECTDESCRIPTOR_SYM",
									  "OPERATION_SYM",
									  "ARGUMENT_SYM",
									  "RESULT_SYM",
									  "ERRORS_SYM",
									  "LINKED_SYM",
									  "ERROR_SYM",
									  "PARAMETER_SYM",
									  "BIND_SYM",
									  "BINDERROR_SYM",
									  "UNBIND_SYM",
									  "UNBINDERROR_SYM",
									  "ASE_SYM",
									  "OPERATIONS_SYM",
									  "CONSUMERINVOKES_SYM",
									  "SUPPLIERINVOKES_SYM",
									  "AC_SYM",
									  "ASES_SYM",
									  "REMOTE_SYM",
									  "INITIATOR_SYM",
									  "RESPONDER_SYM",
									  "ABSTRACTSYNTAXES_SYM",
									  "CONSUMER_SYM",
									  "EXTENSIONS_SYM",
									  "CHOSEN_SYM",
									  "EXTENSION_SYM",
									  "CRITICAL_SYM",
									  "FOR_SYM",
									  "DELIVERY_SYM",
									  "SUBMISSION_SYM",
									  "TRANSFER_SYM",
									  "EXTENSIONATTRIBUTE_SYM",
									  "TOKEN_SYM",
									  "TOKENDATA_SYM",
									  "SECURITYCATEGORY_SYM",
									  "OBJECT_SYM",
									  "PORTS_SYM",
									  "BOXC_SYM",
									  "BOXS_SYM",
									  "PORT_SYM",
									  "ABSTRACTOPS_SYM",
									  "REFINE_SYM",
									  "AS_SYM",
									  "RECURRING_SYM",
									  "VISIBLE_SYM",
									  "PAIRED_SYM",
									  "ABSTRACTBIND_SYM",
									  "ABSTRACTUNBIND_SYM",
									  "TO_SYM",
									  "ABSTRACTERROR_SYM",
									  "ABSTRACTOPERATION_SYM",
									  "ALGORITHM_SYM",
									  "ENCRYPTED_SYM",
									  "SIGNED_SYM",
									  "SIGNATURE_SYM",
									  "PROTECTED_SYM",
									  "OBJECTTYPE_SYM",
									  "SYNTAX_SYM",
									  "ACCESS_SYM",
									  "STATUS_SYM",
									  "DESCRIPTION_SYM",
									  "REFERENCE_SYM",
									  "INDEX_SYM",
									  "DEFVAL_SYM",
									  "ATTRIB_ASN1_TYPE_ID",
									  "ATTRIB_C_TYPE_ID",
									  "ATTRIB_C_TYPE_NAME_SYM",
									  "ATTRIB_C_FIELD_NAME_SYM",
									  "ATTRIB_IS_PDU_SYM",
									  "ATTRIB_IS_PTR_SYM",
									  "ATTRIB_IS_PTR_TYPEDEF_SYM",
									  "ATTRIB_IS_PTR_TYPE_REF_SYM",
									  "ATTRIB_IS_PTR_IN_CHOICE_SYM",
									  "ATTRIB_IS_PTR_FOR_OPT_SYM",
									  "ATTRIB_OPT_TEST_ROUTINE_SYM",
									  "ATTRIB_DEFAULT_FIELD_SYM",
									  "ATTRIB_PRINT_ROUTINE_SYM",
									  "ATTRIB_ENCODE_ROUTINE_SYM",
									  "ATTRIB_DECODE_ROUTINE_SYM",
									  "ATTRIB_FREE_ROUTINE_SYM",
									  "ATTRIB_IS_ENC_DEC_SYM",
									  "ATTRIB_GEN_TYPEDEF_SYM",
									  "ATTRIB_GEN_PRINT_ROUTINE_SYM",
									  "ATTRIB_GEN_ENCODE_ROUTINE_SYM",
									  "ATTRIB_GEN_DECODE_ROUTINE_SYM",
									  "ATTRIB_GEN_FREE_ROUTINE_SYM",
									  "ATTRIB_CHOICE_ID_SYMBOL_SYM",
									  "ATTRIB_CHOICE_ID_VALUE_SYM",
									  "ATTRIB_CHOICE_ID_ENUM_NAME_SYM",
									  "ATTRIB_CHOICE_ID_ENUM_FIELD_NAME_SYM",
									  "ATTRIB_IS_BIG_INT_SYM",
									  "ATTRIB_NAMESPACE_SYM",
									  "C_CHOICE_SYM",
									  "C_LIST_SYM",
									  "C_ANY_SYM",
									  "C_ANYDEFBY_SYM",
									  "C_LIB_SYM",
									  "C_STRUCT_SYM",
									  "C_TYPEDEF_SYM",
									  "C_TYPEREF_SYM",
									  "C_NO_TYPE_SYM",
									  "UNKNOWN_SYM",
									  "BITSTRING_SYM",
									  "OCTETSTRING_SYM",
									  "SEQUENCE_OF_SYM",
									  "SET_OF_SYM",
									  "ANY_DEFINED_BY_SYM",
									  "LOCAL_TYPE_REF_SYM",
									  "IMPORT_TYPE_REF_SYM",
									  "':'",
									  "$accept",
									  "LineNo",
									  "ModuleDefinition",
									  "$@1",
									  "TagDefault",
									  "ExtensionDefault",
									  "ModuleIdentifier",
									  "AssignedIdentifier",
									  "OptionalSnaccNamespace",
									  "ModuleBody",
									  "Exports",
									  "SymbolsExported",
									  "ExportSymbolList",
									  "Imports",
									  "SymbolsImported",
									  "SymbolsFromModuleList",
									  "SymbolsFromModule",
									  "SymbolList",
									  "Symbol",
									  "AssignmentList",
									  "AssignmentOrError",
									  "Assignment",
									  "$@2",
									  "MacroReference",
									  "TypeAssignment",
									  "ExternalTypeReference",
									  "DefinedType",
									  "Type",
									  "BuiltinType",
									  "NamedType",
									  "BooleanType",
									  "IntegerType",
									  "NamedNumberList",
									  "NamedNumber",
									  "SignedNumber",
									  "EnumeratedType",
									  "RealType",
									  "BitStringType",
									  "NamedBitList",
									  "NullType",
									  "ExternalType",
									  "SequenceOpening",
									  "SequenceType",
									  "ElementTypes",
									  "ElementTypeList",
									  "Extension",
									  "ExtensionAdditions",
									  "ExtensionAdditionList",
									  "ExtensionAddition",
									  "ExtList",
									  "VersionNumber",
									  "OptExtMarker",
									  "ElementType",
									  "SequenceOfType",
									  "SetOpening",
									  "SetType",
									  "SetOfType",
									  "ChoiceType",
									  "AlternativeTypes",
									  "AlternativeTypeList",
									  "ChoiceExtension",
									  "ExtAdditionAlternatives",
									  "ExtAddList",
									  "ExtAddAlternative",
									  "AltList",
									  "SelectionType",
									  "TaggedType",
									  "Tag",
									  "ClassNumber",
									  "Class",
									  "AnyType",
									  "OctetContainingType",
									  "BitContainingType",
									  "ObjectIdentifierType",
									  "RelativeOIDType",
									  "UsefulType",
									  "CharStrType",
									  "Subtype",
									  "SubtypeSpec",
									  "SubtypeValueSetList",
									  "SubtypeValueSet",
									  "ExtensibleSubtype",
									  "SingleValue",
									  "ContainedSubtype",
									  "ValueRange",
									  "LowerEndPoint",
									  "UpperEndPoint",
									  "LowerEndValue",
									  "UpperEndValue",
									  "SizeConstraint",
									  "PermittedAlphabet",
									  "InnerTypeConstraints",
									  "SingleTypeConstraint",
									  "MultipleTypeConstraints",
									  "FullSpecification",
									  "PartialSpecification",
									  "TypeConstraints",
									  "NamedConstraint",
									  "Constraint",
									  "ValueConstraint",
									  "PresenceConstraint",
									  "ValueAssignment",
									  "Value",
									  "DefinedValue",
									  "ExternalValueReference",
									  "BuiltinValue",
									  "$@3",
									  "BooleanValue",
									  "SpecialRealValue",
									  "NullValue",
									  "NamedValue",
									  "ObjectIdentifierValue",
									  "ObjIdComponentList",
									  "ObjIdComponent",
									  "NumberForm",
									  "NameForm",
									  "NameAndNumberForm",
									  "BinaryString",
									  "HexString",
									  "CharString",
									  "number",
									  "identifier",
									  "modulereference",
									  "typereference",
									  "empty",
									  "SnaccDirectives",
									  "SnaccDirectiveList",
									  "SnaccDirective",
									  "SnaccDirectiveAsnTypeValue",
									  "SnaccDirectiveCTypeValue",
									  "SnaccDirectiveBoolType",
									  "SnaccDirectiveBoolValue",
									  "SnaccDirectiveStringType",
									  "SnaccDirectiveIntegerType",
									  "DefinedMacroType",
									  "DefinedMacroName",
									  "RosOperationMacroType",
									  "RosOperationMacroBody",
									  "RosOpArgument",
									  "RosOpResult",
									  "RosOpResultType",
									  "RosOpErrors",
									  "RosOpLinkedOps",
									  "RosErrorMacroType",
									  "RosErrParameter",
									  "RosBindMacroType",
									  "RosBindArgument",
									  "RosBindResult",
									  "RosBindError",
									  "RosUnbindMacroType",
									  "RosUnbindError",
									  "RosAseMacroType",
									  "RosAseSymmetricAse",
									  "RosAseConsumerInvokes",
									  "RosAseSupplierInvokes",
									  "RosAseOperationList",
									  "RosAcMacroType",
									  "RosAcNonRoElements",
									  "RosAcRoElements",
									  "RosAcSymmetricAses",
									  "RosAcAsymmetricAses",
									  "RosAcInitiatorConsumerOf",
									  "RosAcResponderConsumerOf",
									  "RosAcAbstractSyntaxes",
									  "OidList",
									  "MtsasExtensionsMacroType",
									  "PossiblyEmptyValueList",
									  "ValueList",
									  "PossiblyEmptyTypeOrValueList",
									  "TypeOrValueList",
									  "TypeOrValue",
									  "MtsasExtensionMacroType",
									  "MtsasExtDefaultVal",
									  "MtsasExtCritical",
									  "MtsasExtCriticalityList",
									  "MtsasExtCriticality",
									  "MtsasExtensionAttributeMacroType",
									  "MtsasTokenMacroType",
									  "MtsasTokenDataMacroType",
									  "MtsasSecurityCategoryMacroType",
									  "AsnObjectMacroType",
									  "AsnPorts",
									  "AsnPortList",
									  "AsnPort",
									  "AsnPortType",
									  "AsnPortMacroType",
									  "AsnOperations",
									  "AsnConsumer",
									  "AsnSupplier",
									  "AsnRefineMacroType",
									  "AsnComponentList",
									  "AsnComponent",
									  "AsnObjectSpec",
									  "AsnPortSpecList",
									  "AsnPortSpec",
									  "AsnPortStatus",
									  "AsnObjectList",
									  "AsnObject",
									  "AsnAbstractBindMacroType",
									  "AsnAbstractBindPorts",
									  "AsnAbstractUnbindMacroType",
									  "AsnAbstractUnbindPorts",
									  "AsnAbstractOperationMacroType",
									  "AsnAbstractErrorMacroType",
									  "AfAlgorithmMacroType",
									  "AfEncryptedMacroType",
									  "AfSignedMacroType",
									  "AfSignatureMacroType",
									  "AfProtectedMacroType",
									  "SnmpObjectTypeMacroType",
									  "SnmpAccess",
									  "SnmpStatus",
									  "SnmpDescrPart",
									  "SnmpReferPart",
									  "SnmpIndexPart",
									  "SnmpDefValPart",
									  YY_NULL};
#endif

#ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] = {0,	  256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321,
										 322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388,
										 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423, 424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436, 437, 438, 439, 440, 441, 442, 443, 444, 445, 446, 447, 448, 449, 450, 451, 452, 453, 454, 58};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] = {0,	  201, 202, 204, 203, 205, 205, 205, 206, 206, 207, 208, 208, 209, 209, 210, 210, 211, 211, 211, 212, 212, 213, 213, 214, 214, 214, 215, 215, 216, 216, 217, 218, 218, 219, 219, 219, 220, 220, 221, 221, 221, 222, 222, 223, 222, 222, 222, 224, 224, 225, 226, 227, 227, 228, 228, 228, 228, 229, 229, 229, 229, 229, 229, 229, 229, 229, 229, 229, 229, 229, 229, 229, 229, 229, 229, 229, 229, 229, 229, 230, 230, 231, 232, 232, 233, 233,
									 234, 234, 235, 235, 235, 235, 236, 237, 238, 238, 239, 240, 241, 242, 243, 243, 244, 245, 245, 245, 246, 247, 247, 247, 248, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253, 253, 253, 253, 253, 254, 255, 256, 256, 257, 258, 259, 260, 260, 260, 261, 262, 262, 262, 263, 263, 264, 264, 265, 265, 266, 267, 267, 267, 268, 269, 269, 270, 270, 270, 270, 271, 271, 272, 273, 274, 275, 276, 276, 276, 277, 277, 277, 277, 277, 277, 277, 277,
									 277, 277, 277, 277, 277, 278, 278, 278, 279, 280, 280, 281, 281, 281, 281, 281, 281, 281, 282, 283, 284, 285, 286, 286, 287, 287, 288, 288, 289, 289, 290, 291, 292, 292, 293, 294, 294, 295, 296, 297, 297, 298, 298, 299, 300, 300, 301, 301, 301, 301, 302, 303, 303, 304, 304, 305, 306, 306, 306, 306, 306, 306, 306, 307, 306, 308, 308, 309, 309, 310, 311, 311, 312, 313, 313, 314, 314, 314, 315, 316, 317, 317, 318, 319, 320, 321, 321,
									 322, 323, 324, 325, 326, 326, 327, 327, 328, 328, 328, 328, 328, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 330, 330, 330, 330, 330, 330, 330, 330, 330, 331, 331, 331, 331, 331, 331, 331, 331, 331, 331, 331, 331, 331, 332, 332, 333, 333, 333, 333, 333, 333, 333, 333, 333, 333, 333, 334, 335, 335, 335, 335, 335, 335, 335, 335, 335, 335, 335, 335,
									 335, 335, 335, 335, 335, 335, 335, 335, 335, 335, 335, 335, 335, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 336, 337, 338, 339, 339, 340, 340, 341, 341, 342, 342, 343, 343, 344, 345, 345, 346, 347, 347, 348, 348, 349, 349, 350, 351, 351, 352, 352, 353, 354, 354, 355, 355, 356, 357, 358, 359, 359, 360, 360, 361, 362, 362, 363, 363, 364, 364, 365, 365, 366,
									 367, 367, 368, 368, 369, 369, 370, 370, 371, 371, 372, 372, 373, 373, 374, 374, 375, 375, 376, 376, 376, 377, 377, 378, 378, 379, 379, 380, 380, 381, 382, 382, 383, 383, 384, 385, 385, 385, 386, 386, 387, 387, 387, 387, 387, 388, 389, 390, 391, 391, 392, 393, 393, 394, 394, 395, 396, 396, 397, 397, 398, 399, 399, 400, 400, 401, 401, 402, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413, 413, 414, 414, 415, 415, 416, 416};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] = {0, 2, 0, 0, 10, 2, 2, 1, 1, 1, 2, 1, 1, 3, 1, 3, 1, 3, 3, 1, 1, 1, 1, 4, 3, 3, 1, 1, 1, 2, 1, 4, 3, 1, 1, 1, 1, 2, 1, 1, 2, 2, 1, 1, 0, 6, 3, 5, 1, 1, 6, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 4, 1, 3, 4, 4, 1, 1, 2, 2, 4, 1, 2, 5, 1, 1, 1, 3, 3, 2, 2, 1, 5, 1, 2, 2, 3, 1, 1, 3, 1, 6, 5, 1, 2, 1,  1, 1, 1, 2, 3, 3, 4, 3, 3, 3, 2, 3,
									5, 2, 1, 4, 1,	2, 2, 3, 1, 1, 4, 1, 6, 1, 3, 3, 2, 3, 3, 4, 1, 1, 1, 1, 1, 1, 1, 4, 6, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 4, 3, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 1, 2, 1, 2, 1, 1, 1, 1, 2, 2, 3, 3, 1, 1, 1, 3, 5, 1, 3, 2, 1, 2, 1, 1, 1, 1, 1, 1, 5, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 0, 3, 1, 1, 1, 1, 1, 1, 2, 3, 2, 1, 1,  1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1,
									1, 1, 0, 1, 1,	1, 2, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 2, 4, 2, 1, 2, 1, 1,
									1, 4, 1, 4, 1,	2, 2, 1, 4, 2, 1, 2, 1, 2, 1, 4, 2, 1, 2, 3, 4, 4, 1, 4, 1, 1, 8, 4, 7, 1, 5, 1, 2, 6, 1, 6, 1, 4, 1, 1, 3, 6, 1, 1, 1, 3, 1, 1, 1, 3, 1, 1, 4, 1, 2, 1, 3, 1, 1, 3, 1, 1, 1, 1, 2, 1, 2, 1, 2, 1, 2, 2, 4, 1, 1, 3, 2, 1, 1, 1, 2, 1, 4, 1, 1, 2, 2, 4, 4, 4, 1, 3, 2, 1, 2, 1, 3, 3, 1, 3, 1, 3, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 2, 3, 2, 2, 2, 2, 11, 1, 1, 2, 0, 2, 0, 4, 0, 4, 0};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] = {0,	  262, 0,	0,	 264, 1,   264, 0,	 10,  11,  12,	0,	 0,	  3,   7,	261, 260, 259, 0,	248, 249, 250, 251, 252, 253, 6,   5,	264, 246, 247, 0,	8,	 0,	  9,   0,	227, 0,	  228, 0,	0,	 255, 254, 2,	264, 0,	  0,   264, 14,	 229, 0,   0,	0,	 264, 19,  13,	0,	 263, 361, 362, 363, 364, 365, 366, 368, 367, 369, 370, 371, 372, 373, 374, 375, 376, 377, 379, 378, 380, 381, 382, 383, 384, 385, 0,	20,
										 22,  35,  34,	21,	 36,  4,   0,	0,	 26,  18,  17,	2,	 0,	  0,   27,	30,	 0,	  33,  28,	0,	 0,	  0,   38,	39,	 42,  43,  0,	0,	 0,	  25,  24,	29,	 0,	  2,   41,	0,	 37,  40,  263, 264, 82,  83,  0,	0,	 98,  2,   2,	2,	 157, 161, 162, 0,	 94,  168, 167, 166, 172, 173, 174, 175, 176, 177, 178, 169, 170, 171, 163, 164, 99,  165, 264, 264, 264, 264, 264, 0,	 0,	  446, 456, 458, 460, 462, 264, 474,
										 0,	  264, 264, 264, 264, 0,   0,	0,	 0,	  0,   0,	52,	 56,  0,   55,	58,	 59,  73,  74,	61,	 62,  63,  0,	64,	 65,  0,   66,	67,	 68,  69,  70,	0,	 71,  75,  60,	72,	 79,  78,  77,	57,	 0,	  0,   53,	54,	 336, 337, 338, 339, 340, 341, 343, 342, 344, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 359, 358, 357, 360, 264, 23,	32,	 0,	  2,   46,	0,	 48,  49,  153, 154, 155, 0,   156, 0,
										 95,  76,  0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  389, 386, 264, 0,	  400, 398, 0,	 403, 264, 264, 0,	 0,	  415, 411, 264, 0,	  0,   0,	81,	 264, 0,   457, 459, 461, 463, 0,	466, 464, 0,   0,	0,	 473, 476, 477, 256, 257, 258, 90,	89,	 237, 0,   243, 239, 240, 241, 242, 233, 495, 226, 225, 230, 232, 231, 235, 234, 236, 0,   0,	499, 496, 0,   503, 500, 505, 504, 0,	507, 508, 509, 510, 0,
										 0,	  2,   179, 264, 102, 0,   121, 0,	 264, 104, 106, 0,	 129, 0,   0,	0,	 147, 0,   2,	0,	 0,	  324, 325, 309, 310, 311, 312, 313, 314, 326, 327, 328, 329, 330, 331, 315, 316, 317, 318, 319, 320, 332, 335, 333, 334, 321, 266, 2,	 265, 267, 0,	0,	 0,	  31,  44,	0,	 0,	  152, 151, 0,	 85,  0,   0,	0,	 0,	  126, 204, 100, 0,	  130, 127, 0,	 0,	  0,   0,	388, 264, 391, 264, 399, 402, 0,   405, 264,
										 264, 0,   0,	0,	 417, 412, 0,	0,	 0,	  0,   448, 264, 80,  0,   0,	0,	 0,	  478, 479, 0,	 92,  91,  0,	0,	 497, 0,   501, 506, 0,	  192, 0,	201, 0,	  0,   0,	183, 191, 185, 186, 187, 0,	  196, 189, 188, 190, 193, 0,	0,	 107, 110, 0,	122, 0,	  101, 103, 264, 0,	  128, 148, 149, 146, 0,   0,	0,	 0,	  268, 0,	0,	 0,	  0,   47,	150, 0,	  84,  0,	97,	 0,	  160, 0,	181, 180, 264, 133, 0,
										 264, 135, 158, 93,	 392, 393, 390, 0,	 395, 264, 404, 0,	 407, 401, 0,	410, 408, 437, 0,	418, 0,	  0,   0,	0,	 264, 447, 0,	450, 445, 264, 0,	467, 98,  443, 444, 228, 0,	  0,   441, 0,	 0,	  238, 482, 483, 0,	  486, 0,	0,	 0,	  194, 205, 0,	 0,	  182, 0,	0,	 197, 224, 108, 264, 264, 111, 113, 124, 244, 123, 228, 2,	 0,	  51,  275, 276, 279, 283, 285, 287, 288, 280, 299, 282, 281, 294, 293, 292,
										 298, 295, 296, 297, 274, 277, 278, 284, 286, 289, 290, 291, 269, 300, 301, 302, 303, 304, 305, 306, 307, 308, 270, 264, 322, 323, 271, 272, 273, 45,  86,	0,	 0,	  96,  0,	0,	 136, 139, 264, 131, 132, 264, 0,	397, 387, 406, 409, 413, 0,	  414, 0,	420, 0,	  436, 0,	435, 0,	  470, 471, 472, 469, 0,   465, 2,	 0,	  480, 481, 475, 0,	  264, 485, 488, 487, 498, 502, 512, 0,	  264, 207, 209, 210, 208, 206, 184,
										 0,	  264, 119, 0,	 109, 120, 245, 0,	 125, 50,  87,	88,	 159, 137, 264, 142, 264, 140, 0,	440, 0,	  439, 264, 438, 416, 264, 434, 455, 453, 454, 449, 451, 468, 0,   442, 484, 0,	  0,   0,	0,	 218, 0,   213, 216, 264, 264, 219, 0,	 203, 195, 198, 202, 0,	  0,   118, 112, 105, 264, 264, 138, 134, 394, 0,	0,	 422, 264, 0,	491, 0,	  490, 489, 513, 515, 264, 264, 211, 223, 220, 221, 217, 222, 215, 199, 0,
										 116, 117, 0,	0,	 396, 0,   0,	431, 419, 452, 0,	0,	 517, 0,   214, 264, 0,	  144, 0,	141, 0,	  0,   492, 493, 514, 0,   519, 212, 2,	  114, 0,	0,	 0,	  432, 0,	0,	 516, 0,   521, 0,	 145, 143, 264, 0,	 430, 494, 0,	0,	 511, 115, 0,	424, 264, 433, 0,	0,	 0,	  0,   427, 421, 264, 518, 0,	0,	 0,	  0,   429, 425, 520, 0,   0,	0,	 423, 0,   0,	0,	 0,	  426, 0,	428};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] = {-1,  44,  2,	27,	 13,  32,  3,	8,	 46,  51,  52,	82,	 83,  91,  97,	98,	 99,  100, 101, 105, 106, 107, 489, 242, 108, 179, 180, 283, 182, 342, 183, 184, 395, 396, 311, 185, 186, 187, 496, 188, 189, 190, 191, 343, 344, 345, 468, 564, 565, 755, 724, 676, 346, 192, 193, 194, 195, 196, 503, 504, 505, 624, 688, 689, 774, 197, 198, 199, 392, 249, 200, 201,
										 202, 203, 204, 205, 206, 207, 338, 454, 455, 456, 457, 458, 459, 460, 721, 461, 722, 462, 463, 464, 670, 666, 667, 668, 713, 714, 715, 716, 751, 109, 521, 313, 35,  314, 439, 315, 316, 317, 569, 9,	 18,  19,  20,	21,	 22,  318, 319, 320, 23,  37,  209, 210, 382, 383, 384, 385, 600, 610, 386, 614, 387, 388, 211, 88,	 212, 266, 267, 414, 510, 513, 632, 213,
										 270, 214, 273, 419, 517, 215, 520, 216, 278, 279, 425, 522, 217, 281, 737, 808, 815, 816, 823, 764, 790, 218, 642, 523, 692, 693, 542, 219, 431, 532, 702, 703, 220, 221, 222, 223, 224, 292, 534, 535, 648, 225, 296, 297, 298, 226, 546, 547, 548, 658, 659, 741, 778, 549, 227, 324, 228, 327, 229, 230, 231, 232, 233, 234, 235, 236, 664, 744, 768, 782, 794, 804};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -752
static const yytype_int16 yypact[] = {57,	-752, 66,	22,	  82,	-752, 27,	135,  -752, -752, -752, 85,	  88,	-752, -752, -752, -752, -752, 103,	-752, -752, -752, -752, -752, 146,	-752, -752, 155,  -752, -752, 127,	-752, 94,	-752, 158,	-752, 165,	-752, 108,	139,  -752, -752, -752, 56,	  218,	41,	  189,	-752, -752, 240,  1285, 196,  202,	206,  -752, 195,  -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752,
									  -752, -752, -752, -752, -752, -752, 197,	253,  -752, -752, -752, -752, -752, -752, 1342, 87,	  -752, -752, -752, -752, 199,	208,  1052, -752, 4,	-752, -752, 214,  260,	26,	  -752, 215,  -752, -752, 3101, 261,  1052, -752, -752, -752, 1052, -752, -752, 3284, -752, -752, 259,	48,	  -752, 272,  247,	263,  -752, -18,  -15,	-752, 223,	-752, -752, 278,  -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752,
									  -752, -752, 207,	200,  211,	211,  40,	198,  186,	3101, 3101, 3101, 3101, 3101, 178,	-53,  191,	168,  243,	200,  207,	219,  3101, 3101, 3101, 3101, 162,	-752, -752, 97,	  -752, -752, -752, -752, -752, -752, -752, -752, 1629, -752, -752, 1771, -752, -752, -752, -752, -752, 1898, -752, -752, -752, -752, -752, -752, -752, -752, 285,	295,  -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752,
									  -752, -752, -752, -752, -752, -752, -752, -752, -752, 3333, -752, -752, 57,	-752, -752, 296,  -752, -752, -752, -752, -752, 127,  -752, 218,  20,	293,  3101, 294,  297,	280,  3101, 298,  281,	302,  250,	218,  3101, -752, -752, 228,  3101, -752, -752, 3101, -752, 229,  229,	306,  309,	-752, -752, 222,  311,	226,  265,	294,  286,	2436, 294,	294,  294,	294,  315,	-752, -752, 317,  325,	326,  -752, 239,  244,	-752, -752, -752, -752, -752,
									  -752, 75,	  -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, 225,  330,	-752, 3101, 332,  -752, 3101, -752, -752, 3101, 294,  294,	294,  294,	3101, 532,	-752, -752, 338,  -752, 314,  105,	337,  3333, -752, 347,	2025, -752, 346,  3101, 3101, 294,	3101, -752, 166,  167,	-752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752,
									  -752, -752, -752, -752, 3333, -752, 169,	170,  172,	-752, -752, 3340, 345,	-752, -752, 112,  -752, 353,  218,	3101, 340,	294,  -752, -752, 3101, 294,  -752, 3101, 2578, 218,  148,	-752, 3101, -752, 283,	-752, -752, 3101, -752, 275,  274,	191,  191,	363,  -752, -752, 191,	3101, 365,	191,  -752, 267,  294,	191,  1487, 1487, 1487, -752, -752, 375,  -752, -752, 191,	191,  294,	191,  294,	294,  -12,	-752, 3101, -752, 294,	15,	  109,	-752,
									  -752, -752, -752, -752, 372,	366,  -752, -752, -752, 19,	  191,	2167, -752, -752, 3101, -752, 191,	-752, -752, 3333, 351,	-752, 294,	294,  294,	385,  1085, -30,  3101, -752, 131,	387,  381,	386,  -752, -752, 218,	-752, 100,	377,  380,	294,  3101, 294,  294,	379,  384,	383,  3333, -752, -752, -752, -752, -752, -752, 388,  -752, 308,  -752, 3101, -752, -752, 3101, -752, -752, -752, 390,	397,  396,	191,  152,	-2,	  191,	-752, 287,	-752,
									  -752, 72,	  161,	-752, -752, 294,  -752, 285,  400,	174,  -752, 187,  188,	-752, 401,	-752, 191,	289,  194,	201,  218,	294,  -752, 399,  294,	-752, 532,	403,  -752, -752, -752, 7,	  193,	-752, -752, 294,  -752, -752, 191,	-752, 3101, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752,
									  -752, -752, -752, 726,  -752, -752, -752, -752, -752, -752, -752, 407,  410,	-752, 232,	2720, -752, -752, 3333, -752, -752, 1487, 402,	-752, -752, -752, -752, -752, 191,	-752, 405,	-752, 3101, -752, 420,	397,  43,	-752, -752, -752, -752, 191,  -752, -752, 1487, -752, -752, -752, 191,	72,	  423,	-752, -752, -752, -752, -752, 300,	24,	  -752, -752, -752, -752, -752, -752, 335,	203,  -752, 2847, -752, -752, -752, 2309, 294,	-752, -752, -752,
									  -752, -752, 25,	-752, 204,	-752, 2578, -752, 427,	430,  1487, -752, -752, -4,	  -752, -752, -752, -752, 431,	-752, -752, 249,  -752, -752, 123,	191,  218,	434,  -752, 205,  -752, -752, -9,	294,  -752, 449,  -752, -752, -752, -752, 2974, 443,  -752, -752, -752, 203,  3333, -752, -752, -752, 439,	355,  -752, 350,  43,	-752, 404,	-752, -752, -752, 312,	64,	  64,	-752, -752, -752, -752, -752, -752, -752, -752, 104,  -752, -752, 3101, 3228,
									  -752, 447,  451,	-752, -752, -752, 191,	191,  319,	209,  -752, 3333, 450,	-752, 126,	-752, 191,	82,	  459,	-752, -752, 191,  324,	-752, -752, -752, 3101, 455,  460,	-752, 212,	191,  -752, 462,  327,	2974, -752, -752, 382,	82,	  -752, -752, 1487, 466,  -752, -752, 452,	-752, 378,	-752, 213,	191,  471,	389,  -752, -752, 392,	-752, 472,	191,  457,	391,  -752, -752, -752, 216,  478,	467,  -752, 191,  479,	217,  191,	-752, 220,	-752};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] = {-752, -89,  -752, -752, -752, -752, 258,	 -752, -752, -752, -752, -752, -752, -752, -752, -752, 408,	 -752, -31,	 -752, 406,	 -752, -752, 116,  -752, -752, -752, -73,  -752, -136, -752, -752, -258, 16,   21,	 -752, -752, -752, -752, -752, -752, -752, -752, 316,  -169, -752, -752, -752, -162, -752, -215, -172, -458, -752, -752, -752, -752, -752, -752, -173, -752, -752, -752, -241, -752, -752, -752, -752, -752, -752, -752, -752,
									   -752, -752, -752, -752, -752, -752, -253, -752, -38,	 -752, -752, -752, -752, -752, -752, -752, -196, 138,  -752, -752, -752, -752, -752, -752, -221, -220, -190, -752, -752, -752, 307,	 -26,  -752, -752, -752, -752, -752, -752, -752, -751, -752, 511,  500,	 -752, -752, -752, -752, -752, -246, 595,  0,	 -40,  18,	 -327, -752, 149,  -752, -752, -752, -752, -752, -752, -752, -108, -752, 360,  -752, -752, -752, -752, -752, -752,
									   369,	 -752, 395,	 268,  -752, -752, -752, -752, -752, -752, -752, -407, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -419, -160, -422, -111, -752, -752, -752, -752, -193, -752, -752, -752, -752, -752, -752, -199, -100, -104, -752, -752, 257,  262,	 -752, -752, -96,  -752, -752, -148, -752, -752, -167, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752, -752};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -265
static const yytype_int16 yytable[] = {
	4,	 321, 402, 394, 34,	 410, 112, 526, 336, 566, 86,  245, 541, 543, 544, 524, 336, 474, 336, 84,	254, 116, 10,  258, 14,	 284, 789, 103, 240, 673, 38,  15,	56,	 15,  104, -200, 748, 181, 398, 711, 256, 259, 261, -200, 336,	33,	 255, 729, 809, 255, 86,  111, 293, 294, 399, 246, 247, 248, 86,  749, 750, 47,	 555, 1,   53,	111, 5,	  11,  87,	117, 92,  15,  86,	246, 247, 248, 86,	6,	 295, 244,	-15, 238, 556, 12,	336, 239,  440, 441, 103, 286, 287, 288, 289, 56,  15,	104, 246, 247,
	248, 640, 7,   331, 332, 333, 334, 735, 1,	 15,  102, 643, 15,	 302, 303, 25,	16,	 17,  26,  336, 638, 243, 39,  771, 28,	 337, 42,  305, 352, 772, 411, 492, 557, 493, 415, 1,	 15,  416, 558, 552, 16,  17,  495, 250,  15,	786, 275, 276, 16,	17,	 571, 787, 471, 472, 390, 601, 602, 603, 604, 605, 606, 607, 608, 609, 699, 700, 701, 492, 30,	507, 38,  636, 31,	639, 265, 269, 272, 272, 277, 628,	649, 40,  650, 401, 612, 613,  291, 405, 41,  323, 326, 269, 265, 652, 43,	653, 299, 300,
	301, 1,	  15,  554, 645, 646, 302, 303, 652, 652, 654, 655, 674, 304, 675, 649, 432, 661, 16,  17,	305, 566, 649, 674, 662, 730, 746, 393, 747, 15,  746, 306, 783, 799, 652, 800,	 817, 636, 636, 828, 833, 636, 45,	835,  4,	49,	 307, 308, 550, 54,	 551, 50,  466, 38,	 89,  444, 336, 684, 446, 56,  15,	447, 739, 740, -16, 90,	 448, 309, 310, 481, 756, 257, 260, 93,	 95,  94,  502, 113, 432, -262, 508, 478, 479, 252, 480, 514,  114, 245, 681, 413, 119, 237, 118, 121, 251, 418, 418, 262,
	484, 253, 263, 424, 268, 690, 264, 282, 430, 669, 271, 290, 280, 322, 325, 353, 335, 354, 391, 400, 336, 403, 406, 330, 404, 407, 408, 409, 412, 417, 421, 427, 497, 422, 423, 426,	 428, 499, 429, 433, 500, 434, 38,	805,  299,	300, 301, 1,   15,	435, 436, 294, 302, 303, 443, 293, 445, 244, 470, 304, 527, 467, 473, 469, 442, 719, 305, 537, 537, 537, 475, 477, 482, 483, 491, 486, 487, 306, 488, 494,	498, 515, 518, 553, 511, 633,  810, 525, 634, 528, 530, 545, 307, 308, 559, 572, 560, 56,
	615, 616, 492, 617, 623, 567, 720, 621, 825, 626, 627, 759, 630, 644, 629, 309, 310, 635, 831, 611, 712, 834, 636, 637, 651, 665, 656, 672, 694, 38,  38,  660, 696, 622, 38,  725,	 682, 38,  509, 683, 512, 38,  540, 540,  540,	516, 519, 698, 709, 573, 38,  38,  784, 38,	 733, 652, 738, 531, 710, 745, 299, 300, 301, 1,   15,	757, 760, 761, 302, 303, 762, 767, 712, 776, 38,  304, 620, 777, 766, 781,	38,	 785, 305, 312, 791, 793,  797, 798, 802, 803, 679, 725, 811, 306, 806, 687, 813, 819,
	812, 824, 712, 712, 38,	 826, 829, 832, 389, 680, 307, 308, 820, 821, 827, 830, 115, 490, 618, 349, 728, 120, 720, 727, 758, 619, 731, 732, 775, 625, 671, 309, 310, 754, 769, 38,	 770, 753, 38,	29,	 36,  631, 329, 485,  734,	299, 300, 301, 1,	15,	 328, 706, 420, 302, 303, 765, 641, 449, 38,  704, 304, 647, 274, 708, 502, 438, 537, 305, 38,	437, 707, 742, 705, 0,	 0,	  0,   0,	697, 306, 0,	38,	 0,	  0,   0,	0,	 0,	   0,	0,	 0,	  537, 0,	250, 677, 307, 308, 0,	 0,	  0,
	0,	 0,	  0,   0,	0,	 450, 451, 0,	255, 452, 453, 779, 0,	 0,	  24,  0,	309, 310, 0,   0,	0,	 0,	  0,   0,	0,	 24,  0,   0,	0,	 0,	  0,   0,	0,	 537, 773, 687,	 801, 0,   0,	0,	 0,	  540, 0,	0,	  0,	0,	 0,	  0,   38,	0,	 0,	  48,  0,	0,	 0,	  465, 0,	85,	 0,	  691, 0,	38,	 796, 0,   540, 0,	 0,	  0,   38,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	573, 0,	  0,	0,	 0,	  0,   0,	38,	 0,	   0,	647, 0,	  0,   0,	0,	 0,	  0,   0,	718, 0,	  85,
	110, 0,	  0,   0,	0,	 726, 0,   85,	540, 795, 0,   0,	0,	 0,	  110, 0,	0,	 0,	  250, 208, 677, 85,  0,   38,	0,	 85,  691, 0,	0,	 736, 0,   0,	0,	 38,  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	537,  0,	0,	 0,	  0,   752, 718, 529, 0,   0,	0,	 533, 538, 538, 538, 0,	  0,   336, 726, 0,	  312, 533, 0,	 533, 0,   0,	763, 285, 208, 208, 208, 208, 0,   0,	718, 718, 0,	38,	 38,  0,   208, 208, 208,  208, 561, 0,	  0,   38,	0,	 0,	  568, 0,	38,	 0,	  0,
	0,	 347, 0,   0,	347, 0,	  0,   38,	0,	 0,	  208, 0,	0,	 0,	  0,   0,	0,	 0,	  540, 0,	0,	 0,	  0,   0,	0,	 0,	  0,   38,	0,	 0,	  0,   0,	807, 0,	  0,   38,	 0,	  0,   0,	0,	 0,	  0,   814, 0,	  0,	38,	 0,	  0,   38,	0,	 822, 0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 397, 0,   0,	208, 0,	  0,   0,	208, 0,	  657, 0,	0,	 397, 285, 0,	0,	 0,	  285,	0,	 465, 285, 0,	0,	 0,	   0,	0,	 0,	  0,   0,	0,	 0,	  678, 0,	0,	 208, 355,
	356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	 0,	  208, 0,	0,	 208, 0,   0,	208,  0,	0,	 0,	  0,   208, 0,	 0,	  0,   0,	0,	 538, 0,   0,	0,	 0,	  0,   208, 695, 0,	  208, 208, 0,	 208, 0,   0,	0,	 0,	  0,   0,	0,	 533, 0,   0,	538, 0,	  0,	0,	 312, 0,   0,	0,	 0,	   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  723,
	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 397, 208, 0,	0,	 0,	  0,   208, 0,	 538, 208, 285, 506, 0,	  0,   285, 0,	 0,	  0,   0,	285, 0,	  0,   0,	 657, 0,   0,	0,	 0,	  0,   208, 0,	  0,	0,	 723, 0,   0,	539, 539, 539, 0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	208, 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 56,  15,	0,	 0,	  347, 0,	0,	 208,  0,	570, 0,	  0,   0,	0,	 0,	  312, 780, 0,	 0,	  0,
	0,	 208, 0,   0,	0,	 788, 0,   0,	0,	 397, 792, 0,	0,	 0,	  0,   208, 0,	 0,	  0,   0,	312, 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   538, 285, 0,	  0,   285,	 574, 575, 0,	0,	 818, 0,   0,	576,  577,	0,	 578, 0,   579, 580, 581, 582, 0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   583, 0,	 57,  584, 0,	663, 0,	  58,  0,	59,	 0,	  60,  0,	61,	 0,	  0,	0,	 62,  585, 586, 587, 588,  0,	0,	 63,  208, 64,	0,	 589, 590, 591, 0,	 65,  66,
	67,	 68,  69,  0,	0,	 0,	  70,  0,	71,	 0,	  0,   0,	0,	 72,  73,  0,	74,	 75,  76,  77,	78,	 79,  80,  81,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   285, 0,	  0,	0,	 0,	  0,   539, 0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  208, 0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  539, 0,	0,	 0,	  0,   0,	0,	 0,	  0,	0,	 0,	  0,   0,	717, 0,	   0,	0,	 0,	  0,   0,	0,	 0,	  0,   347, 0,	 0,	  0,
	347, 0,	  0,   592, 593, 594, 595, 596, 597, 598, 599, 285, 55,	 0,	  0,   539, 0,	 56,  15,  0,	0,	 0,	  0,   0,	0,	 0,	  48,  0,	0,	 0,	  0,   743, 0,	 0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	  0,	347, 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 717, 717, 0,	96,	 0,	  0,   0,	0,	 56,  15,  0,	0,	 0,	  285,	285, 0,	  0,   0,	0,	 -264, 0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,
	0,	 0,	  0,   0,	0,	 57,  0,   0,	0,	 285, 58,  0,	59,	 0,	  60,  0,	61,	 0,	  347, 0,	62,	 0,	  0,   0,	0,	 539, 0,   63,	0,	 64,  0,   0,	0,	 0,	  0,   65,	 66,  67,  68,	69,	 0,	  0,   0,	70,	  -264, 71,	 0,	  0,   0,	0,	 72,  73,  0,	74,	 75,  76,  77,	78,	 79,  80,  81,	0,	 57,  0,   0,	0,	 0,	  58,  0,	59,	 0,	  60,  0,	61,	 0,	  0,   0,	62,	 0,	  0,	0,	 0,	  0,   0,	63,	 0,	   64,	0,	 0,	  0,   0,	0,	 65,  66,  67,	68,	 69,  0,
	0,	 0,	  70,  0,	71,	 0,	  0,   0,	0,	 72,  73,  0,	74,	 75,  76,  77,	78,	 79,  80,  81,	299, 300, 301, 122, 15,	 0,	  0,   0,	302, 303, 0,   0,	0,	 0,	  0,   304,	 0,	  0,   0,	123, 0,	  0,   305, 0,	  0,	0,	 124, 125, 126, 0,	 127, 0,   0,	536, 129, 0,   130, 0,	 131, 132, 133, 134, 0,	  0,   0,	0,	 0,	  0,   307, 308, 0,	  0,   0,	0,	 135, 0,   0,	136, 0,	  0,	0,	 0,	  0,   0,	0,	 0,	   0,	0,	 0,	  309, 310, 0,	 137, 138, 139, 140, 141, 142,
	143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 0,	 0,	  0,   0,	155, 0,	  156, 0,	157, 0,	  158, 0,	0,	 0,	  159, 0,	0,	 0,	  0,   0,	0,	 160, 0,   161,	 0,	  0,   0,	0,	 0,	  162, 163, 164,  165,	166, 0,	  0,   0,	167, 0,	  168, 0,	0,	 0,	  0,   169, 170, 0,	  171, 172, 173, 174, 175, 176, 177, 178, 122, 15,	0,	 0,	  0,   0,	0,	 0,	  0,   339, 0,	 0,	  0,	340, 0,	  0,   123, 0,	 0,	   0,	0,	 0,	  0,   124, 125, 126, 0,   127, 0,	 0,	  128,
	129, 0,	  130, 0,	131, 132, 133, 134, 0,	 0,	  341, 0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	135, 0,	  0,   136, 0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	 0,	  0,   137, 138, 139, 140, 141, 142,  143,	144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 0,	0,	 0,	  0,   155, 0,	 156, 0,   157, 0,	 158, 0,   0,	0,	 159, 0,   0,	0,	 0,	  0,   0,	160, 0,	  161,	0,	 0,	  0,   0,	0,	 162,  163, 164, 165, 166, 0,	0,	 0,	  167, 0,	168, 0,	  0,
	0,	 0,	  169, 170, 0,	 171, 172, 173, 174, 175, 176, 177, 178, 122, 15,  0,	0,	 0,	  0,   0,	0,	 0,	  339, 0,	0,	 0,	  348, 0,	0,	 123, 0,   0,	0,	 0,	  0,   0,	 124, 125, 126, 0,	 127, 0,   0,	128,  129,	0,	 130, 0,   131, 132, 133, 134, 0,	0,	 341, 0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   135, 0,	 0,	  136, 0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,	0,	 0,	  137, 138, 139, 140,  141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152,
	153, 154, 0,   0,	0,	 0,	  155, 0,	156, 0,	  157, 0,	158, 0,	  0,   0,	159, 0,	  0,   0,	0,	 0,	  0,   160, 0,	 161, 0,   0,	0,	 0,	  0,   162, 163, 164, 165, 166,	 0,	  0,   0,	167, 0,	  168, 122, 15,	  0,	0,	 169, 170, 0,	171, 172, 173, 174, 175, 176, 177, 178, 0,	 123, 0,   0,	0,	 0,	  0,   0,	124, 125, 126, 0,	127, 0,	  0,   128, 129, 0,	  130, 350, 131, 132, 133,	134, 0,	  0,   0,	0,	 0,	   0,	0,	 0,	  0,   0,	0,	 351, 135, 0,	0,	 136, 0,
	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 0,	0,	 0,	  0,   155,	 0,	  156, 0,	157, 0,	  158, 0,	0,	  0,	159, 0,	  0,   0,	0,	 0,	  0,   160, 0,	 161, 0,   0,	0,	 0,	  0,   162, 163, 164, 165, 166, 0,	 0,	  0,   167, 0,	 168, 122, 15,	0,	 0,	  169, 170, 0,	 171, 172,	173, 174, 175, 176, 177, 178,  0,	123, 0,	  353, 0,	0,	 0,	  0,   124, 125, 126, 0,
	127, 0,	  0,   128, 129, 0,	  130, 0,	131, 132, 133, 134, 0,	 0,	  476, 0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	135, 0,	  0,   136, 0,	 0,	  0,   0,	0,	 0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   137, 138,  139,	140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 0,	0,	 0,	  0,   155, 0,	 156, 0,   157, 0,	 158, 0,   0,	0,	 159, 0,   0,	0,	 0,	  0,	0,	 160, 0,   161, 0,	 0,	   0,	0,	 0,	  162, 163, 164, 165, 166, 0,	0,	 0,	  167,
	0,	 168, 0,   0,	0,	 0,	  169, 170, 0,	 171, 172, 173, 174, 175, 176, 177, 178, 122, 15,  0,	0,	 0,	  0,   0,	0,	 0,	  562, 0,	0,	 0,	  0,   0,	0,	 563, 0,   0,	 0,	  0,   0,	0,	 124, 125, 126, 0,	  127,	0,	 0,	  128, 129, 0,	 130, 0,   131, 132, 133, 134, 0,	0,	 341, 0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   135, 0,	 0,	  136, 0,	0,	 0,	  0,   0,	0,	 0,	  0,	0,	 0,	  0,   0,	0,	 0,	   137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148,
	149, 150, 151, 152, 153, 154, 0,   0,	0,	 0,	  155, 0,	156, 0,	  157, 0,	158, 0,	  0,   0,	159, 0,	  0,   0,	0,	 0,	  0,   160, 0,	 161, 0,   0,	0,	 0,	  0,   162,	 163, 164, 165, 166, 0,	  0,   0,	167,  0,	168, 0,	  0,   0,	0,	 169, 170, 0,	171, 172, 173, 174, 175, 176, 177, 178, 122, 15,  0,   0,	0,	 0,	  0,   0,	0,	 339, 0,   0,	0,	 0,	  0,   0,	123, 0,	  0,	0,	 0,	  0,   0,	124, 125,  126, 0,	 127, 0,   0,	128, 129, 0,   130, 0,	 131, 132,
	133, 134, 0,   0,	341, 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  135, 0,	0,	 136, 0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	137, 138, 139, 140,	 141, 142, 143, 144, 145, 146, 147, 148,  149,	150, 151, 152, 153, 154, 0,	  0,   0,	0,	 155, 0,   156, 0,	 157, 0,   158, 0,	 0,	  0,   159, 0,	 0,	  0,   0,	0,	 0,	  160, 0,	161, 0,	  0,   0,	0,	 0,	  162,	163, 164, 165, 166, 0,	 0,	   0,	167, 0,	  168, 122, 15,	 0,	  0,   169, 170, 0,	  171,
	172, 173, 174, 175, 176, 177, 178, 0,	123, 0,	  353, 0,	0,	 0,	  0,   124, 125, 126, 0,   127, 0,	 0,	  128, 129, 0,	 130, 0,   131, 132, 133, 134, 0,	0,	 0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	135,  0,	0,	 136, 0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 0,	0,	 0,	  0,   155, 0,	 156,  0,	157, 0,	  158, 0,	0,	 0,	  159, 0,	0,	 0,	  0,
	0,	 0,	  160, 0,	161, 0,	  0,   0,	0,	 0,	  162, 163, 164, 165, 166, 0,	0,	 0,	  167, 0,	168, 0,	  0,   0,	0,	 169, 170, 0,	171, 172, 173, 174, 175, 176, 177, 178,	 122, 15,  0,	0,	 0,	  0,   0,	0,	  0,	501, 0,	  0,   0,	0,	 0,	  0,   123, 0,	 0,	  0,   0,	0,	 0,	  124, 125, 126, 0,	  127, 0,	0,	 128, 129, 0,	130, 0,	  131, 132, 133, 134, 0,   0,	0,	 0,	  0,	0,	 0,	  0,   0,	0,	 0,	   0,	135, 0,	  0,   136, 0,	 0,	  0,   0,	0,	 0,	  0,
	0,	 0,	  0,   0,	0,	 0,	  0,   137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 0,	  0,   0,	0,	 155, 0,   156, 0,	 157, 0,   158,	 0,	  0,   0,	159, 0,	  0,   0,	0,	  0,	0,	 160, 0,   161, 0,	 0,	  0,   0,	0,	 162, 163, 164, 165, 166, 0,   0,	0,	 167, 0,   168, 0,	 0,	  0,   0,	169, 170, 0,   171, 172, 173, 174, 175, 176, 177, 178,	122, 15,  0,   0,	0,	 0,	   0,	0,	 0,	  685, 0,	0,	 0,	  0,   0,	0,	 686, 0,
	0,	 0,	  0,   0,	0,	 124, 125, 126, 0,	 127, 0,   0,	128, 129, 0,   130, 0,	 131, 132, 133, 134, 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 135, 0,   0,	 136, 0,   0,	0,	 0,	  0,   0,	0,	  0,	0,	 0,	  0,   0,	0,	 0,	  137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 0,	 0,	  0,   0,	155, 0,	  156, 0,	157, 0,	  158,	0,	 0,	  0,   159, 0,	 0,	   0,	0,	 0,	  0,   160, 0,	 161, 0,   0,	0,	 0,	  0,
	162, 163, 164, 165, 166, 0,	  0,   0,	167, 0,	  168, 122, 15,	 0,	  0,   169, 170, 0,	  171, 172, 173, 174, 175, 176, 177, 178, 0,   563, 0,	 0,	  0,   0,	0,	 0,	  124, 125,	 126, 0,   127, 0,	 0,	  128, 129, 0,	  130,	0,	 131, 132, 133, 134, 0,	  0,   341, 0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 135, 0,   0,	136, 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,	137, 138, 139, 140, 141, 142,  143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154,
	0,	 0,	  0,   0,	155, 0,	  156, 0,	157, 0,	  158, 0,	0,	 0,	  159, 0,	0,	 0,	  0,   0,	0,	 160, 0,   161, 0,	 0,	  0,   0,	0,	 162, 163, 164, 165, 166, 0,   0,	 0,	  167, 0,	168, 122, 15,  0,	0,	  169,	170, 0,	  171, 172, 173, 174, 175, 176, 177, 178, 0,   123, 0,	 0,	  0,   0,	0,	 0,	  124, 125, 126, 0,	  127, 0,	0,	 128, 129, 0,	130, 0,	  131, 132, 133, 134, 0,	0,	 341, 0,   0,	0,	 0,	   0,	0,	 0,	  0,   0,	135, 0,	  0,   136, 0,	 0,	  0,
	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 0,	  0,   0,	0,	 155, 0,   156,	 0,	  157, 0,	158, 0,	  0,   0,	159,  0,	0,	 0,	  0,   0,	0,	 160, 0,   161, 0,	 0,	  0,   0,	0,	 162, 163, 164, 165, 166, 0,   0,	0,	 167, 0,   168, 122, 15,  0,   0,	169, 170, 0,   171, 172, 173, 174,	175, 176, 177, 178, 0,	 123,  0,	0,	 0,	  0,   0,	0,	 124, 125, 126, 0,	 127, 0,
	0,	 128, 129, 0,	130, 0,	  131, 132, 133, 134, 0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  135, 0,	0,	 136, 0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	 0,	  0,   0,	0,	 137, 138, 139, 140,  141,	142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 0,	  0,   0,	0,	 155, 0,   156, 0,	 157, 0,   158, 0,	 0,	  0,   159, 0,	 0,	  0,   0,	0,	 0,	  160,	0,	 161, 0,   0,	0,	 0,	   0,	162, 163, 164, 165, 166, 0,	  0,   0,	167, 0,	  168,
	122, 15,  0,   0,	169, 170, 0,   171, 172, 173, 174, 175, 176, 177, 178, 0,	686, 0,	  0,   0,	0,	 0,	  0,   124, 125, 126, 0,   127, 0,	 0,	  128, 129, 0,	 130, 0,   131,	 132, 133, 134, 0,	 0,	  0,   0,	0,	  0,	0,	 0,	  0,   0,	0,	 0,	  135, 0,	0,	 136, 0,   122, 0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147,	148, 149, 150, 151, 152, 153,  154, 0,	 0,	  0,   0,	155, 0,	  156, 0,	157, 0,	  158,
	0,	 0,	  0,   159, 0,	 241, 0,   0,	0,	 0,	  160, 0,	161, 0,	  56,  0,	0,	 0,	  162, 163, 164, 165, 166, 0,	0,	 0,	  167, 0,	168, 0,	  0,   0,	0,	 169, 170, 0,	 171, 172, 173, 174, 175, 176, 177, 178,  57,	0,	 0,	  0,   0,	58,	 0,	  59,  0,	60,	 0,	  61,  0,	0,	 0,	  62,  0,	0,	 0,	  0,   0,	0,	 63,  0,   64,	0,	 0,	  0,   0,	0,	 65,  66,  67,	68,	 69,  0,	0,	 0,	  70,  0,	71,	 0,	   0,	0,	 0,	  72,  73,	0,	 74,  75,  76,	77,	 78,  79,
	80,	 81,  57,  0,	0,	 0,	  0,   58,	0,	 59,  0,   60,	0,	 61,  0,   0,	0,	 62,  0,   0,	0,	 0,	  0,   0,	63,	 0,	  64,  0,	0,	 0,	  0,   0,	65,	 66,  67,  68,	 69,  0,   0,	0,	 70,  0,   71,	0,	  0,	0,	 0,	  72,  73,	0,	 74,  75,  76,	77,	 78,  79,  80,	81,	 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376,	377, 378, 379, 380, 381};

#define yypact_value_is_default(Yystate) (!!((Yystate) == (-752)))

#define yytable_value_is_error(Yytable_value) YYID(0)

static const yytype_int16 yycheck[] = {
	0,	 168, 255, 249, 30,	 263, 95,  426, 20,	 467, 50,  119, 434, 435, 436, 422, 20,	 344, 20,  50,	38,	 17,  4,   38,	6,	 161, 777, 1,	117, 22,  30,  7,	6,	 7,	  8,   16,	45,	 110, 18,  15,	129, 130, 131, 24,	20,	 27,  64,  22,	799, 64,  90,  91,	105, 106, 34,  48,	49,	 50,  98,  68,	69,	 43,  47,  6,	46,	 105, 0,   40,	50,	 65,  52,  7,	112, 48,  49,  50,	116, 55,  131, 119, 54,	 112, 67,  56,	20,	 116, 11,  12,	1,	 162, 163, 164, 165, 6,	  7,   8,	48,	 49,
	50,	 101, 18,  174, 175, 176, 177, 109, 6,	 7,	  90,  528, 7,	 11,  12,  28,	11,	 12,  28,  20,	525, 119, 26,  17,	19,	 26,  16,  25,	199, 23,  264, 17,	21,	 19,  268, 6,	7,	 271, 27,  149, 11,	 12,  398, 123, 7,	 17,  104, 105, 11,	 12,  475, 23,	45,	 46,  241, 183, 184, 185, 186, 187, 188, 189, 190, 191, 119, 120, 121, 17,	20,	 19,  168, 17,	15,	 19,  154, 155, 156, 157, 158, 504, 17,	 21,  19,  254, 51,	 52,  166, 258, 21,	 169, 170, 171, 172, 17,  53,  19,	3,	 4,
	5,	 6,	  7,   452, 128, 129, 11,  12,	17,	 17,  19,  19,	15,	 18,  17,  17,	285, 19,  11,  12,	25,	 675, 17,  15,	19,	 17,  17,  249, 19,	 7,	  17,  36,	19,	 17,  17,  19,	19,	 17,  17,  19,	19,	 17,  182, 19,	240, 200, 51,  52,	443, 5,	  445, 58,	337, 249, 54,  324, 20,	 21,  327, 6,	7,	 330, 135, 136, 54,	 59,  335, 72,	73,	 354, 724, 129, 130, 74,  17,  74,	408, 74,  347, 16,	412, 350, 351, 32,	353, 417, 74,  391, 611, 267, 26,  26,	74,	 74,  18,  273, 274, 70,
	383, 32,  18,  279, 98,	 626, 93,  115, 284, 556, 93,  127, 108, 139, 65,  24,	148, 16,  16,  20,	20,	 18,  18,  98,	38,	 38,  18,  71,	94,	 94,  18,  99,	399, 18,  106, 18,	65,	 404, 46,  18,	407, 18,  336, 795, 3,	 4,	  5,   6,	7,	 18,  18,  106, 11,	 12,  18,  105, 18,	 391, 38,  18,	427, 17,  19,  339, 133, 24,  25,  434, 435, 436, 17,  19,	200, 200, 23,  200, 200, 36,  200, 20,	34,	 100, 102, 450, 95,	 515, 802, 18,	518, 18,  117, 10,	51,	 52,  16,  38,	24,	 6,
	5,	 12,  17,  9,	17,	 470, 63,  19,	819, 17,  19,  730, 96,	 118, 18,  72,	73,	 19,  829, 484, 665, 832, 17,  19,	16,	 18,  17,  16,	18,	 421, 422, 134, 19,	 498, 426, 673, 21,	 429, 412, 21,	414, 433, 434, 435, 436, 419, 420, 19,	17,	 481, 442, 443, 771, 445, 19,  17,	17,	 431, 150, 17,	3,	 4,	  5,   6,	7,	 14,  19,  104, 11,	 12,  112, 151, 717, 18,  466, 18,	494, 18,  66,  152, 472, 23,  25,  168, 17,	 153, 23,  19,	18,	 154, 571, 729, 18,	 36,  104, 623, 110, 18,
	38,	 19,  745, 746, 494, 38,  18,  18,	240, 572, 51,  52,	113, 111, 113, 38,	98,	 391, 492, 193, 679, 105, 63,  675, 729, 494, 688, 690, 759, 501, 558, 72,	73,	 719, 745, 525, 746, 717, 528, 18,	30,	 513, 172, 384, 694, 3,	  4,   5,	6,	 7,	  171, 652, 274, 11,  12,  738, 528, 15,  548, 649, 18,	 533, 157, 657, 690, 298, 629, 25,	558, 297, 656, 709, 651, -1,  -1,  -1,	-1,	 640, 36,  -1,	570, -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 652, -1,  563, 564, 51,  52,  -1,	-1,	 -1,
	-1,	 -1,  -1,  -1,	-1,	 61,  62,  -1,	64,	 65,  66,  766, -1,	 -1,  7,   -1,	72,	 73,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 18,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 694, 758, 759, 791, -1,  -1,  -1,	-1,	 629, -1,  -1,	-1,	 -1,  -1,  -1,	636, -1,  -1,  44,	-1,	 -1,  -1,  336, -1,	 50,  -1,  629, -1,	 649, 786, -1,	652, -1,  -1,  -1,	656, -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 705, -1,  -1,	-1,	 -1,  -1,  -1,	672, -1,  -1,  657, -1,	 -1,  -1,  -1,	-1,	 -1,  -1,  665, -1,	 90,
	91,	 -1,  -1,  -1,	-1,	 673, -1,  98,	694, 784, -1,  -1,	-1,	 -1,  105, -1,	-1,	 -1,  686, 110, 688, 112, -1,  709, -1,	 116, 694, -1,	-1,	 697, -1,  -1,	-1,	 719, -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  802, -1,	 -1,  -1,  -1,	716, 717, 429, -1,	-1,	 -1,  433, 434, 435, 436, -1,  -1,	20,	 729, -1,  442, 443, -1,  445, -1,	-1,	 737, 161, 162, 163, 164, 165, -1,	-1,	 745, 746, -1,	766, 767, -1,  174, 175, 176, 177, 466, -1,	 -1,  776, -1,	-1,	 472, -1,  781, -1,	 -1,
	-1,	 190, -1,  -1,	193, -1,  -1,  791, -1,	 -1,  199, -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  802, -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  811, -1,	 -1,  -1,  -1,	798, -1,  -1,  819, -1,	 -1,  -1,  -1,	-1,	 -1,  808, -1,	-1,	 829, -1,  -1,	832, -1,  816, -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  251, -1,	-1,	 254, -1,  -1,	-1,	 258, -1,  548, -1,	 -1,  263, 264, -1,	 -1,  -1,  268, -1,	 558, 271, -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 570, -1,  -1,	285, 155,
	156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 324, -1,  -1,	327, -1,  -1,  330, -1,	 -1,  -1,  -1,	335, -1,  -1,  -1,	-1,	 -1,  629, -1,	-1,	 -1,  -1,  -1,	347, 636, -1,  350, 351, -1,  353, -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  649, -1,	-1,	 652, -1,  -1,	-1,	 656, -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 672,
	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 398, 399, -1,	-1,	 -1,  -1,  404, -1,	 694, 407, 408, 409, -1,  -1,  412, -1,	 -1,  -1,  -1,	417, -1,  -1,  -1,	709, -1,  -1,  -1,	-1,	 -1,  427, -1,	-1,	 -1,  719, -1,	-1,	 434, 435, 436, -1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 450, -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  6,   7,	-1,	 -1,  467, -1,	-1,	 470, -1,  472, -1,	 -1,  -1,  -1,	-1,	 766, 767, -1,	-1,	 -1,
	-1,	 484, -1,  -1,	-1,	 776, -1,  -1,	-1,	 492, 781, -1,	-1,	 -1,  -1,  498, -1,	 -1,  -1,  -1,	791, -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  802, 515, -1,  -1,  518, 29,	 30,  -1,  -1,	811, -1,  -1,  36,	37,	 -1,  39,  -1,	41,	 42,  43,  44,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	57,	 -1,  92,  60,	-1,	 552, -1,  97,	-1,	 99,  -1,  101, -1,	 103, -1,  -1,	-1,	 107, 75,  76,	77,	 78,  -1,  -1,	114, 572, 116, -1,	85,	 86,  87,  -1,	122, 123,
	124, 125, 126, -1,	-1,	 -1,  130, -1,	132, -1,  -1,  -1,	-1,	 137, 138, -1,	140, 141, 142, 143, 144, 145, 146, 147, -1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  623, -1,	-1,	 -1,  -1,  -1,	629, -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  640, -1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  652, -1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	665, -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  675, -1,	-1,	 -1,
	679, -1,  -1,  192, 193, 194, 195, 196, 197, 198, 199, 690, 1,	 -1,  -1,  694, -1,	 6,	  7,   -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  705, -1,	-1,	 -1,  -1,  710, -1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 724, -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  745, 746, -1,	 1,	  -1,  -1,	-1,	 -1,  6,   7,	-1,	 -1,  -1,  758, 759, -1,  -1,  -1,	-1,	 74,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,
	-1,	 -1,  -1,  -1,	-1,	 92,  -1,  -1,	-1,	 786, 97,  -1,	99,	 -1,  101, -1,	103, -1,  795, -1,	107, -1,  -1,  -1,	-1,	 802, -1,  114, -1,	 116, -1,  -1,	-1,	 -1,  -1,  122, 123, 124, 125, 126, -1,	 -1,  -1,  130, 74,	 132, -1,  -1,	-1,	 -1,  137, 138, -1,	 140, 141, 142, 143, 144, 145, 146, 147, -1,  92,  -1,	-1,	 -1,  -1,  97,	-1,	 99,  -1,  101, -1,	 103, -1,  -1,	-1,	 107, -1,  -1,	-1,	 -1,  -1,  -1,	114, -1,  116, -1,	-1,	 -1,  -1,  -1,	122, 123, 124, 125, 126, -1,
	-1,	 -1,  130, -1,	132, -1,  -1,  -1,	-1,	 137, 138, -1,	140, 141, 142, 143, 144, 145, 146, 147, 3,	 4,	  5,   6,	7,	 -1,  -1,  -1,	11,	 12,  -1,  -1,	-1,	 -1,  -1,  18,	-1,	 -1,  -1,  22,	-1,	 -1,  25,  -1,	-1,	 -1,  29,  30,	31,	 -1,  33,  -1,	-1,	 36,  37,  -1,	39,	 -1,  41,  42,	43,	 44,  -1,  -1,	-1,	 -1,  -1,  -1,	51,	 52,  -1,  -1,	-1,	 -1,  57,  -1,	-1,	 60,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 72,  73,  -1,	75,	 76,  77,  78,	79,	 80,
	81,	 82,  83,  84,	85,	 86,  87,  88,	89,	 90,  91,  92,	-1,	 -1,  -1,  -1,	97,	 -1,  99,  -1,	101, -1,  103, -1,	-1,	 -1,  107, -1,	-1,	 -1,  -1,  -1,	-1,	 114, -1,  116, -1,	 -1,  -1,  -1,	-1,	 122, 123, 124, 125, 126, -1,  -1,	-1,	 130, -1,  132, -1,	 -1,  -1,  -1,	137, 138, -1,  140, 141, 142, 143, 144, 145, 146, 147, 6,	7,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	15,	 -1,  -1,  -1,	19,	 -1,  -1,  22,	-1,	 -1,  -1,  -1,	-1,	 -1,  29,  30,	31,	 -1,  33,  -1,	-1,	 36,
	37,	 -1,  39,  -1,	41,	 42,  43,  44,	-1,	 -1,  47,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	57,	 -1,  -1,  60,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  75,  76,	77,	 78,  79,  80,	81,	 82,  83,  84,	85,	 86,  87,  88,	89,	 90,  91,  92,	-1,	 -1,  -1,  -1,	97,	 -1,  99,  -1,	101, -1,  103, -1,	-1,	 -1,  107, -1,	-1,	 -1,  -1,  -1,	-1,	 114, -1,  116, -1,	 -1,  -1,  -1,	-1,	 122, 123, 124, 125, 126, -1,  -1,	-1,	 130, -1,  132, -1,	 -1,
	-1,	 -1,  137, 138, -1,	 140, 141, 142, 143, 144, 145, 146, 147, 6,	  7,   -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  15,  -1,	-1,	 -1,  19,  -1,	-1,	 22,  -1,  -1,	-1,	 -1,  -1,  -1,	29,	 30,  31,  -1,	33,	 -1,  -1,  36,	37,	 -1,  39,  -1,	41,	 42,  43,  44,	-1,	 -1,  47,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	57,	 -1,  -1,  60,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  75,  76,	77,	 78,  79,  80,	81,	 82,  83,  84,	85,	 86,  87,  88,	89,	 90,
	91,	 92,  -1,  -1,	-1,	 -1,  97,  -1,	99,	 -1,  101, -1,	103, -1,  -1,  -1,	107, -1,  -1,  -1,	-1,	 -1,  -1,  114, -1,	 116, -1,  -1,	-1,	 -1,  -1,  122, 123, 124, 125, 126, -1,	 -1,  -1,  130, -1,	 132, 6,   7,	-1,	 -1,  137, 138, -1,	 140, 141, 142, 143, 144, 145, 146, 147, -1,  22,  -1,	-1,	 -1,  -1,  -1,	-1,	 29,  30,  31,	-1,	 33,  -1,  -1,	36,	 37,  -1,  39,	40,	 41,  42,  43,	44,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	56,	 57,  -1,  -1,	60,	 -1,
	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 75,  76,  77,	78,	 79,  80,  81,	82,	 83,  84,  85,	86,	 87,  88,  89,	90,	 91,  92,  -1,	-1,	 -1,  -1,  97,	-1,	 99,  -1,  101, -1,	 103, -1,  -1,	-1,	 107, -1,  -1,	-1,	 -1,  -1,  -1,	114, -1,  116, -1,	-1,	 -1,  -1,  -1,	122, 123, 124, 125, 126, -1,  -1,  -1,	130, -1,  132, 6,	7,	 -1,  -1,  137, 138, -1,  140, 141, 142, 143, 144, 145, 146, 147, -1,  22,	-1,	 24,  -1,  -1,	-1,	 -1,  29,  30,	31,	 -1,
	33,	 -1,  -1,  36,	37,	 -1,  39,  -1,	41,	 42,  43,  44,	-1,	 -1,  47,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	57,	 -1,  -1,  60,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  75,  76,	77,	 78,  79,  80,	81,	 82,  83,  84,	85,	 86,  87,  88,	89,	 90,  91,  92,	-1,	 -1,  -1,  -1,	97,	 -1,  99,  -1,	101, -1,  103, -1,	-1,	 -1,  107, -1,	-1,	 -1,  -1,  -1,	-1,	 114, -1,  116, -1,	 -1,  -1,  -1,	-1,	 122, 123, 124, 125, 126, -1,  -1,	-1,	 130,
	-1,	 132, -1,  -1,	-1,	 -1,  137, 138, -1,	 140, 141, 142, 143, 144, 145, 146, 147, 6,	  7,   -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  15,  -1,	-1,	 -1,  -1,  -1,	-1,	 22,  -1,  -1,	-1,	 -1,  -1,  -1,	29,	 30,  31,  -1,	33,	 -1,  -1,  36,	37,	 -1,  39,  -1,	41,	 42,  43,  44,	-1,	 -1,  47,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	57,	 -1,  -1,  60,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  75,  76,	77,	 78,  79,  80,	81,	 82,  83,  84,	85,	 86,
	87,	 88,  89,  90,	91,	 92,  -1,  -1,	-1,	 -1,  97,  -1,	99,	 -1,  101, -1,	103, -1,  -1,  -1,	107, -1,  -1,  -1,	-1,	 -1,  -1,  114, -1,	 116, -1,  -1,	-1,	 -1,  -1,  122, 123, 124, 125, 126, -1,	 -1,  -1,  130, -1,	 132, -1,  -1,	-1,	 -1,  137, 138, -1,	 140, 141, 142, 143, 144, 145, 146, 147, 6,	  7,   -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  15,  -1,	-1,	 -1,  -1,  -1,	-1,	 22,  -1,  -1,	-1,	 -1,  -1,  -1,	29,	 30,  31,  -1,	33,	 -1,  -1,  36,	37,	 -1,  39,  -1,	41,	 42,
	43,	 44,  -1,  -1,	47,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  57,  -1,	-1,	 60,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	75,	 76,  77,  78,	79,	 80,  81,  82,	83,	 84,  85,  86,	87,	 88,  89,  90,	91,	 92,  -1,  -1,	-1,	 -1,  97,  -1,	99,	 -1,  101, -1,	103, -1,  -1,  -1,	107, -1,  -1,  -1,	-1,	 -1,  -1,  114, -1,	 116, -1,  -1,	-1,	 -1,  -1,  122, 123, 124, 125, 126, -1,	 -1,  -1,  130, -1,	 132, 6,   7,	-1,	 -1,  137, 138, -1,	 140,
	141, 142, 143, 144, 145, 146, 147, -1,	22,	 -1,  24,  -1,	-1,	 -1,  -1,  29,	30,	 31,  -1,  33,	-1,	 -1,  36,  37,	-1,	 39,  -1,  41,	42,	 43,  44,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  57,	-1,	 -1,  60,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 75,  76,  77,	78,	 79,  80,  81,	82,	 83,  84,  85,	86,	 87,  88,  89,	90,	 91,  92,  -1,	-1,	 -1,  -1,  97,	-1,	 99,  -1,  101, -1,	 103, -1,  -1,	-1,	 107, -1,  -1,	-1,	 -1,
	-1,	 -1,  114, -1,	116, -1,  -1,  -1,	-1,	 -1,  122, 123, 124, 125, 126, -1,	-1,	 -1,  130, -1,	132, -1,  -1,  -1,	-1,	 137, 138, -1,	140, 141, 142, 143, 144, 145, 146, 147, 6,	 7,	  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 15,  -1,  -1,	-1,	 -1,  -1,  -1,	22,	 -1,  -1,  -1,	-1,	 -1,  -1,  29,	30,	 31,  -1,  33,	-1,	 -1,  36,  37,	-1,	 39,  -1,  41,	42,	 43,  44,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  57,	-1,	 -1,  60,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,
	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  75,	76,	 77,  78,  79,	80,	 81,  82,  83,	84,	 85,  86,  87,	88,	 89,  90,  91,	92,	 -1,  -1,  -1,	-1,	 97,  -1,  99,	-1,	 101, -1,  103, -1,	 -1,  -1,  107, -1,	 -1,  -1,  -1,	-1,	 -1,  114, -1,	116, -1,  -1,  -1,	-1,	 -1,  122, 123, 124, 125, 126, -1,	-1,	 -1,  130, -1,	132, -1,  -1,  -1,	-1,	 137, 138, -1,	140, 141, 142, 143, 144, 145, 146, 147, 6,	 7,	  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 15,  -1,  -1,	-1,	 -1,  -1,  -1,	22,	 -1,
	-1,	 -1,  -1,  -1,	-1,	 29,  30,  31,	-1,	 33,  -1,  -1,	36,	 37,  -1,  39,	-1,	 41,  42,  43,	44,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 57,  -1,  -1,	60,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  75,	76,	 77,  78,  79,	80,	 81,  82,  83,	84,	 85,  86,  87,	88,	 89,  90,  91,	92,	 -1,  -1,  -1,	-1,	 97,  -1,  99,	-1,	 101, -1,  103, -1,	 -1,  -1,  107, -1,	 -1,  -1,  -1,	-1,	 -1,  114, -1,	116, -1,  -1,  -1,	-1,	 -1,
	122, 123, 124, 125, 126, -1,  -1,  -1,	130, -1,  132, 6,	7,	 -1,  -1,  137, 138, -1,  140, 141, 142, 143, 144, 145, 146, 147, -1,  22,	-1,	 -1,  -1,  -1,	-1,	 -1,  29,  30,	31,	 -1,  33,  -1,	-1,	 36,  37,  -1,	39,	 -1,  41,  42,	43,	 44,  -1,  -1,	47,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  57,  -1,	-1,	 60,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	75,	 76,  77,  78,	79,	 80,  81,  82,	83,	 84,  85,  86,	87,	 88,  89,  90,	91,	 92,
	-1,	 -1,  -1,  -1,	97,	 -1,  99,  -1,	101, -1,  103, -1,	-1,	 -1,  107, -1,	-1,	 -1,  -1,  -1,	-1,	 114, -1,  116, -1,	 -1,  -1,  -1,	-1,	 122, 123, 124, 125, 126, -1,  -1,	-1,	 130, -1,  132, 6,	 7,	  -1,  -1,	137, 138, -1,  140, 141, 142, 143, 144, 145, 146, 147, -1,	22,	 -1,  -1,  -1,	-1,	 -1,  -1,  29,	30,	 31,  -1,  33,	-1,	 -1,  36,  37,	-1,	 39,  -1,  41,	42,	 43,  44,  -1,	-1,	 47,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  57,	-1,	 -1,  60,  -1,	-1,	 -1,
	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  75,	76,	 77,  78,  79,	80,	 81,  82,  83,	84,	 85,  86,  87,	88,	 89,  90,  91,	92,	 -1,  -1,  -1,	-1,	 97,  -1,  99,	-1,	 101, -1,  103, -1,	 -1,  -1,  107, -1,	 -1,  -1,  -1,	-1,	 -1,  114, -1,	116, -1,  -1,  -1,	-1,	 -1,  122, 123, 124, 125, 126, -1,	-1,	 -1,  130, -1,	132, 6,	  7,   -1,	-1,	 137, 138, -1,	140, 141, 142, 143, 144, 145, 146, 147, -1,	 22,  -1,  -1,	-1,	 -1,  -1,  -1,	29,	 30,  31,  -1,	33,	 -1,
	-1,	 36,  37,  -1,	39,	 -1,  41,  42,	43,	 44,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  57,  -1,	-1,	 60,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	75,	 76,  77,  78,	79,	 80,  81,  82,	83,	 84,  85,  86,	87,	 88,  89,  90,	91,	 92,  -1,  -1,	-1,	 -1,  97,  -1,	99,	 -1,  101, -1,	103, -1,  -1,  -1,	107, -1,  -1,  -1,	-1,	 -1,  -1,  114, -1,	 116, -1,  -1,	-1,	 -1,  -1,  122, 123, 124, 125, 126, -1,	 -1,  -1,  130, -1,	 132,
	6,	 7,	  -1,  -1,	137, 138, -1,  140, 141, 142, 143, 144, 145, 146, 147, -1,	22,	 -1,  -1,  -1,	-1,	 -1,  -1,  29,	30,	 31,  -1,  33,	-1,	 -1,  36,  37,	-1,	 39,  -1,  41,	42,	 43,  44,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  57,	-1,	 -1,  60,  -1,	6,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 -1,  -1,  -1,	-1,	 75,  76,  77,	78,	 79,  80,  81,	82,	 83,  84,  85,	86,	 87,  88,  89,	90,	 91,  92,  -1,	-1,	 -1,  -1,  97,	-1,	 99,  -1,  101, -1,	 103,
	-1,	 -1,  -1,  107, -1,	 53,  -1,  -1,	-1,	 -1,  114, -1,	116, -1,  6,   -1,	-1,	 -1,  122, 123, 124, 125, 126, -1,	-1,	 -1,  130, -1,	132, -1,  -1,  -1,	-1,	 137, 138, -1,	140, 141, 142, 143, 144, 145, 146, 147, 92,	 -1,  -1,  -1,	-1,	 97,  -1,  99,	-1,	 101, -1,  103, -1,	 -1,  -1,  107, -1,	 -1,  -1,  -1,	-1,	 -1,  114, -1,	116, -1,  -1,  -1,	-1,	 -1,  122, 123, 124, 125, 126, -1,	-1,	 -1,  130, -1,	132, -1,  -1,  -1,	-1,	 137, 138, -1,	140, 141, 142, 143, 144, 145,
	146, 147, 92,  -1,	-1,	 -1,  -1,  97,	-1,	 99,  -1,  101, -1,	 103, -1,  -1,	-1,	 107, -1,  -1,	-1,	 -1,  -1,  -1,	114, -1,  116, -1,	-1,	 -1,  -1,  -1,	122, 123, 124, 125, 126, -1,  -1,  -1,	130, -1,  132, -1,	-1,	 -1,  -1,  137, 138, -1,  140, 141, 142, 143, 144, 145, 146, 147, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] = {0,	6,	 203, 207, 323, 0,	 55,  18,  208, 312, 325, 40,  56,	205, 325, 7,   11,	12,	 313, 314, 315, 316, 317, 321, 322, 28,	 28,  204, 19,	314, 20,  15,  206, 325, 304, 305, 315, 322, 323, 26,  21,	21,	 16,  53,  202, 182, 209, 325, 322, 200, 58,  210, 211, 325, 5,	  1,   6,	92,	 97,  99,  101, 103, 107, 114, 116, 122, 123, 124, 125, 126, 130, 132, 137, 138, 140, 141, 142, 143, 144, 145, 146, 147, 212, 213,
									   219, 322, 324, 325, 336, 54,	 59,  214, 325, 74,	 74,  17,  1,	215, 216, 217, 218, 219, 325, 1,   8,	220, 221, 222, 225, 302, 322, 324, 202, 74,	 74,  217, 17,	65,	 74,  26,  221, 74,	 6,	  22,  29,	30,	 31,  33,  36,	37,	 39,  41,  42,	43,	 44,  57,  60,	75,	 76,  77,  78,	79,	 80,  81,  82,	83,	 84,  85,  86,	87,	 88,  89,  90,	91,	 92,  97,  99,	101, 103, 107, 114, 116, 122, 123, 124, 125, 126, 130,
									   132, 137, 138, 140, 141, 142, 143, 144, 145, 146, 147, 226, 227, 228, 229, 231, 232, 236, 237, 238, 240, 241, 242, 243, 254, 255, 256, 257, 258, 266, 267, 268, 271, 272, 273, 274, 275, 276, 277, 278, 322, 323, 324, 335, 337, 344, 346, 350, 352, 357, 366, 372, 377, 378, 379, 380, 381, 386, 390, 399, 401, 403, 404, 405, 406, 407, 408, 409, 410, 26,	 219, 219, 202, 53,	 224, 323, 324, 336, 48,  49,  50,	270, 325, 18,
									   32,	32,	 38,  64,  202, 290, 38,  202, 290, 202, 70,  18,  93,	325, 338, 339, 98,	325, 345, 93,  325, 347, 347, 104, 105, 325, 353, 354, 108, 358, 115, 228, 230, 322, 228, 228, 228, 228, 127, 325, 382, 105, 106, 131, 387, 388, 389, 3,   4,	5,	 11,  12,  18,	25,	 36,  51,  52,	72,	 73,  235, 303, 304, 306, 308, 309, 310, 318, 319, 320, 398, 139, 325, 400, 65,	 325, 402, 345, 338, 98,  228, 228, 228, 228, 148,
									   20,	26,	 279, 15,  19,	47,	 230, 244, 245, 246, 253, 322, 19,	244, 40,  56,  228, 24,	 16,  155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 325, 326, 327, 328, 331, 333, 334, 207, 202, 16,  269, 304, 321, 233, 234, 322, 18,  34,  20,	228, 279, 18,  38,	228, 18,  38,  18,	71,	 233, 230, 94,	325, 340, 230, 230, 94,	 325, 348,
									   348, 18,	 18,  106, 325, 355, 18,  99,  65,	46,	 325, 373, 228, 18,	 18,  18,  18,	389, 388, 307, 11,	12,	 133, 18,  228, 18,	 228, 228, 228, 15,	 61,  62,  65,	66,	 280, 281, 282, 283, 284, 285, 286, 288, 290, 291, 292, 303, 202, 17,  247, 325, 38,  45,  46,	19,	 326, 17,  47,	19,	 228, 228, 228, 202, 200, 200, 202, 328, 200, 200, 200, 223, 224, 23,  17,	19,	 20,  233, 239, 228, 34,  228, 228, 15,	 230, 259,
									   260, 261, 322, 19,  230, 325, 341, 95,  325, 342, 230, 100, 325, 349, 102, 325, 351, 303, 356, 368, 356, 18,	 368, 228, 18,	303, 117, 325, 374, 303, 383, 384, 36,	228, 303, 322, 323, 370, 371, 370, 370, 10,	 391, 392, 393, 398, 383, 383, 149, 228, 279, 47,  67,	21,	 27,  16,  24,	303, 15,  22,  248, 249, 253, 228, 303, 311, 322, 326, 38,	324, 29,  30,  36,	37,	 39,  41,  42,	43,	 44,  57,  60,	75,	 76,  77,
									   78,	85,	 86,  87,  192, 193, 194, 195, 196, 197, 198, 199, 329, 183, 184, 185, 186, 187, 188, 189, 190, 191, 330, 228, 51,	52,	 332, 5,   12,	9,	 234, 235, 304, 19,	 228, 17,  262, 325, 17,  19,  326, 18,	 96,  325, 343, 230, 230, 19,  17,	19,	 356, 19,  101, 325, 367, 368, 118, 128, 129, 325, 385, 17,	 19,  16,  17,	19,	 19,  19,  17,	303, 394, 395, 134, 19,	 19,  322, 411, 18,	 294, 295, 296, 279, 293, 281,
									   16,	22,	 15,  17,  252, 325, 303, 202, 228, 326, 21,  21,  21,	15,	 22,  230, 263, 264, 326, 325, 369, 370, 18,  303, 19,	228, 19,  119, 120, 121, 375, 376, 384, 202, 371, 392, 385, 17,	 150, 15,  279, 297, 298, 299, 300, 322, 325, 24,  63,	287, 289, 303, 251, 321, 325, 249, 245, 22,	 17,  252, 260, 19,	 369, 109, 325, 359, 17,  135, 136, 396, 395, 322, 412, 17,	 17,  19,  45,	68,	 69,  301, 325, 299, 289, 250,
									   253, 14,	 251, 326, 19,	104, 112, 325, 364, 376, 66,  151, 413, 297, 298, 17,  23,	230, 265, 264, 18,	18,	 397, 398, 303, 152, 414, 19,  326, 23,	 17,  23,  303, 312, 365, 17,  303, 153, 415, 202, 230, 23,	 19,  17,  19,	398, 18,  154, 416, 253, 104, 325, 360, 312, 370, 18,  38,	110, 325, 361, 362, 19,	 303, 18,  113, 111, 325, 363, 19,	368, 38,  113, 19,	18,	 38,  368, 18,	19,	 368, 19};

#define yyerrok (yyerrstatus = 0)
#define yyclearin (yychar = YYEMPTY)
#define YYEMPTY (-2)
#define YYEOF 0

#define YYACCEPT goto yyacceptlab
#define YYABORT goto yyabortlab
#define YYERROR goto yyerrorlab

/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL goto yyerrlab
#if defined YYFAIL
/* This is here to suppress warnings from the GCC cpp's
   -Wunused-macros.  Normally we don't worry about that warning, but
   some users do, and we want to make it easy for users to remove
   YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING() (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
	do                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
		if (yychar == YYEMPTY)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
		{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
			yychar = (Token);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
			yylval = (Value);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
			YYPOPSTACK(yylen);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
			yystate = *yyssp;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
			goto yybackup;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
		}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
		else                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
		{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
			yyerror(YY_("syntax error: cannot back up"));                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
			YYERROR;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
		}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
	while (YYID(0))

/* Error token number */
#define YYTERROR 1
#define YYERRCODE 256

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
#define YY_LOCATION_PRINT(File, Loc) ((void)0)
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
#define YYLEX yylex(YYLEX_PARAM)
#else
#define YYLEX yylex()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

#ifndef YYFPRINTF
#include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#define YYFPRINTF fprintf
#endif

#define YYDPRINTF(Args)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
	do                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
	{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
		if (yydebug)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
			YYFPRINTF Args;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
	} while (YYID(0))

#define YY_SYMBOL_PRINT(Title, Type, Value, Location)                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
	do                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
	{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
		if (yydebug)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
		{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
			YYFPRINTF(stderr, "%s ", Title);                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
			yy_symbol_print(stderr, Type, Value);                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
			YYFPRINTF(stderr, "\n");                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
		}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
	} while (YYID(0))

/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
static void yy_symbol_value_print(FILE* yyoutput, int yytype, YYSTYPE const* const yyvaluep)
#else
static void yy_symbol_value_print(yyoutput, yytype, yyvaluep) FILE* yyoutput;
int yytype;
YYSTYPE const* const yyvaluep;
#endif
{
	FILE* yyo = yyoutput;
	YYUSE(yyo);
	if (!yyvaluep)
		return;
#ifdef YYPRINT
	if (yytype < YYNTOKENS)
		YYPRINT(yyoutput, yytoknum[yytype], *yyvaluep);
#else
	YYUSE(yyoutput);
#endif
	switch (yytype)
	{
		default:
			break;
	}
}

/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
static void yy_symbol_print(FILE* yyoutput, int yytype, YYSTYPE const* const yyvaluep)
#else
static void yy_symbol_print(yyoutput, yytype, yyvaluep) FILE* yyoutput;
int yytype;
YYSTYPE const* const yyvaluep;
#endif
{
	if (yytype < YYNTOKENS)
		YYFPRINTF(yyoutput, "token %s (", yytname[yytype]);
	else
		YYFPRINTF(yyoutput, "nterm %s (", yytname[yytype]);

	yy_symbol_value_print(yyoutput, yytype, yyvaluep);
	YYFPRINTF(yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
static void yy_stack_print(yytype_int16* yybottom, yytype_int16* yytop)
#else
static void yy_stack_print(yybottom, yytop) yytype_int16* yybottom;
yytype_int16* yytop;
#endif
{
	YYFPRINTF(stderr, "Stack now");
	for (; yybottom <= yytop; yybottom++)
	{
		int yybot = *yybottom;
		YYFPRINTF(stderr, " %d", yybot);
	}
	YYFPRINTF(stderr, "\n");
}

#define YY_STACK_PRINT(Bottom, Top)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
	do                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
	{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
		if (yydebug)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
			yy_stack_print((Bottom), (Top));                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
	} while (YYID(0))

/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
static void yy_reduce_print(YYSTYPE* yyvsp, int yyrule)
#else
static void yy_reduce_print(yyvsp, yyrule) YYSTYPE* yyvsp;
int yyrule;
#endif
{
	int yynrhs = yyr2[yyrule];
	int yyi;
	unsigned long int yylno = yyrline[yyrule];
	YYFPRINTF(stderr, "Reducing stack by rule %d (line %lu):\n", yyrule - 1, yylno);
	/* The symbols being reduced.  */
	for (yyi = 0; yyi < yynrhs; yyi++)
	{
		YYFPRINTF(stderr, "   $%d = ", yyi + 1);
		yy_symbol_print(stderr, yyrhs[yyprhs[yyrule] + yyi], &(yyvsp[(yyi + 1) - (yynrhs)]));
		YYFPRINTF(stderr, "\n");
	}
}

#define YY_REDUCE_PRINT(Rule)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
	do                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
	{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
		if (yydebug)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
			yy_reduce_print(yyvsp, Rule);                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
	} while (YYID(0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
#define YYDPRINTF(Args)
#define YY_SYMBOL_PRINT(Title, Type, Value, Location)
#define YY_STACK_PRINT(Bottom, Top)
#define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
#define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

#if YYERROR_VERBOSE

#ifndef yystrlen
#if defined __GLIBC__ && defined _STRING_H
#define yystrlen strlen
#else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T yystrlen(const char* yystr)
#else
static YYSIZE_T yystrlen(yystr) const char* yystr;
#endif
{
	YYSIZE_T yylen;
	for (yylen = 0; yystr[yylen]; yylen++)
		continue;
	return yylen;
}
#endif
#endif

#ifndef yystpcpy
#if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#define yystpcpy stpcpy
#else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
static char* yystpcpy(char* yydest, const char* yysrc)
#else
static char* yystpcpy(yydest, yysrc)
char* yydest;
const char* yysrc;
#endif
{
	char* yyd = yydest;
	const char* yys = yysrc;

	while ((*yyd++ = *yys++) != '\0')
		continue;

	return yyd - 1;
}
#endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T yytnamerr(char* yyres, const char* yystr)
{
	if (*yystr == '"')
	{
		YYSIZE_T yyn = 0;
		char const* yyp = yystr;

		for (;;)
			switch (*++yyp)
			{
				case '\'':
				case ',':
					goto do_not_strip_quotes;

				case '\\':
					if (*++yyp != '\\')
						goto do_not_strip_quotes;
					/* Fall through.  */
				default:
					if (yyres)
						yyres[yyn] = *yyp;
					yyn++;
					break;

				case '"':
					if (yyres)
						yyres[yyn] = '\0';
					return yyn;
			}
	do_not_strip_quotes:;
	}

	if (!yyres)
		return yystrlen(yystr);

	return yystpcpy(yyres, yystr) - yyres;
}
#endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int yysyntax_error(YYSIZE_T* yymsg_alloc, char** yymsg, yytype_int16* yyssp, int yytoken)
{
	YYSIZE_T yysize0 = yytnamerr(YY_NULL, yytname[yytoken]);
	YYSIZE_T yysize = yysize0;
	enum
	{
		YYERROR_VERBOSE_ARGS_MAXIMUM = 5
	};
	/* Internationalized format string. */
	const char* yyformat = YY_NULL;
	/* Arguments of yyformat. */
	char const* yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	/* Number of reported tokens (one for the "unexpected", one per
	   "expected"). */
	int yycount = 0;

	/* There are many possibilities here to consider:
	   - Assume YYFAIL is not used.  It's too flawed to consider.  See
		 <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
		 for details.  YYERROR is fine as it does not invoke this
		 function.
	   - If this state is a consistent state with a default action, then
		 the only way this function was invoked is if the default action
		 is an error action.  In that case, don't check for expected
		 tokens because there are none.
	   - The only way there can be no lookahead present (in yychar) is if
		 this state is a consistent state with a default action.  Thus,
		 detecting the absence of a lookahead is sufficient to determine
		 that there is no unexpected or expected token to report.  In that
		 case, just report a simple "syntax error".
	   - Don't assume there isn't a lookahead just because this state is a
		 consistent state with a default action.  There might have been a
		 previous inconsistent state, consistent state with a non-default
		 action, or user semantic action that manipulated yychar.
	   - Of course, the expected token list depends on states to have
		 correct lookahead information, and it depends on the parser not
		 to perform extra reductions after fetching a lookahead from the
		 scanner and before detecting a syntax error.  Thus, state merging
		 (from LALR or IELR) and default reductions corrupt the expected
		 token list.  However, the list is correct for canonical LR with
		 one exception: it will still contain any token that will not be
		 accepted due to an error action in a later state.
	*/
	if (yytoken != YYEMPTY)
	{
		int yyn = yypact[*yyssp];
		yyarg[yycount++] = yytname[yytoken];
		if (!yypact_value_is_default(yyn))
		{
			/* Start YYX at -YYN if negative to avoid negative indexes in
			   YYCHECK.  In other words, skip the first -YYN actions for
			   this state because they are default actions.  */
			int yyxbegin = yyn < 0 ? -yyn : 0;
			/* Stay within bounds of both yycheck and yytname.  */
			int yychecklim = YYLAST - yyn + 1;
			int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
			int yyx;

			for (yyx = yyxbegin; yyx < yyxend; ++yyx)
				if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR && !yytable_value_is_error(yytable[yyx + yyn]))
				{
					if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
					{
						yycount = 1;
						yysize = yysize0;
						break;
					}
					yyarg[yycount++] = yytname[yyx];
					{
						YYSIZE_T yysize1 = yysize + yytnamerr(YY_NULL, yytname[yyx]);
						if (!(yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
							return 2;
						yysize = yysize1;
					}
				}
		}
	}

	switch (yycount)
	{
#define YYCASE_(N, S)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
	case N:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
		yyformat = S;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
		break
		YYCASE_(0, YY_("syntax error"));
		YYCASE_(1, YY_("syntax error, unexpected %s"));
		YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
		YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
		YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
		YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
	}

	{
		YYSIZE_T yysize1 = yysize + yystrlen(yyformat);
		if (!(yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
			return 2;
		yysize = yysize1;
	}

	if (*yymsg_alloc < yysize)
	{
		*yymsg_alloc = 2 * yysize;
		if (!(yysize <= *yymsg_alloc && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
			*yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
		return 1;
	}

	/* Avoid sprintf, as that infringes on the user's name space.
	   Don't have undefined behavior even if the translation
	   produced a string with the wrong number of "%s"s.  */
	{
		char* yyp = *yymsg;
		int yyi = 0;
		while ((*yyp = *yyformat) != '\0')
			if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
			{
				yyp += yytnamerr(yyp, yyarg[yyi++]);
				yyformat += 2;
			}
			else
			{
				yyp++;
				yyformat++;
			}
	}
	return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
static void yydestruct(const char* yymsg, int yytype, YYSTYPE* yyvaluep)
#else
static void yydestruct(yymsg, yytype, yyvaluep) const char* yymsg;
int yytype;
YYSTYPE* yyvaluep;
#endif
{
	YYUSE(yyvaluep);

	if (!yymsg)
		yymsg = "Deleting";
	YY_SYMBOL_PRINT(yymsg, yytype, yyvaluep, yylocationp);

	switch (yytype)
	{

		default:
			break;
	}
}

/* The lookahead symbol.  */
int yychar;

#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
#define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
#define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);

/* Number of syntax errors so far.  */
int yynerrs;

/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
int yyparse(void* YYPARSE_PARAM)
#else
int yyparse(YYPARSE_PARAM) void* YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ || defined __cplusplus || defined _MSC_VER)
int yyparse(void)
#else
int yyparse()

#endif
#endif
{
	int yystate;
	/* Number of tokens to shift before error messages enabled.  */
	int yyerrstatus;

	/* The stacks and their tools:
	   `yyss': related to states.
	   `yyvs': related to semantic values.

	   Refer to the stacks through separate pointers, to allow yyoverflow
	   to reallocate them elsewhere.  */

	/* The state stack.  */
	yytype_int16 yyssa[YYINITDEPTH];
	yytype_int16* yyss;
	yytype_int16* yyssp;

	/* The semantic value stack.  */
	YYSTYPE yyvsa[YYINITDEPTH];
	YYSTYPE* yyvs;
	YYSTYPE* yyvsp;

	YYSIZE_T yystacksize;

	int yyn;
	int yyresult;
	/* Lookahead token as an internal (translated) token number.  */
	int yytoken = 0;
	/* The variables used to return semantic value and location from the
	   action routines.  */
	YYSTYPE yyval;

#if YYERROR_VERBOSE
	/* Buffer for error messages, and its allocated size.  */
	char yymsgbuf[128];
	char* yymsg = yymsgbuf;
	YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N) (yyvsp -= (N), yyssp -= (N))

	/* The number of symbols on the RHS of the reduced rule.
	   Keep to zero when no symbol should be popped.  */
	int yylen = 0;

	yyssp = yyss = yyssa;
	yyvsp = yyvs = yyvsa;
	yystacksize = YYINITDEPTH;

	YYDPRINTF((stderr, "Starting parse\n"));

	yystate = 0;
	yyerrstatus = 0;
	yynerrs = 0;
	yychar = YYEMPTY; /* Cause a token to be read.  */
	goto yysetstate;

	/*------------------------------------------------------------.
	| yynewstate -- Push a new state, which is found in yystate.  |
	`------------------------------------------------------------*/
yynewstate:
	/* In all cases, when you get here, the value and location stacks
	   have just been pushed.  So pushing a state here evens the stacks.  */
	yyssp++;

yysetstate:
	*yyssp = (yytype_int16)yystate;

	if (yyss + yystacksize - 1 <= yyssp)
	{
		/* Get the current used size of the three stacks, in elements.  */
		YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
		{
			/* Give user a chance to reallocate the stack.  Use copies of
			   these so that the &'s don't force the real ones into
			   memory.  */
			YYSTYPE* yyvs1 = yyvs;
			yytype_int16* yyss1 = yyss;

			/* Each stack pointer address is followed by the size of the
			   data in use in that stack, in bytes.  This used to be a
			   conditional around just the two extra args, but that might
			   be undefined if yyoverflow is a macro.  */
			yyoverflow(YY_("memory exhausted"), &yyss1, yysize * sizeof(*yyssp), &yyvs1, yysize * sizeof(*yyvsp), &yystacksize);

			yyss = yyss1;
			yyvs = yyvs1;
		}
#else /* no yyoverflow */
#ifndef YYSTACK_RELOCATE
		goto yyexhaustedlab;
#else
		/* Extend the stack our own way.  */
		if (YYMAXDEPTH <= yystacksize)
			goto yyexhaustedlab;
		yystacksize *= 2;
		if (YYMAXDEPTH < yystacksize)
			yystacksize = YYMAXDEPTH;

		{
			yytype_int16* yyss1 = yyss;
			union yyalloc* yyptr = (union yyalloc*)YYSTACK_ALLOC(YYSTACK_BYTES(yystacksize));
			if (!yyptr)
				goto yyexhaustedlab;
			YYSTACK_RELOCATE(yyss_alloc, yyss);
			YYSTACK_RELOCATE(yyvs_alloc, yyvs);
#undef YYSTACK_RELOCATE
			if (yyss1 != yyssa)
				YYSTACK_FREE(yyss1);
		}
#endif
#endif /* no yyoverflow */

		yyssp = yyss + yysize - 1;
		yyvsp = yyvs + yysize - 1;

		YYDPRINTF((stderr, "Stack size increased to %lu\n", (unsigned long int)yystacksize));

		if (yyss + yystacksize - 1 <= yyssp)
			YYABORT;
	}

	YYDPRINTF((stderr, "Entering state %d\n", yystate));

	if (yystate == YYFINAL)
		YYACCEPT;

	goto yybackup;

	/*-----------.
	| yybackup.  |
	`-----------*/
yybackup:

	/* Do appropriate processing given the current state.  Read a
	   lookahead token if we need one and don't already have one.  */

	/* First try to decide what to do without reference to lookahead token.  */
	yyn = yypact[yystate];
	if (yypact_value_is_default(yyn))
		goto yydefault;

	/* Not known => get a lookahead token if don't already have one.  */

	/* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
	if (yychar == YYEMPTY)
	{
		YYDPRINTF((stderr, "Reading a token: "));
		yychar = YYLEX;
	}

	if (yychar <= YYEOF)
	{
		yychar = yytoken = YYEOF;
		YYDPRINTF((stderr, "Now at end of input.\n"));
	}
	else
	{
		yytoken = YYTRANSLATE(yychar);
		YY_SYMBOL_PRINT("Next token is", yytoken, &yylval, &yylloc);
	}

	/* If the proper action on seeing token YYTOKEN is to reduce or to
	   detect an error, take that action.  */
	yyn += yytoken;
	if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
		goto yydefault;
	yyn = yytable[yyn];
	if (yyn <= 0)
	{
		if (yytable_value_is_error(yyn))
			goto yyerrlab;
		yyn = -yyn;
		goto yyreduce;
	}

	/* Count tokens shifted since error; after three, turn off error
	   status.  */
	if (yyerrstatus)
		yyerrstatus--;

	/* Shift the lookahead token.  */
	YY_SYMBOL_PRINT("Shifting", yytoken, &yylval, &yylloc);

	/* Discard the shifted token.  */
	yychar = YYEMPTY;

	yystate = yyn;
	YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
	*++yyvsp = yylval;
	YY_IGNORE_MAYBE_UNINITIALIZED_END

	goto yynewstate;

	/*-----------------------------------------------------------.
	| yydefault -- do the default action for the current state.  |
	`-----------------------------------------------------------*/
yydefault:
	yyn = yydefact[yystate];
	if (yyn == 0)
		goto yyerrlab;
	goto yyreduce;

	/*-----------------------------.
	| yyreduce -- Do a reduction.  |
	`-----------------------------*/
yyreduce:
	/* yyn is the number of a rule to reduce with.  */
	yylen = yyr2[yyn];

	/* If YYLEN is nonzero, implement the default value of the action:
	   `$$ = $1'.

	   Otherwise, the following line sets YYVAL to garbage.
	   This behavior is undocumented and Bison
	   users should not rely upon it.  Assigning to YYVAL
	   unconditionally makes the parser a bit smaller, and it avoids a
	   GCC warning that YYVAL may be used uninitialized.  */
	yyval = yyvsp[1 - yylen];

	YY_REDUCE_PRINT(yyn);
	switch (yyn)
	{
		case 2:
			/* Line 1792 of yacc.c  */
#line 481 "core\\parse-asn1.y"
			{
				(yyval.intVal) = myLineNoG;
			}
			break;

		case 3:
			/* Line 1792 of yacc.c  */
#line 486 "core\\parse-asn1.y"
			{
				modulePtrG->tagDefault = (yyvsp[(3) - (3)].intVal);
			}
			break;

		case 4:
			/* Line 1792 of yacc.c  */
#line 493 "core\\parse-asn1.y"
			{
				modulePtrG->modId = (yyvsp[(1) - (10)].moduleId);
				modulePtrG->namespaceToUse = (yyvsp[(8) - (10)].charPtr);

				/*
				 * Set exported flags in type/value defs as appropriate
				 */
				SetExports(modulePtrG, exportListG, exportsParsedG);

				/* clean up */

				/* Free Application tag list */
				FreeApplTags();

				/*
				 * Add values defined in any parsed object identifiers.
				 * Only the Module name and some macro oids have been parsed,
				 * the rest are just "{...}" strings at this point
				 * (they will be parsed in later)
		// REN -- 9/23/02
				 modulePtrG->valueDefs =
					 AsnListConcat (modulePtrG->valueDefs, oidElmtValDefsG);
				  */

				/*
				 * free list head only
	   // REN -- 9/23/02
				Free (oidElmtValDefsG);
				 */
			}
			break;

		case 5:
			/* Line 1792 of yacc.c  */
#line 526 "core\\parse-asn1.y"
			{
				(yyval.intVal) = EXPLICIT_TAGS;
			}
			break;

		case 6:
			/* Line 1792 of yacc.c  */
#line 527 "core\\parse-asn1.y"
			{
				(yyval.intVal) = IMPLICIT_TAGS;
			}
			break;

		case 7:
			/* Line 1792 of yacc.c  */
#line 529 "core\\parse-asn1.y"
			{
				/* default is EXPLICIT TAGS */
				(yyval.intVal) = EXPLICIT_TAGS;
			}
			break;

		case 8:
			/* Line 1792 of yacc.c  */
#line 537 "core\\parse-asn1.y"
			{
				(yyval.boolVal) = TRUE;
			}
			break;

		case 9:
			/* Line 1792 of yacc.c  */
#line 541 "core\\parse-asn1.y"
			{
				(yyval.boolVal) = FALSE;
			}
			break;

		case 10:
			/* Line 1792 of yacc.c  */
#line 550 "core\\parse-asn1.y"
			{
				(yyval.moduleId) = MT(ModuleId);
				(yyval.moduleId)->name = (yyvsp[(1) - (2)].charPtr);
				(yyval.moduleId)->oid = (yyvsp[(2) - (2)].oidPtr);
			}
			break;

		case 12:
			/* Line 1792 of yacc.c  */
#line 559 "core\\parse-asn1.y"
			{
				(yyval.oidPtr) = NULL;
			}
			break;

		case 13:
			/* Line 1792 of yacc.c  */
#line 563 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = (yyvsp[(3) - (3)].charPtr);
			}
			break;

		case 14:
			/* Line 1792 of yacc.c  */
#line 564 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = NULL;
			}
			break;

		case 17:
			/* Line 1792 of yacc.c  */
#line 575 "core\\parse-asn1.y"
			{
				/*
				 *  allows differentiation between "EXPORTS;"
				 *         (in which no exports allowed)
				 *  and when the EXPORTS symbol does not appear
				 *         (then all are exported)
				 */
				exportsParsedG = TRUE;
			}
			break;

		case 18:
			/* Line 1792 of yacc.c  */
#line 585 "core\\parse-asn1.y"
			{
				PARSE_ERROR();
				exportsParsedG = FALSE;
				exportListG = NULL;
				yyerrok;
			}
			break;

		case 19:
			/* Line 1792 of yacc.c  */
#line 591 "core\\parse-asn1.y"
			{
				exportsParsedG = FALSE;
			}
			break;

		case 20:
			/* Line 1792 of yacc.c  */
#line 595 "core\\parse-asn1.y"
			{
				exportListG = (yyvsp[(1) - (1)].exportList);
			}
			break;

		case 21:
			/* Line 1792 of yacc.c  */
#line 596 "core\\parse-asn1.y"
			{
				exportListG = NULL;
			}
			break;

		case 22:
			/* Line 1792 of yacc.c  */
#line 601 "core\\parse-asn1.y"
			{
				(yyval.exportList) = MT(ExportElmt);
				(yyval.exportList)->name = (yyvsp[(1) - (1)].charPtr);
				(yyval.exportList)->lineNo = myLineNoG;
				(yyval.exportList)->next = NULL;
			}
			break;

		case 23:
			/* Line 1792 of yacc.c  */
#line 608 "core\\parse-asn1.y"
			{
				(yyval.exportList) = MT(ExportElmt);
				(yyval.exportList)->name = (yyvsp[(4) - (4)].charPtr);
				(yyval.exportList)->next = (yyvsp[(1) - (4)].exportList);
				(yyval.exportList)->lineNo = (yyvsp[(3) - (4)].intVal);
			}
			break;

		case 25:
			/* Line 1792 of yacc.c  */
#line 619 "core\\parse-asn1.y"
			{
				PARSE_ERROR();
				yyerrok;
			}
			break;

		case 27:
			/* Line 1792 of yacc.c  */
#line 627 "core\\parse-asn1.y"
			{
				modulePtrG->imports = (yyvsp[(1) - (1)].importModuleListPtr);
			}
			break;

		case 29:
			/* Line 1792 of yacc.c  */
#line 633 "core\\parse-asn1.y"
			{
				APPEND((yyvsp[(2) - (2)].importModulePtr), (yyvsp[(1) - (2)].importModuleListPtr));
			}
			break;

		case 30:
			/* Line 1792 of yacc.c  */
#line 637 "core\\parse-asn1.y"
			{
				(yyval.importModuleListPtr) = NEWLIST();
				APPEND((yyvsp[(1) - (1)].importModulePtr), (yyval.importModuleListPtr));
			}
			break;

		case 31:
			/* Line 1792 of yacc.c  */
#line 645 "core\\parse-asn1.y"
			{
				(yyval.importModulePtr) = MT(ImportModule);
				(yyval.importModulePtr)->modId = (yyvsp[(4) - (4)].moduleId);
				(yyval.importModulePtr)->lineNo = (yyvsp[(3) - (4)].intVal);
				(yyval.importModulePtr)->importElmts = (yyvsp[(1) - (4)].importElmtListPtr);
			}
			break;

		case 32:
			/* Line 1792 of yacc.c  */
#line 656 "core\\parse-asn1.y"
			{
				ImportElmt* ie;

				ie = MT(ImportElmt);
				ie->name = (yyvsp[(3) - (3)].charPtr);
				ie->lineNo = myLineNoG;
				APPEND(ie, (yyvsp[(1) - (3)].importElmtListPtr));
				(yyval.importElmtListPtr) = (yyvsp[(1) - (3)].importElmtListPtr);
			}
			break;

		case 33:
			/* Line 1792 of yacc.c  */
#line 666 "core\\parse-asn1.y"
			{
				ImportElmt* ie;

				/* called for the first element only, so create list head */
				(yyval.importElmtListPtr) = NEWLIST();
				ie = MT(ImportElmt);
				ie->name = (yyvsp[(1) - (1)].charPtr);
				ie->lineNo = myLineNoG;
				APPEND(ie, (yyval.importElmtListPtr));
			}
			break;

		case 36:
			/* Line 1792 of yacc.c  */
#line 682 "core\\parse-asn1.y"
			{
				/*
				 * hack to make DefinedMacroNames "freeable"
				 * like idents and typeref
				 */
				size_t size = strlen((yyvsp[(1) - (1)].charPtr)) + 1;
				(yyval.charPtr) = Malloc(size);
				strcpy_s((yyval.charPtr), size, (yyvsp[(1) - (1)].charPtr));
			}
			break;

		case 41:
			/* Line 1792 of yacc.c  */
#line 703 "core\\parse-asn1.y"
			{
				PARSE_ERROR();
				yyerrok;
			}
			break;

		case 42:
			/* Line 1792 of yacc.c  */
#line 711 "core\\parse-asn1.y"
			{
				/*
				 * a macro may produce a null type
				 */
				if ((yyvsp[(1) - (1)].typeDefPtr) != NULL)
				{
					/*
					 * add to head of  type def list
					 */
					APPEND((yyvsp[(1) - (1)].typeDefPtr), modulePtrG->typeDefs);
				}
			}
			break;

		case 43:
			/* Line 1792 of yacc.c  */
#line 725 "core\\parse-asn1.y"
			{
				/*
				 * a macro may produce a null value
				 */
				if ((yyvsp[(1) - (1)].valueDefPtr) != NULL)
				{
					/*
					 * add to head of value def list
					 */
					APPEND((yyvsp[(1) - (1)].valueDefPtr), modulePtrG->valueDefs);
				}
			}
			break;

		case 44:
			/* Line 1792 of yacc.c  */
#line 737 "core\\parse-asn1.y"
			{
				LexBeginMacroDefContext();
			}
			break;

		case 45:
			/* Line 1792 of yacc.c  */
#line 739 "core\\parse-asn1.y"
			{
				TypeDef* tmpTypeDef;

				/*
				 *  LEXICAL TIE IN!!
				 * create macro type to eliminate import resolution
				 * errors msgs from other modules importing the macro.
				 * (hopefully) Only the import list will link with
				 * these type defs.
				 * keeps macro def around incase of future processing needs
				 *
				 * NOTE: MACRODEFBODY_SYM returns the macro def body with
				 * with "BEGIN" at the begininning and "END" at the end
				 */

				/*
				 * put lexical analyzer back in normal state
				 */
				/*  BEGIN (INITIAL);  */
				LexBeginInitialContext();

				tmpTypeDef = MT(TypeDef);
				SetupType(&tmpTypeDef->type, BASICTYPE_MACRODEF, (yyvsp[(4) - (6)].intVal));
				tmpTypeDef->definedName = (yyvsp[(1) - (6)].charPtr);

				/*
				 * keeps the macro def body
				 * (all text between & including the BEGIN and END)
				 * as a simple string - incase you want to fart around with
				 * it.
				 */
				tmpTypeDef->type->basicType->a.macroDef = (yyvsp[(6) - (6)].charPtr);

				/*
				 * put in type list
				 */
				APPEND(tmpTypeDef, modulePtrG->typeDefs);
			}
			break;

		case 46:
			/* Line 1792 of yacc.c  */
#line 779 "core\\parse-asn1.y"
			{
				TypeDef* tmpTypeDef;

				tmpTypeDef = MT(TypeDef);
				SetupType(&tmpTypeDef->type, BASICTYPE_MACRODEF, myLineNoG);
				tmpTypeDef->definedName = (yyvsp[(1) - (3)].charPtr);

				tmpTypeDef->type->basicType->a.macroDef = (yyvsp[(3) - (3)].charPtr);

				/*
				 * put in type list
				 */
				APPEND(tmpTypeDef, modulePtrG->typeDefs);
			}
			break;

		case 47:
			/* Line 1792 of yacc.c  */
#line 795 "core\\parse-asn1.y"
			{
				TypeDef* tmpTypeDef;

				tmpTypeDef = MT(TypeDef);
				SetupType(&tmpTypeDef->type, BASICTYPE_MACRODEF, myLineNoG);
				tmpTypeDef->definedName = (yyvsp[(1) - (5)].charPtr);

				size_t size = strlen((yyvsp[(3) - (5)].charPtr)) + strlen((yyvsp[(5) - (5)].charPtr)) + 2;
				tmpTypeDef->type->basicType->a.macroDef = (MyString)Malloc(size);

				strcpy_s(tmpTypeDef->type->basicType->a.macroDef, size, (yyvsp[(3) - (5)].charPtr));
				strcat_s(tmpTypeDef->type->basicType->a.macroDef, size, ".");
				strcat_s(tmpTypeDef->type->basicType->a.macroDef, size, (yyvsp[(5) - (5)].charPtr));

				/*
				 * put in type list
				 */
				APPEND(tmpTypeDef, modulePtrG->typeDefs);

				Free((yyvsp[(3) - (5)].charPtr));
				Free((yyvsp[(5) - (5)].charPtr));
			}
			break;

		case 50:
			/* Line 1792 of yacc.c  */
#line 830 "core\\parse-asn1.y"
			{
				/*
				 * a macro type may produce a null type
				 */
				if ((yyvsp[(5) - (6)].typePtr) != NULL)
				{
					(yyval.typeDefPtr) = MT(TypeDef);
					(yyval.typeDefPtr)->type = (yyvsp[(5) - (6)].typePtr);
					(yyval.typeDefPtr)->type->lineNo = (yyvsp[(4) - (6)].intVal);
					(yyval.typeDefPtr)->type->attrList = (yyvsp[(6) - (6)].directiveList);
					(yyval.typeDefPtr)->definedName = (yyvsp[(1) - (6)].charPtr);
					(yyval.typeDefPtr)->attrList = (yyvsp[(3) - (6)].directiveList);
				}
				else
					(yyval.typeDefPtr) = NULL;
			}
			break;

		case 51:
			/* Line 1792 of yacc.c  */
#line 851 "core\\parse-asn1.y"
			{
				/* allocate a Type with basic type of ImportTypeRef */
				SetupType(&(yyval.typePtr), BASICTYPE_IMPORTTYPEREF, (yyvsp[(3) - (4)].intVal));
				(yyval.typePtr)->basicType->a.importTypeRef = MT(TypeRef);
				(yyval.typePtr)->basicType->a.importTypeRef->typeName = (yyvsp[(4) - (4)].charPtr);
				(yyval.typePtr)->basicType->a.importTypeRef->moduleName = (yyvsp[(1) - (4)].charPtr);

				/* add entry to this module's import list */
				AddPrivateImportElmt(modulePtrG, (yyvsp[(4) - (4)].charPtr), (yyvsp[(1) - (4)].charPtr), (yyvsp[(3) - (4)].intVal));
			}
			break;

		case 52:
			/* Line 1792 of yacc.c  */
#line 865 "core\\parse-asn1.y"
			{
				(yyval.typePtr) = (yyvsp[(1) - (1)].typePtr);
			}
			break;

		case 53:
			/* Line 1792 of yacc.c  */
#line 867 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_LOCALTYPEREF, myLineNoG);
				(yyval.typePtr)->basicType->a.localTypeRef = MT(TypeRef);
				(yyval.typePtr)->basicType->a.localTypeRef->typeName = (yyvsp[(1) - (1)].charPtr);
			}
			break;

		case 76:
			/* Line 1792 of yacc.c  */
#line 903 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_OCTETSTRING, myLineNoG);
			}
			break;

		case 80:
			/* Line 1792 of yacc.c  */
#line 913 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = MT(NamedType);
				(yyval.namedTypePtr)->type = (yyvsp[(2) - (2)].typePtr);
				(yyval.namedTypePtr)->fieldName = (yyvsp[(1) - (2)].charPtr);
			}
			break;

		case 81:
			/* Line 1792 of yacc.c  */
#line 919 "core\\parse-asn1.y"
			{
				printf("Line %ld: Warning - 2002 ASN.1 syntax no longer supports un-named types!\n", myLineNoG);
				printf("	--Suggest using an identifier or may cause errors\n");
				(yyval.namedTypePtr) = MT(NamedType);
				(yyval.namedTypePtr)->type = (yyvsp[(1) - (1)].typePtr);
			}
			break;

		case 82:
			/* Line 1792 of yacc.c  */
#line 929 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_BOOLEAN, myLineNoG);
			}
			break;

		case 83:
			/* Line 1792 of yacc.c  */
#line 936 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_INTEGER, myLineNoG);
				(yyval.typePtr)->basicType->a.integer = NEWLIST(); /* empty list */
			}
			break;

		case 84:
			/* Line 1792 of yacc.c  */
#line 941 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_INTEGER, myLineNoG);
				(yyval.typePtr)->basicType->a.integer = (yyvsp[(3) - (4)].valueDefListPtr);
			}
			break;

		case 85:
			/* Line 1792 of yacc.c  */
#line 950 "core\\parse-asn1.y"
			{
				(yyval.valueDefListPtr) = NEWLIST();
				APPEND((yyvsp[(1) - (1)].valueDefPtr), (yyval.valueDefListPtr));
			}
			break;

		case 86:
			/* Line 1792 of yacc.c  */
#line 955 "core\\parse-asn1.y"
			{
				APPEND((yyvsp[(3) - (3)].valueDefPtr), (yyvsp[(1) - (3)].valueDefListPtr));
				(yyval.valueDefListPtr) = (yyvsp[(1) - (3)].valueDefListPtr);
			}
			break;

		case 87:
			/* Line 1792 of yacc.c  */
#line 963 "core\\parse-asn1.y"
			{
				(yyval.valueDefPtr) = MT(ValueDef);
				(yyval.valueDefPtr)->definedName = (yyvsp[(1) - (4)].charPtr);
				SetupValue(&(yyval.valueDefPtr)->value, BASICVALUE_INTEGER, myLineNoG);
				(yyval.valueDefPtr)->value->basicValue->a.integer = (yyvsp[(3) - (4)].intVal);
			}
			break;

		case 88:
			/* Line 1792 of yacc.c  */
#line 970 "core\\parse-asn1.y"
			{
				(yyval.valueDefPtr) = MT(ValueDef);
				(yyval.valueDefPtr)->definedName = (yyvsp[(1) - (4)].charPtr);
				(yyval.valueDefPtr)->value = (yyvsp[(3) - (4)].valuePtr);
			}
			break;

		case 89:
			/* Line 1792 of yacc.c  */
#line 979 "core\\parse-asn1.y"
			{
				(yyval.intVal) = (yyvsp[(1) - (1)].uintVal);
			}
			break;

		case 90:
			/* Line 1792 of yacc.c  */
#line 983 "core\\parse-asn1.y"
			{
			}
			break;

		case 91:
			/* Line 1792 of yacc.c  */
#line 986 "core\\parse-asn1.y"
			{
				(yyvsp[(2) - (2)].uintVal) = 0 - (yyvsp[(2) - (2)].uintVal);
				(yyval.intVal) = (yyvsp[(2) - (2)].uintVal);
			}
			break;

		case 92:
			/* Line 1792 of yacc.c  */
#line 991 "core\\parse-asn1.y"
			{
			}
			break;

		case 93:
			/* Line 1792 of yacc.c  */
#line 997 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_ENUMERATED, myLineNoG);
				(yyval.typePtr)->basicType->a.enumerated = (yyvsp[(3) - (4)].valueDefListPtr);
			}
			break;

		case 94:
			/* Line 1792 of yacc.c  */
#line 1008 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_REAL, myLineNoG);
			}
			break;

		case 95:
			/* Line 1792 of yacc.c  */
#line 1016 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_BITSTRING, myLineNoG);
				(yyval.typePtr)->basicType->a.bitString = NEWLIST(); /* empty list */
			}
			break;

		case 96:
			/* Line 1792 of yacc.c  */
#line 1021 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_BITSTRING, myLineNoG);
				(yyval.typePtr)->basicType->a.bitString = (yyvsp[(4) - (5)].valueDefListPtr);
			}
			break;

		case 98:
			/* Line 1792 of yacc.c  */
#line 1035 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_NULL, myLineNoG);
			}
			break;

		case 99:
			/* Line 1792 of yacc.c  */
#line 1042 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_EXTERNAL, myLineNoG);
			}
			break;

		case 100:
			/* Line 1792 of yacc.c  */
#line 1049 "core\\parse-asn1.y"
			{
				(yyval.intVal) = (yyvsp[(2) - (3)].intVal);
			}
			break;

		case 101:
			/* Line 1792 of yacc.c  */
#line 1054 "core\\parse-asn1.y"
			{
				NamedType* n;

				SetupType(&(yyval.typePtr), BASICTYPE_SEQUENCE, (yyvsp[(1) - (3)].intVal));

				if (AsnListCount((AsnList*)(yyvsp[(2) - (3)].namedTypeListPtr)) != 0)
				{
					n = (NamedType*)FIRST_LIST_ELMT((AsnList*)(yyvsp[(2) - (3)].namedTypeListPtr));
					n->type->lineNo = (yyvsp[(1) - (3)].intVal);
				}

				(yyval.typePtr)->basicType->a.sequence = (yyvsp[(2) - (3)].namedTypeListPtr);
			}
			break;

		case 102:
			/* Line 1792 of yacc.c  */
#line 1069 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_SEQUENCE, (yyvsp[(1) - (2)].intVal));

				/* set up empty list for SEQ with no elmts */
				(yyval.typePtr)->basicType->a.sequence = AsnListNew(sizeof(void*));
			}
			break;

		case 103:
			/* Line 1792 of yacc.c  */
#line 1085 "core\\parse-asn1.y"
			{
				NamedType* lastElmt;
				if ((yyvsp[(2) - (2)].directiveList) != NULL)
				{
					lastElmt = (NamedType*)LAST_LIST_ELMT((yyvsp[(1) - (2)].namedTypeListPtr));
					lastElmt->type->attrList = (yyvsp[(2) - (2)].directiveList);
				}

				(yyval.namedTypeListPtr) = (yyvsp[(1) - (2)].namedTypeListPtr);
			}
			break;

		case 105:
			/* Line 1792 of yacc.c  */
#line 1100 "core\\parse-asn1.y"
			{
				NamedType* firstElmt;

				if ((yyvsp[(3) - (5)].directiveList) != NULL)
				{
					firstElmt = (NamedType*)FIRST_LIST_ELMT((yyvsp[(5) - (5)].namedTypeListPtr));
					firstElmt->type->attrList = (yyvsp[(3) - (5)].directiveList);
				}

				PREPEND((yyvsp[(1) - (5)].namedTypePtr), (yyvsp[(5) - (5)].namedTypeListPtr));
				firstElmt = (NamedType*)FIRST_LIST_ELMT((yyvsp[(5) - (5)].namedTypeListPtr));
				firstElmt->type->lineNo = (yyvsp[(4) - (5)].intVal);
				(yyval.namedTypeListPtr) = (yyvsp[(5) - (5)].namedTypeListPtr);
			}
			break;

		case 106:
			/* Line 1792 of yacc.c  */
#line 1115 "core\\parse-asn1.y"
			{
				(yyval.namedTypeListPtr) = NEWLIST();
				PREPEND((yyvsp[(1) - (1)].namedTypePtr), (yyval.namedTypeListPtr));
			}
			break;

		case 107:
			/* Line 1792 of yacc.c  */
#line 1124 "core\\parse-asn1.y"
			{
				(yyval.namedTypeListPtr) = (yyvsp[(2) - (2)].namedTypeListPtr);
			}
			break;

		case 108:
			/* Line 1792 of yacc.c  */
#line 1131 "core\\parse-asn1.y"
			{
				NamedType* nt;
				(yyval.namedTypeListPtr) = NEWLIST();
				nt = MT(NamedType);
				SetupType(&nt->type, BASICTYPE_EXTENSION, myLineNoG);
				nt->type->extensionAddition = TRUE;
				APPEND(nt, (yyval.namedTypeListPtr));
			}
			break;

		case 109:
			/* Line 1792 of yacc.c  */
#line 1140 "core\\parse-asn1.y"
			{
				NamedType* nt;
				(yyval.namedTypeListPtr) = (yyvsp[(2) - (3)].namedTypeListPtr);
				nt = MT(NamedType);
				SetupType(&nt->type, BASICTYPE_EXTENSION, myLineNoG);
				nt->type->extensionAddition = TRUE;
				APPEND(nt, (yyval.namedTypeListPtr));
			}
			break;

		case 110:
			/* Line 1792 of yacc.c  */
#line 1150 "core\\parse-asn1.y"
			{
				NamedType* nt;
				(yyval.namedTypeListPtr) = NEWLIST();
				nt = MT(NamedType);
				SetupType(&nt->type, BASICTYPE_EXTENSION, myLineNoG);
				nt->type->extensionAddition = TRUE; /* PL: wasn't set */
				APPEND(nt, (yyval.namedTypeListPtr));
			}
			break;

		case 112:
			/* Line 1792 of yacc.c  */
#line 1163 "core\\parse-asn1.y"
			{
				CONCAT((yyvsp[(1) - (3)].namedTypeListPtr), (yyvsp[(3) - (3)].namedTypeListPtr));
				(yyval.namedTypeListPtr) = (yyvsp[(1) - (3)].namedTypeListPtr);
			}
			break;

		case 113:
			/* Line 1792 of yacc.c  */
#line 1171 "core\\parse-asn1.y"
			{
				(yyval.namedTypeListPtr) = NEWLIST();
				(yyvsp[(1) - (1)].namedTypePtr)->type->extensionAddition = TRUE;
				APPEND((yyvsp[(1) - (1)].namedTypePtr), (yyval.namedTypeListPtr));
			}
			break;

		case 114:
			/* Line 1792 of yacc.c  */
#line 1177 "core\\parse-asn1.y"
			{
				(yyval.namedTypeListPtr) = (yyvsp[(4) - (6)].namedTypeListPtr);
			}
			break;

		case 115:
			/* Line 1792 of yacc.c  */
#line 1184 "core\\parse-asn1.y"
			{
				NamedType* lastElmt;
				(yyvsp[(5) - (5)].namedTypePtr)->type->extensionAddition = TRUE;

				if ((yyvsp[(3) - (5)].directiveList) != NULL)
				{
					lastElmt = (NamedType*)LAST_LIST_ELMT((yyvsp[(1) - (5)].namedTypeListPtr));
					lastElmt->type->attrList = (yyvsp[(3) - (5)].directiveList);
				}

				APPEND((yyvsp[(5) - (5)].namedTypePtr), (yyvsp[(1) - (5)].namedTypeListPtr));
				lastElmt = (NamedType*)LAST_LIST_ELMT((yyvsp[(1) - (5)].namedTypeListPtr));
				lastElmt->type->lineNo = (yyvsp[(4) - (5)].intVal);
				(yyval.namedTypeListPtr) = (yyvsp[(1) - (5)].namedTypeListPtr);
			}
			break;

		case 116:
			/* Line 1792 of yacc.c  */
#line 1200 "core\\parse-asn1.y"
			{
				(yyval.namedTypeListPtr) = NEWLIST();
				(yyvsp[(1) - (1)].namedTypePtr)->type->extensionAddition = TRUE;
				APPEND((yyvsp[(1) - (1)].namedTypePtr), (yyval.namedTypeListPtr));
			}
			break;

		case 118:
			/* Line 1792 of yacc.c  */
#line 1210 "core\\parse-asn1.y"
			{
				(yyval.intVal) = -1;
			}
			break;

		case 122:
			/* Line 1792 of yacc.c  */
#line 1224 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = (yyvsp[(1) - (2)].namedTypePtr);
				(yyval.namedTypePtr)->type->optional = TRUE;
			}
			break;

		case 123:
			/* Line 1792 of yacc.c  */
#line 1229 "core\\parse-asn1.y"
			{
				/*
				 * this rules uses NamedValue instead of Value
				 * for the stupid choice value syntax (fieldname value)
				 * it should be like a set/seq value (ie with
				 * enclosing { }
				 */
				(yyval.namedTypePtr) = (yyvsp[(1) - (3)].namedTypePtr);
				(yyval.namedTypePtr)->type->defaultVal = (yyvsp[(3) - (3)].namedValuePtr);
				/*
				 * could link value to the elmt type here (done in link_types.c)
				 */
			}
			break;

		case 124:
			/* Line 1792 of yacc.c  */
#line 1243 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = MT(NamedType);
				SetupType(&(yyval.namedTypePtr)->type, BASICTYPE_COMPONENTSOF, myLineNoG);
				(yyval.namedTypePtr)->type->basicType->a.componentsOf = (yyvsp[(3) - (3)].typePtr);
			}
			break;

		case 125:
			/* Line 1792 of yacc.c  */
#line 1249 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = MT(NamedType);
				SetupType(&(yyval.namedTypePtr)->type, BASICTYPE_COMPONENTSOF, myLineNoG);
				(yyval.namedTypePtr)->fieldName = (yyvsp[(1) - (4)].charPtr);
				(yyval.namedTypePtr)->type->basicType->a.componentsOf = (yyvsp[(4) - (4)].typePtr);
			}
			break;

		case 126:
			/* Line 1792 of yacc.c  */
#line 1261 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_SEQUENCEOF, myLineNoG);

				/* grab line number from first elmt */
				if ((yyvsp[(3) - (3)].typePtr) != NULL)
					(yyval.typePtr)->lineNo = (yyvsp[(3) - (3)].typePtr)->lineNo - 1;

				(yyval.typePtr)->basicType->a.sequenceOf = (yyvsp[(3) - (3)].typePtr);
			}
			break;

		case 127:
			/* Line 1792 of yacc.c  */
#line 1273 "core\\parse-asn1.y"
			{
				(yyval.intVal) = (yyvsp[(2) - (3)].intVal);
			}
			break;

		case 128:
			/* Line 1792 of yacc.c  */
#line 1278 "core\\parse-asn1.y"
			{
				NamedType* n;

				SetupType(&(yyval.typePtr), BASICTYPE_SET, (yyvsp[(1) - (3)].intVal));

				/* reset first elmt's line number */
				if (AsnListCount((AsnList*)(yyvsp[(2) - (3)].namedTypeListPtr)) != 0)
				{
					n = (NamedType*)FIRST_LIST_ELMT((AsnList*)(yyvsp[(2) - (3)].namedTypeListPtr));
					n->type->lineNo = (yyvsp[(1) - (3)].intVal);
				}
				(yyval.typePtr)->basicType->a.set = (yyvsp[(2) - (3)].namedTypeListPtr);
			}
			break;

		case 129:
			/* Line 1792 of yacc.c  */
#line 1292 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_SET, (yyvsp[(1) - (2)].intVal));

				/* set up empty elmt list for SET */
				(yyval.typePtr)->basicType->a.set = AsnListNew(sizeof(void*));
			}
			break;

		case 130:
			/* Line 1792 of yacc.c  */
#line 1308 "core\\parse-asn1.y"
			{
				/* does not allow SET == SET OF ANY Abrev */
				SetupType(&(yyval.typePtr), BASICTYPE_SETOF, myLineNoG);

				if ((yyvsp[(3) - (3)].typePtr) != NULL)
					(yyval.typePtr)->lineNo = (yyvsp[(3) - (3)].typePtr)->lineNo;

				(yyval.typePtr)->basicType->a.setOf = (yyvsp[(3) - (3)].typePtr);
			}
			break;

		case 131:
			/* Line 1792 of yacc.c  */
#line 1322 "core\\parse-asn1.y"
			{
				NamedType* n;

				SetupType(&(yyval.typePtr), BASICTYPE_CHOICE, (yyvsp[(2) - (5)].intVal));

				(yyval.typePtr)->basicType->a.choice = (yyvsp[(4) - (5)].namedTypeListPtr);

				if (AsnListCount((yyvsp[(4) - (5)].namedTypeListPtr)) != 0)
				{
					n = (NamedType*)FIRST_LIST_ELMT((yyvsp[(4) - (5)].namedTypeListPtr));
					n->type->lineNo = (yyvsp[(2) - (5)].intVal);
				}
			}
			break;

		case 132:
			/* Line 1792 of yacc.c  */
#line 1339 "core\\parse-asn1.y"
			{
				NamedType* lastElmt;
				if ((yyvsp[(2) - (2)].directiveList) != NULL)
				{
					lastElmt = (NamedType*)LAST_LIST_ELMT((yyvsp[(1) - (2)].namedTypeListPtr));
					lastElmt->type->attrList = (yyvsp[(2) - (2)].directiveList);
				}
				(yyval.namedTypeListPtr) = (yyvsp[(1) - (2)].namedTypeListPtr);
			}
			break;

		case 133:
			/* Line 1792 of yacc.c  */
#line 1352 "core\\parse-asn1.y"
			{
				(yyval.namedTypeListPtr) = NEWLIST();
				PREPEND((yyvsp[(1) - (1)].namedTypePtr), (yyval.namedTypeListPtr));
			}
			break;

		case 134:
			/* Line 1792 of yacc.c  */
#line 1357 "core\\parse-asn1.y"
			{
				NamedType* firstElmt;

				if ((yyvsp[(3) - (4)].directiveList) != NULL)
				{
					firstElmt = (NamedType*)FIRST_LIST_ELMT((yyvsp[(4) - (4)].namedTypeListPtr));
					firstElmt->type->attrList = (yyvsp[(3) - (4)].directiveList);
				}
				PREPEND((yyvsp[(1) - (4)].namedTypePtr), (yyvsp[(4) - (4)].namedTypeListPtr));
				(yyval.namedTypeListPtr) = (yyvsp[(4) - (4)].namedTypeListPtr);
			}
			break;

		case 136:
			/* Line 1792 of yacc.c  */
#line 1373 "core\\parse-asn1.y"
			{
				(yyval.namedTypeListPtr) = (yyvsp[(2) - (2)].namedTypeListPtr);
			}
			break;

		case 137:
			/* Line 1792 of yacc.c  */
#line 1379 "core\\parse-asn1.y"
			{
				NamedType* nt;
				(yyval.namedTypeListPtr) = NEWLIST();
				nt = MT(NamedType);
				SetupType(&nt->type, BASICTYPE_EXTENSION, myLineNoG);
				nt->type->extensionAddition = TRUE;
				APPEND(nt, (yyval.namedTypeListPtr));
			}
			break;

		case 138:
			/* Line 1792 of yacc.c  */
#line 1388 "core\\parse-asn1.y"
			{
				(yyval.namedTypeListPtr) = (yyvsp[(2) - (3)].namedTypeListPtr);
			}
			break;

		case 139:
			/* Line 1792 of yacc.c  */
#line 1392 "core\\parse-asn1.y"
			{
				NamedType* nt;
				(yyval.namedTypeListPtr) = NEWLIST();
				nt = MT(NamedType);
				SetupType(&nt->type, BASICTYPE_EXTENSION, myLineNoG);
				nt->type->extensionAddition = TRUE;
				APPEND(nt, (yyval.namedTypeListPtr));
			}
			break;

		case 141:
			/* Line 1792 of yacc.c  */
#line 1405 "core\\parse-asn1.y"
			{
				CONCAT((yyvsp[(1) - (4)].namedTypeListPtr), (yyvsp[(4) - (4)].namedTypeListPtr));
				(yyval.namedTypeListPtr) = (yyvsp[(1) - (4)].namedTypeListPtr);
			}
			break;

		case 142:
			/* Line 1792 of yacc.c  */
#line 1413 "core\\parse-asn1.y"
			{
				(yyval.namedTypeListPtr) = NEWLIST();
				PREPEND((yyvsp[(1) - (1)].namedTypePtr), (yyval.namedTypeListPtr));
			}
			break;

		case 143:
			/* Line 1792 of yacc.c  */
#line 1418 "core\\parse-asn1.y"
			{
				(yyval.namedTypeListPtr) = (yyvsp[(4) - (6)].namedTypeListPtr);
			}
			break;

		case 144:
			/* Line 1792 of yacc.c  */
#line 1425 "core\\parse-asn1.y"
			{
				(yyval.namedTypeListPtr) = NEWLIST();
				PREPEND((yyvsp[(1) - (1)].namedTypePtr), (yyval.namedTypeListPtr));
			}
			break;

		case 145:
			/* Line 1792 of yacc.c  */
#line 1430 "core\\parse-asn1.y"
			{
				NamedType* firstElmt;

				if ((yyvsp[(1) - (3)].namedTypeListPtr) != NULL)
				{
					firstElmt = (NamedType*)FIRST_LIST_ELMT((yyvsp[(1) - (3)].namedTypeListPtr));
					firstElmt->type->attrList = (yyvsp[(1) - (3)].namedTypeListPtr);
				}
				PREPEND((yyvsp[(3) - (3)].namedTypePtr), (yyvsp[(1) - (3)].namedTypeListPtr));
				(yyval.namedTypeListPtr) = (yyvsp[(1) - (3)].namedTypeListPtr);
			}
			break;

		case 146:
			/* Line 1792 of yacc.c  */
#line 1445 "core\\parse-asn1.y"
			{
				/*
				 * the selection type should be replaced after
				 * link with actual type
				 */
				SetupType(&(yyval.typePtr), BASICTYPE_SELECTION, myLineNoG);

				(yyval.typePtr)->basicType->a.selection = MT(SelectionType);
				(yyval.typePtr)->basicType->a.selection->typeRef = (yyvsp[(3) - (3)].typePtr);
				(yyval.typePtr)->basicType->a.selection->fieldName = (yyvsp[(1) - (3)].charPtr);
			}
			break;

		case 147:
			/* Line 1792 of yacc.c  */
#line 1460 "core\\parse-asn1.y"
			{
				Tag* tag;

				/* remove next tag if any  && IMPLICIT_TAGS */
				if ((modulePtrG->tagDefault == IMPLICIT_TAGS) && ((yyvsp[(2) - (2)].typePtr)->tags != NULL) && !LIST_EMPTY((yyvsp[(2) - (2)].typePtr)->tags))
				{
					tag = (Tag*)FIRST_LIST_ELMT((yyvsp[(2) - (2)].typePtr)->tags); /* set curr to first */
					AsnListFirst((yyvsp[(2) - (2)].typePtr)->tags);				   /* set curr to first elmt */
					AsnListRemove((yyvsp[(2) - (2)].typePtr)->tags);			   /* remove first elmt */

					/*
					 * set implicit if implicitly tagged built in type (ie not ref)
					 * (this simplifies the module ASN.1 printer (print.c))
					 */
					if (tag->tclass == UNIV)
						(yyvsp[(2) - (2)].typePtr)->implicit = TRUE;

					Free(tag);
				}

				PREPEND((yyvsp[(1) - (2)].tagPtr), (yyvsp[(2) - (2)].typePtr)->tags);
				(yyval.typePtr) = (yyvsp[(2) - (2)].typePtr);
			}
			break;

		case 148:
			/* Line 1792 of yacc.c  */
#line 1485 "core\\parse-asn1.y"
			{
				Tag* tag;

				/* remove next tag if any */
				if (((yyvsp[(3) - (3)].typePtr)->tags != NULL) && !LIST_EMPTY((yyvsp[(3) - (3)].typePtr)->tags))
				{
					tag = (Tag*)FIRST_LIST_ELMT((yyvsp[(3) - (3)].typePtr)->tags); /* set curr to first */
					AsnListFirst((yyvsp[(3) - (3)].typePtr)->tags);				   /* set curr to first elmt */
					AsnListRemove((yyvsp[(3) - (3)].typePtr)->tags);			   /* remove first elmt */

					if (tag->tclass == UNIV)
						(yyvsp[(3) - (3)].typePtr)->implicit = TRUE;

					Free(tag);
				}

				/*
				 * must check after linking that implicitly tagged
				 * local/import type refs are not untagged choice/any etc
				 */
				else if (((yyvsp[(3) - (3)].typePtr)->basicType->choiceId == BASICTYPE_IMPORTTYPEREF) || ((yyvsp[(3) - (3)].typePtr)->basicType->choiceId == BASICTYPE_LOCALTYPEREF) || ((yyvsp[(3) - (3)].typePtr)->basicType->choiceId == BASICTYPE_SELECTION))
					(yyvsp[(3) - (3)].typePtr)->implicit = TRUE;

				/*
				 *  all other implicitly tagable types should have tags
				 *  to remove - if this else clause fires then it is
				 *  probably a CHOICE or ANY type
				 */
				else
				{
					PrintErrLoc(modulePtrG->asn1SrcFileName, (long)((yyvsp[(3) - (3)].typePtr)->lineNo));
					fprintf(errFileG, "ERROR - attempt to implicitly reference untagged type\n");
					smallErrG = 1;
				}

				PREPEND((yyvsp[(1) - (3)].tagPtr), (yyvsp[(3) - (3)].typePtr)->tags);
				(yyval.typePtr) = (yyvsp[(3) - (3)].typePtr);
			}
			break;

		case 149:
			/* Line 1792 of yacc.c  */
#line 1526 "core\\parse-asn1.y"
			{
				/* insert tag at head of list */
				(yyvsp[(1) - (3)].tagPtr)->_explicit = TRUE;
				PREPEND((yyvsp[(1) - (3)].tagPtr), (yyvsp[(3) - (3)].typePtr)->tags);
				(yyval.typePtr) = (yyvsp[(3) - (3)].typePtr);
			}
			break;

		case 150:
			/* Line 1792 of yacc.c  */
#line 1536 "core\\parse-asn1.y"
			{
				(yyval.tagPtr) = (yyvsp[(3) - (4)].tagPtr);
				(yyval.tagPtr)->tclass = (yyvsp[(2) - (4)].intVal);
				(yyval.tagPtr)->_explicit = FALSE; /* default to false */

				/*
				 *  keep track of APPLICATION Tags per module
				 *  should only be used once
				 */
				if (((yyvsp[(2) - (4)].intVal) == APPL) && ((yyval.tagPtr)->valueRef == NULL))
					PushApplTag((yyval.tagPtr)->code, myLineNoG);
			}
			break;

		case 151:
			/* Line 1792 of yacc.c  */
#line 1554 "core\\parse-asn1.y"
			{
				(yyval.tagPtr) = MT(Tag);
				(yyval.tagPtr)->code = (yyvsp[(1) - (1)].intVal);
			}
			break;

		case 152:
			/* Line 1792 of yacc.c  */
#line 1559 "core\\parse-asn1.y"
			{
				(yyval.tagPtr) = MT(Tag);
				(yyval.tagPtr)->code = NO_TAG_CODE;
				(yyval.tagPtr)->valueRef = (yyvsp[(1) - (1)].valuePtr);
			}
			break;

		case 153:
			/* Line 1792 of yacc.c  */
#line 1567 "core\\parse-asn1.y"
			{
				(yyval.intVal) = UNIV;
			}
			break;

		case 154:
			/* Line 1792 of yacc.c  */
#line 1568 "core\\parse-asn1.y"
			{
				(yyval.intVal) = APPL;
			}
			break;

		case 155:
			/* Line 1792 of yacc.c  */
#line 1569 "core\\parse-asn1.y"
			{
				(yyval.intVal) = PRIV;
			}
			break;

		case 156:
			/* Line 1792 of yacc.c  */
#line 1570 "core\\parse-asn1.y"
			{
				(yyval.intVal) = CNTX;
			}
			break;

		case 157:
			/* Line 1792 of yacc.c  */
#line 1576 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_ANY, myLineNoG);
			}
			break;

		case 158:
			/* Line 1792 of yacc.c  */
#line 1580 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_ANYDEFINEDBY, myLineNoG);
				(yyval.typePtr)->basicType->a.anyDefinedBy = MT(AnyDefinedByType);
				(yyval.typePtr)->basicType->a.anyDefinedBy->fieldName = (yyvsp[(4) - (4)].charPtr);
			}
			break;

		case 159:
			/* Line 1792 of yacc.c  */
#line 1589 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_OCTETCONTAINING, myLineNoG);
				(yyval.typePtr)->basicType->a.stringContaining = (yyvsp[(5) - (6)].typePtr);
			}
			break;

		case 160:
			/* Line 1792 of yacc.c  */
#line 1597 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_BITCONTAINING, myLineNoG);
				(yyval.typePtr)->basicType->a.stringContaining = (yyvsp[(4) - (4)].typePtr);
			}
			break;

		case 161:
			/* Line 1792 of yacc.c  */
#line 1605 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_OID, myLineNoG);
			}
			break;

		case 162:
			/* Line 1792 of yacc.c  */
#line 1612 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_RELATIVE_OID, myLineNoG);
			}
			break;

		case 163:
			/* Line 1792 of yacc.c  */
#line 1619 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_GENERALIZEDTIME, myLineNoG);
			}
			break;

		case 164:
			/* Line 1792 of yacc.c  */
#line 1623 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_UTCTIME, myLineNoG);
			}
			break;

		case 165:
			/* Line 1792 of yacc.c  */
#line 1627 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_OBJECTDESCRIPTOR, myLineNoG);
			}
			break;

		case 166:
			/* Line 1792 of yacc.c  */
#line 1634 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_NUMERIC_STR, myLineNoG);
			}
			break;

		case 167:
			/* Line 1792 of yacc.c  */
#line 1638 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_PRINTABLE_STR, myLineNoG);
			}
			break;

		case 168:
			/* Line 1792 of yacc.c  */
#line 1642 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_IA5_STR, myLineNoG);
			}
			break;

		case 169:
			/* Line 1792 of yacc.c  */
#line 1646 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_BMP_STR, myLineNoG);
			}
			break;

		case 170:
			/* Line 1792 of yacc.c  */
#line 1650 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_UNIVERSAL_STR, myLineNoG);
			}
			break;

		case 171:
			/* Line 1792 of yacc.c  */
#line 1654 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_UTF8_STR, myLineNoG);
			}
			break;

		case 172:
			/* Line 1792 of yacc.c  */
#line 1658 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_T61_STR, myLineNoG);
			}
			break;

		case 173:
			/* Line 1792 of yacc.c  */
#line 1662 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_T61_STR, myLineNoG);
			}
			break;

		case 174:
			/* Line 1792 of yacc.c  */
#line 1666 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_VIDEOTEX_STR, myLineNoG);
			}
			break;

		case 175:
			/* Line 1792 of yacc.c  */
#line 1670 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_VISIBLE_STR, myLineNoG);
			}
			break;

		case 176:
			/* Line 1792 of yacc.c  */
#line 1674 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_VISIBLE_STR, myLineNoG);
			}
			break;

		case 177:
			/* Line 1792 of yacc.c  */
#line 1678 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_GRAPHIC_STR, myLineNoG);
			}
			break;

		case 178:
			/* Line 1792 of yacc.c  */
#line 1682 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_GENERAL_STR, myLineNoG);
			}
			break;

		case 179:
			/* Line 1792 of yacc.c  */
#line 1689 "core\\parse-asn1.y"
			{
				/*
				 * append new subtype list to existing one (s) if any
				 * with AND relation
				 */
				AppendSubtype(&(yyvsp[(1) - (2)].typePtr)->subtypes, (yyvsp[(2) - (2)].subtypePtr), SUBTYPE_AND);
				(yyval.typePtr) = (yyvsp[(1) - (2)].typePtr);
			}
			break;

		case 180:
			/* Line 1792 of yacc.c  */
#line 1698 "core\\parse-asn1.y"
			{
				Subtype* s;

				SetupType(&(yyval.typePtr), BASICTYPE_SETOF, myLineNoG);
				(yyval.typePtr)->basicType->a.setOf = (yyvsp[(4) - (4)].typePtr);

				/* add size constraint */
				s = MT(Subtype);
				s->choiceId = SUBTYPE_SINGLE;
				s->a.single = (yyvsp[(2) - (4)].subtypeValuePtr);
				AppendSubtype(&(yyval.typePtr)->subtypes, s, SUBTYPE_AND);
			}
			break;

		case 181:
			/* Line 1792 of yacc.c  */
#line 1711 "core\\parse-asn1.y"
			{
				Subtype* s;

				SetupType(&(yyval.typePtr), BASICTYPE_SEQUENCEOF, myLineNoG);
				(yyval.typePtr)->basicType->a.sequenceOf = (yyvsp[(4) - (4)].typePtr);

				/* add size constraint */
				s = MT(Subtype);
				s->choiceId = SUBTYPE_SINGLE;
				s->a.single = (yyvsp[(2) - (4)].subtypeValuePtr);
				AppendSubtype(&(yyval.typePtr)->subtypes, s, SUBTYPE_AND);
			}
			break;

		case 182:
			/* Line 1792 of yacc.c  */
#line 1728 "core\\parse-asn1.y"
			{
				(yyval.subtypePtr) = (yyvsp[(2) - (3)].subtypePtr);
			}
			break;

		case 183:
			/* Line 1792 of yacc.c  */
#line 1735 "core\\parse-asn1.y"
			{
				(yyval.subtypePtr) = MT(Subtype);
				(yyval.subtypePtr)->choiceId = SUBTYPE_SINGLE;
				(yyval.subtypePtr)->a.single = (yyvsp[(1) - (1)].subtypeValuePtr);

				/*		Subtype *s;

						OR relation between all elmts of in	ValueSetList *

						$$ = MT (Subtype);
						$$->choiceId = SUBTYPE_OR;
						$$->a.or = NEWLIST();

						s = MT (Subtype);
						s->choiceId = SUBTYPE_SINGLE;
						s->a.single = $1;
						APPEND (s, $$->a.or);
						*/
			}
			break;

		case 184:
			/* Line 1792 of yacc.c  */
#line 1755 "core\\parse-asn1.y"
			{
				Subtype* s = MT(Subtype);
				s->choiceId = SUBTYPE_SINGLE;
				s->a.single = (yyvsp[(3) - (3)].subtypeValuePtr);

				if ((yyvsp[(1) - (3)].subtypePtr)->choiceId == SUBTYPE_OR)
					(yyval.subtypePtr) = (yyvsp[(1) - (3)].subtypePtr);
				else
				{
					(yyval.subtypePtr) = MT(Subtype);
					(yyval.subtypePtr)->choiceId = SUBTYPE_OR;
					APPEND((yyvsp[(1) - (3)].subtypePtr), (yyval.subtypePtr)->a.or);
				}

				APPEND(s, (yyval.subtypePtr)->a.or);
			}
			break;

		case 192:
			/* Line 1792 of yacc.c  */
#line 1786 "core\\parse-asn1.y"
			{
				(yyval.subtypeValuePtr) = MT(SubtypeValue);
			}
			break;

		case 193:
			/* Line 1792 of yacc.c  */
#line 1792 "core\\parse-asn1.y"
			{
				(yyval.subtypeValuePtr) = MT(SubtypeValue);
				(yyval.subtypeValuePtr)->choiceId = SUBTYPEVALUE_SINGLEVALUE;
				(yyval.subtypeValuePtr)->a.singleValue = (yyvsp[(1) - (1)].valuePtr);
			}
			break;

		case 194:
			/* Line 1792 of yacc.c  */
#line 1800 "core\\parse-asn1.y"
			{
				(yyval.subtypeValuePtr) = MT(SubtypeValue);
				(yyval.subtypeValuePtr)->choiceId = SUBTYPEVALUE_CONTAINED;
				(yyval.subtypeValuePtr)->a.contained = (yyvsp[(2) - (2)].typePtr);
			}
			break;

		case 195:
			/* Line 1792 of yacc.c  */
#line 1809 "core\\parse-asn1.y"
			{
				(yyval.subtypeValuePtr) = MT(SubtypeValue);
				(yyval.subtypeValuePtr)->choiceId = SUBTYPEVALUE_VALUERANGE;
				(yyval.subtypeValuePtr)->a.valueRange = MT(ValueRangeSubtype);
				(yyval.subtypeValuePtr)->a.valueRange->lowerEndValue = (yyvsp[(1) - (4)].valueRangeEndpointPtr);
				(yyval.subtypeValuePtr)->a.valueRange->upperEndValue = (yyvsp[(4) - (4)].valueRangeEndpointPtr);
			}
			break;

		case 196:
			/* Line 1792 of yacc.c  */
#line 1820 "core\\parse-asn1.y"
			{
				(yyval.valueRangeEndpointPtr) = MT(ValueRangeEndpoint);
				(yyval.valueRangeEndpointPtr)->valueInclusive = TRUE;
				(yyval.valueRangeEndpointPtr)->endValue = (yyvsp[(1) - (1)].valuePtr);
			}
			break;

		case 197:
			/* Line 1792 of yacc.c  */
#line 1826 "core\\parse-asn1.y"
			{
				(yyval.valueRangeEndpointPtr) = MT(ValueRangeEndpoint);
				(yyval.valueRangeEndpointPtr)->valueInclusive = FALSE;
				(yyval.valueRangeEndpointPtr)->endValue = (yyvsp[(1) - (2)].valuePtr);
			}
			break;

		case 198:
			/* Line 1792 of yacc.c  */
#line 1835 "core\\parse-asn1.y"
			{
				(yyval.valueRangeEndpointPtr) = MT(ValueRangeEndpoint);
				(yyval.valueRangeEndpointPtr)->valueInclusive = TRUE;
				(yyval.valueRangeEndpointPtr)->endValue = (yyvsp[(1) - (1)].valuePtr);
			}
			break;

		case 199:
			/* Line 1792 of yacc.c  */
#line 1841 "core\\parse-asn1.y"
			{
				(yyval.valueRangeEndpointPtr) = MT(ValueRangeEndpoint);
				(yyval.valueRangeEndpointPtr)->valueInclusive = FALSE;
				(yyval.valueRangeEndpointPtr)->endValue = (yyvsp[(2) - (2)].valuePtr);
			}
			break;

		case 200:
			/* Line 1792 of yacc.c  */
#line 1849 "core\\parse-asn1.y"
			{
				(yyval.valuePtr) = (yyvsp[(1) - (1)].valuePtr);
			}
			break;

		case 201:
			/* Line 1792 of yacc.c  */
#line 1851 "core\\parse-asn1.y"
			{
				SetupValue(&(yyval.valuePtr), BASICVALUE_SPECIALINTEGER, myLineNoG);
				(yyval.valuePtr)->basicValue->a.specialInteger = MIN_INT;
			}
			break;

		case 202:
			/* Line 1792 of yacc.c  */
#line 1858 "core\\parse-asn1.y"
			{
				(yyval.valuePtr) = (yyvsp[(1) - (1)].valuePtr);
			}
			break;

		case 203:
			/* Line 1792 of yacc.c  */
#line 1860 "core\\parse-asn1.y"
			{
				SetupValue(&(yyval.valuePtr), BASICVALUE_SPECIALINTEGER, myLineNoG);
				(yyval.valuePtr)->basicValue->a.specialInteger = MAX_INT;
			}
			break;

		case 204:
			/* Line 1792 of yacc.c  */
#line 1868 "core\\parse-asn1.y"
			{
				(yyval.subtypeValuePtr) = MT(SubtypeValue);
				(yyval.subtypeValuePtr)->choiceId = SUBTYPEVALUE_SIZECONSTRAINT;
				(yyval.subtypeValuePtr)->a.sizeConstraint = (yyvsp[(2) - (2)].subtypePtr);
			}
			break;

		case 205:
			/* Line 1792 of yacc.c  */
#line 1878 "core\\parse-asn1.y"
			{
				(yyval.subtypeValuePtr) = MT(SubtypeValue);
				(yyval.subtypeValuePtr)->choiceId = SUBTYPEVALUE_PERMITTEDALPHABET;
				(yyval.subtypeValuePtr)->a.permittedAlphabet = (yyvsp[(2) - (2)].subtypePtr);
			}
			break;

		case 206:
			/* Line 1792 of yacc.c  */
#line 1887 "core\\parse-asn1.y"
			{
				(yyval.subtypeValuePtr) = MT(SubtypeValue);
				(yyval.subtypeValuePtr)->choiceId = SUBTYPEVALUE_INNERSUBTYPE;
				(yyval.subtypeValuePtr)->a.innerSubtype = (yyvsp[(3) - (3)].innerSubtypePtr);
			}
			break;

		case 207:
			/* Line 1792 of yacc.c  */
#line 1893 "core\\parse-asn1.y"
			{
				(yyval.subtypeValuePtr) = MT(SubtypeValue);
				(yyval.subtypeValuePtr)->choiceId = SUBTYPEVALUE_INNERSUBTYPE;
				(yyval.subtypeValuePtr)->a.innerSubtype = (yyvsp[(3) - (3)].innerSubtypePtr);
			}
			break;

		case 208:
			/* Line 1792 of yacc.c  */
#line 1902 "core\\parse-asn1.y"
			{
				Constraint* constraint;

				/* this constrains the elmt of setof or seq of */
				(yyval.innerSubtypePtr) = MT(InnerSubtype);
				(yyval.innerSubtypePtr)->constraintType = SINGLE_CT;
				(yyval.innerSubtypePtr)->constraints = NEWLIST();
				constraint = MT(Constraint);
				APPEND(constraint, (yyval.innerSubtypePtr)->constraints);
				constraint->valueConstraints = (yyvsp[(1) - (1)].subtypePtr);
			}
			break;

		case 211:
			/* Line 1792 of yacc.c  */
#line 1922 "core\\parse-asn1.y"
			{
				(yyval.innerSubtypePtr) = MT(InnerSubtype);
				(yyval.innerSubtypePtr)->constraintType = FULL_CT;
				(yyval.innerSubtypePtr)->constraints = (yyvsp[(2) - (3)].constraintListPtr);
			}
			break;

		case 212:
			/* Line 1792 of yacc.c  */
#line 1931 "core\\parse-asn1.y"
			{
				(yyval.innerSubtypePtr) = MT(InnerSubtype);
				(yyval.innerSubtypePtr)->constraintType = PARTIAL_CT;
				(yyval.innerSubtypePtr)->constraints = (yyvsp[(4) - (5)].constraintListPtr);
			}
			break;

		case 213:
			/* Line 1792 of yacc.c  */
#line 1941 "core\\parse-asn1.y"
			{
				(yyval.constraintListPtr) = NEWLIST();
				APPEND((yyvsp[(1) - (1)].constraintPtr), (yyval.constraintListPtr));
			}
			break;

		case 214:
			/* Line 1792 of yacc.c  */
#line 1946 "core\\parse-asn1.y"
			{
				APPEND((yyvsp[(3) - (3)].constraintPtr), (yyvsp[(1) - (3)].constraintListPtr));
				(yyval.constraintListPtr) = (yyvsp[(1) - (3)].constraintListPtr);
			}
			break;

		case 215:
			/* Line 1792 of yacc.c  */
#line 1954 "core\\parse-asn1.y"
			{
				(yyval.constraintPtr) = (yyvsp[(2) - (2)].constraintPtr);
				(yyval.constraintPtr)->fieldRef = (yyvsp[(1) - (2)].charPtr);
			}
			break;

		case 217:
			/* Line 1792 of yacc.c  */
#line 1964 "core\\parse-asn1.y"
			{
				(yyval.constraintPtr) = MT(Constraint);
				(yyval.constraintPtr)->presenceConstraint = (yyvsp[(2) - (2)].intVal);
				(yyval.constraintPtr)->valueConstraints = (yyvsp[(1) - (2)].subtypePtr);
			}
			break;

		case 218:
			/* Line 1792 of yacc.c  */
#line 1972 "core\\parse-asn1.y"
			{
				(yyval.subtypePtr) = (yyvsp[(1) - (1)].subtypePtr);
			}
			break;

		case 219:
			/* Line 1792 of yacc.c  */
#line 1973 "core\\parse-asn1.y"
			{
				(yyval.subtypePtr) = NULL;
			}
			break;

		case 220:
			/* Line 1792 of yacc.c  */
#line 1977 "core\\parse-asn1.y"
			{
				(yyval.intVal) = PRESENT_CT;
			}
			break;

		case 221:
			/* Line 1792 of yacc.c  */
#line 1978 "core\\parse-asn1.y"
			{
				(yyval.intVal) = ABSENT_CT;
			}
			break;

		case 222:
			/* Line 1792 of yacc.c  */
#line 1979 "core\\parse-asn1.y"
			{
				(yyval.intVal) = EMPTY_CT;
			}
			break;

		case 223:
			/* Line 1792 of yacc.c  */
#line 1980 "core\\parse-asn1.y"
			{
				(yyval.intVal) = OPTIONAL_CT;
			}
			break;

		case 224:
			/* Line 1792 of yacc.c  */
#line 1994 "core\\parse-asn1.y"
			{
				(yyval.valueDefPtr) = MT(ValueDef);
				(yyval.valueDefPtr)->definedName = (yyvsp[(1) - (5)].charPtr);
				(yyval.valueDefPtr)->value = (yyvsp[(5) - (5)].valuePtr);
				(yyval.valueDefPtr)->value->lineNo = (yyvsp[(4) - (5)].intVal);
				(yyval.valueDefPtr)->value->type = (yyvsp[(2) - (5)].typePtr);
			}
			break;

		case 227:
			/* Line 1792 of yacc.c  */
#line 2010 "core\\parse-asn1.y"
			{
				(yyval.valuePtr) = (yyvsp[(1) - (1)].valuePtr);
			}
			break;

		case 228:
			/* Line 1792 of yacc.c  */
#line 2012 "core\\parse-asn1.y"
			{
				/*
				 * for parse, may be set to BASICVALUE_IMPORTEDTYPEREF
				 * by linker
				 */
				SetupValue(&(yyval.valuePtr), BASICVALUE_LOCALVALUEREF, myLineNoG);
				(yyval.valuePtr)->basicValue->a.localValueRef = MT(ValueRef);
				(yyval.valuePtr)->basicValue->a.localValueRef->valueName = (yyvsp[(1) - (1)].charPtr);
				(yyval.valuePtr)->valueType = BASICTYPE_UNKNOWN;
			}
			break;

		case 229:
			/* Line 1792 of yacc.c  */
#line 2026 "core\\parse-asn1.y"
			{
				/* Alloc value with basicValue of importValueRef */
				SetupValue(&(yyval.valuePtr), BASICVALUE_IMPORTVALUEREF, (yyvsp[(3) - (4)].intVal));
				(yyval.valuePtr)->valueType = BASICTYPE_UNKNOWN;
				(yyval.valuePtr)->basicValue->a.importValueRef = MT(ValueRef);
				(yyval.valuePtr)->basicValue->a.importValueRef->valueName = (yyvsp[(4) - (4)].charPtr);
				(yyval.valuePtr)->basicValue->a.importValueRef->moduleName = (yyvsp[(1) - (4)].charPtr);

				/* add entry to this module's import list */
				AddPrivateImportElmt(modulePtrG, (yyvsp[(4) - (4)].charPtr), (yyvsp[(1) - (4)].charPtr), (yyvsp[(3) - (4)].intVal));
			}
			break;

		case 233:
			/* Line 1792 of yacc.c  */
#line 2044 "core\\parse-asn1.y"
			{
				SetupValue(&(yyval.valuePtr), BASICVALUE_INTEGER, myLineNoG);
				(yyval.valuePtr)->valueType = BASICTYPE_UNKNOWN;
				(yyval.valuePtr)->basicValue->a.integer = (yyvsp[(1) - (1)].intVal);
			}
			break;

		case 234:
			/* Line 1792 of yacc.c  */
#line 2050 "core\\parse-asn1.y"
			{
				SetupValue(&(yyval.valuePtr), BASICVALUE_ASCIIHEX, myLineNoG);
				(yyval.valuePtr)->valueType = BASICTYPE_UNKNOWN;
				(yyval.valuePtr)->basicValue->a.asciiHex = MT(AsnOcts);
				(yyval.valuePtr)->basicValue->a.asciiHex->octs = (yyvsp[(1) - (1)].charPtr);
				(yyval.valuePtr)->basicValue->a.asciiHex->octetLen = strlen((yyvsp[(1) - (1)].charPtr));
			}
			break;

		case 235:
			/* Line 1792 of yacc.c  */
#line 2058 "core\\parse-asn1.y"
			{
				SetupValue(&(yyval.valuePtr), BASICVALUE_ASCIIBITSTRING, myLineNoG);
				(yyval.valuePtr)->valueType = BASICTYPE_UNKNOWN;
				(yyval.valuePtr)->basicValue->a.asciiBitString = MT(AsnOcts);
				(yyval.valuePtr)->basicValue->a.asciiBitString->octs = (yyvsp[(1) - (1)].charPtr);
				(yyval.valuePtr)->basicValue->a.asciiBitString->octetLen = strlen((yyvsp[(1) - (1)].charPtr));
			}
			break;

		case 236:
			/* Line 1792 of yacc.c  */
#line 2066 "core\\parse-asn1.y"
			{
				SetupValue(&(yyval.valuePtr), BASICVALUE_ASCIITEXT, myLineNoG);
				(yyval.valuePtr)->valueType = BASICTYPE_UNKNOWN;
				(yyval.valuePtr)->basicValue->a.asciiText = MT(AsnOcts);
				(yyval.valuePtr)->basicValue->a.asciiText->octs = (yyvsp[(1) - (1)].charPtr);
				(yyval.valuePtr)->basicValue->a.asciiText->octetLen = strlen((yyvsp[(1) - (1)].charPtr));
			}
			break;

		case 237:
			/* Line 1792 of yacc.c  */
#line 2073 "core\\parse-asn1.y"
			{
				LexBeginBraceBalContext();
			}
			break;

		case 238:
			/* Line 1792 of yacc.c  */
#line 2074 "core\\parse-asn1.y"
			{
				/*
				 *  LEXICAL TIE IN!!
				 * string returned by BRACEBAL_SYM has
				 * the $1 '{' prepended and includes everything
				 * upto and including '}' that balances $1
				 */
				LexBeginInitialContext();
				SetupValue(&(yyval.valuePtr), BASICVALUE_VALUENOTATION, myLineNoG);
				(yyval.valuePtr)->basicValue->a.valueNotation = MT(AsnOcts);
				(yyval.valuePtr)->basicValue->a.valueNotation->octs = (yyvsp[(3) - (3)].charPtr);
				(yyval.valuePtr)->basicValue->a.valueNotation->octetLen = strlen((yyvsp[(3) - (3)].charPtr));
				(yyval.valuePtr)->valueType = BASICTYPE_UNKNOWN;
			}
			break;

		case 239:
			/* Line 1792 of yacc.c  */
#line 2092 "core\\parse-asn1.y"
			{
				SetupValue(&(yyval.valuePtr), BASICVALUE_BOOLEAN, myLineNoG);
				(yyval.valuePtr)->valueType = BASICTYPE_UNKNOWN;
				(yyval.valuePtr)->basicValue->a.boolean = TRUE;
			}
			break;

		case 240:
			/* Line 1792 of yacc.c  */
#line 2098 "core\\parse-asn1.y"
			{
				SetupValue(&(yyval.valuePtr), BASICVALUE_BOOLEAN, myLineNoG);
				(yyval.valuePtr)->valueType = BASICTYPE_UNKNOWN;
				(yyval.valuePtr)->basicValue->a.boolean = FALSE;
			}
			break;

		case 241:
			/* Line 1792 of yacc.c  */
#line 2108 "core\\parse-asn1.y"
			{
				SetupValue(&(yyval.valuePtr), BASICVALUE_SPECIALREAL, myLineNoG);
				(yyval.valuePtr)->valueType = BASICTYPE_UNKNOWN;
				(yyval.valuePtr)->basicValue->a.specialReal = PLUS_INFINITY_REAL;
			}
			break;

		case 242:
			/* Line 1792 of yacc.c  */
#line 2114 "core\\parse-asn1.y"
			{
				SetupValue(&(yyval.valuePtr), BASICVALUE_SPECIALREAL, myLineNoG);
				(yyval.valuePtr)->valueType = BASICTYPE_UNKNOWN;
				(yyval.valuePtr)->basicValue->a.specialReal = MINUS_INFINITY_REAL;
			}
			break;

		case 243:
			/* Line 1792 of yacc.c  */
#line 2125 "core\\parse-asn1.y"
			{
				/* create a NULL value  */
				SetupValue(&(yyval.valuePtr), BASICVALUE_NULL, myLineNoG);
				(yyval.valuePtr)->valueType = BASICTYPE_UNKNOWN;
			}
			break;

		case 244:
			/* Line 1792 of yacc.c  */
#line 2135 "core\\parse-asn1.y"
			{
				(yyval.namedValuePtr) = MT(NamedValue);
				(yyval.namedValuePtr)->value = (yyvsp[(1) - (1)].valuePtr);
			}
			break;

		case 245:
			/* Line 1792 of yacc.c  */
#line 2140 "core\\parse-asn1.y"
			{
				(yyval.namedValuePtr) = MT(NamedValue);
				(yyval.namedValuePtr)->value = (yyvsp[(2) - (2)].valuePtr);
				(yyval.namedValuePtr)->fieldName = (yyvsp[(1) - (2)].charPtr);
			}
			break;

		case 246:
			/* Line 1792 of yacc.c  */
#line 2150 "core\\parse-asn1.y"
			{
				/*
				 * example OID setup
				 *
				 * for { ccitt foo (1) bar bell (bunt) 2 }
				 *
				 * ccitt
				 *   - arcnum is set to number from oid table (oid.c)
				 * foo (1)
				 *   - sets up a new value def foo defined as 1
				 *   - makes oid valueref a value ref to foo (doesn't link it tho)
				 * bar
				 *   - makes oid valueref a value ref to bar (doesn't link it tho)
				 * bell (bunt)
				 *   - sets up a new value def bell defined as a val ref to bunt
				 *   - makes oid valueref a value ref to bell (doesn't link it tho)
				 * 2
				 *  - arcnum is set to 2
				 */

				(yyval.oidPtr) = (yyvsp[(2) - (3)].oidPtr);
			}
			break;

		case 247:
			/* Line 1792 of yacc.c  */
#line 2177 "core\\parse-asn1.y"
			{
				OID* o;
				/* append component */
				for (o = (yyvsp[(1) - (2)].oidPtr); o->next != NULL; o = o->next)
					;
				o->next = (yyvsp[(2) - (2)].oidPtr);
				(yyval.oidPtr) = (yyvsp[(1) - (2)].oidPtr);
			}
			break;

		case 250:
			/* Line 1792 of yacc.c  */
#line 2193 "core\\parse-asn1.y"
			{
				Value* newVal;
				/*
				 * if the arcName is a defined arc name like
				 * ccitt or iso etc, fill in the arc number.
				 * otherwise make a value ref to that named value
				 */
				(yyval.oidPtr) = MT(OID);

				(yyval.oidPtr)->arcNum = OidArcNameToNum((yyvsp[(1) - (1)].charPtr));
				if ((yyval.oidPtr)->arcNum == NULL_OID_ARCNUM)
				{
					/* set up value ref to named value */
					SetupValue(&newVal, BASICVALUE_LOCALVALUEREF, myLineNoG);
					newVal->basicValue->a.localValueRef = MT(ValueRef);
					newVal->valueType = BASICTYPE_INTEGER;
					newVal->basicValue->a.localValueRef->valueName = (yyvsp[(1) - (1)].charPtr);
					(yyval.oidPtr)->valueRef = newVal;
				}
			}
			break;

		case 252:
			/* Line 1792 of yacc.c  */
#line 2219 "core\\parse-asn1.y"
			{
				(yyval.oidPtr) = MT(OID);
				(yyval.oidPtr)->arcNum = (yyvsp[(1) - (1)].intVal);
			}
			break;

		case 254:
			/* Line 1792 of yacc.c  */
#line 2232 "core\\parse-asn1.y"
			{
				/*		Value *newVal;	// REN -- 9/23/02 */

				(yyval.oidPtr) = (yyvsp[(3) - (4)].oidPtr);

				/* shared refs to named numbers name
		// REN -- 9/23/02 --
				SetupValue (&newVal, BASICVALUE_INTEGER, myLineNoG);
				newVal->basicValue->a.integer = $$->arcNum;
				newVal->valueType = BASICTYPE_INTEGER;
				AddNewValueDef (oidElmtValDefsG, $1, newVal);
				SetupValue (&newVal, BASICVALUE_LOCALVALUEREF, myLineNoG);
				newVal->basicValue->a.localValueRef = MT (ValueRef);
				newVal->basicValue->a.localValueRef->valueName = $1;

				$$->valueRef = newVal;
		*/
				Free((yyvsp[(1) - (4)].charPtr)); // REN -- 9/23/02
			}
			break;

		case 255:
			/* Line 1792 of yacc.c  */
#line 2252 "core\\parse-asn1.y"
			{
				/*		Value *newVal;	// REN -- 9/23/02 */

				/* shared refs to named numbers name */
				(yyval.oidPtr) = MT(OID);
				(yyval.oidPtr)->arcNum = NULL_OID_ARCNUM;

				/*		AddNewValueDef (oidElmtValDefsG, $1, $3);	// REN -- 9/23/02 */

				(yyval.oidPtr)->valueRef = (yyvsp[(3) - (4)].valuePtr);

				Free((yyvsp[(1) - (4)].charPtr)); // REN -- 9/23/02
			}
			break;

		case 259:
			/* Line 1792 of yacc.c  */
#line 2283 "core\\parse-asn1.y"
			{
				if ((yyvsp[(1) - (1)].uintVal) > 0x7FFFFFFF)
				{
					yyerror("Warning: number out of range");
					(yyval.intVal) = 0x7FFFFFFF;
				}
			}
			break;

		case 260:
			/* Line 1792 of yacc.c  */
#line 2290 "core\\parse-asn1.y"
			{
				yyerror("Warning: number out of range");
				(yyval.intVal) = 0x7FFFFFFF;
				/* modulePtrG->status = MOD_ERROR; */
			}
			break;

		case 266:
			/* Line 1792 of yacc.c  */
#line 2318 "core\\parse-asn1.y"
			{
				(yyval.directiveList) = NULL;
			}
			break;

		case 267:
			/* Line 1792 of yacc.c  */
#line 2323 "core\\parse-asn1.y"
			{
				(yyval.directiveList) = NEWLIST();
				APPEND((yyvsp[(1) - (1)].directivePtr), (yyval.directiveList));
			}
			break;

		case 268:
			/* Line 1792 of yacc.c  */
#line 2328 "core\\parse-asn1.y"
			{
				APPEND((yyvsp[(2) - (2)].directivePtr), (yyvsp[(1) - (2)].directiveList));
				(yyval.directiveList) = (yyvsp[(1) - (2)].directiveList);
			}
			break;

		case 269:
			/* Line 1792 of yacc.c  */
#line 2336 "core\\parse-asn1.y"
			{
				(yyval.directivePtr) = MT(SnaccDirective);
				(yyval.directivePtr)->type = ASN1_TypeID;
				(yyval.directivePtr)->value.asnTypeVal = (yyvsp[(3) - (3)].basicTypeChoiceIdEnum);
			}
			break;

		case 270:
			/* Line 1792 of yacc.c  */
#line 2342 "core\\parse-asn1.y"
			{
				(yyval.directivePtr) = MT(SnaccDirective);
				(yyval.directivePtr)->type = C_TypeID;
				(yyval.directivePtr)->value.cTypeVal = (yyvsp[(3) - (3)].cTypeIdEnum);
			}
			break;

		case 271:
			/* Line 1792 of yacc.c  */
#line 2348 "core\\parse-asn1.y"
			{
				(yyval.directivePtr) = MT(SnaccDirective);
				(yyval.directivePtr)->type = (yyvsp[(1) - (3)].directiveEnum);
				(yyval.directivePtr)->value.boolVal = (yyvsp[(3) - (3)].boolVal);
			}
			break;

		case 272:
			/* Line 1792 of yacc.c  */
#line 2354 "core\\parse-asn1.y"
			{
				(yyval.directivePtr) = MT(SnaccDirective);
				(yyval.directivePtr)->type = (yyvsp[(1) - (3)].directiveEnum);
				(yyval.directivePtr)->value.stringVal = (yyvsp[(3) - (3)].charPtr);
			}
			break;

		case 273:
			/* Line 1792 of yacc.c  */
#line 2360 "core\\parse-asn1.y"
			{
				(yyval.directivePtr) = MT(SnaccDirective);
				(yyval.directivePtr)->type = (yyvsp[(1) - (3)].directiveEnum);
				(yyval.directivePtr)->value.integerVal = (yyvsp[(3) - (3)].uintVal);
			}
			break;

		case 274:
			/* Line 1792 of yacc.c  */
#line 2368 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_UNKNOWN;
			}
			break;

		case 275:
			/* Line 1792 of yacc.c  */
#line 2369 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_BOOLEAN;
			}
			break;

		case 276:
			/* Line 1792 of yacc.c  */
#line 2370 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_INTEGER;
			}
			break;

		case 277:
			/* Line 1792 of yacc.c  */
#line 2371 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_BITSTRING;
			}
			break;

		case 278:
			/* Line 1792 of yacc.c  */
#line 2372 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_OCTETSTRING;
			}
			break;

		case 279:
			/* Line 1792 of yacc.c  */
#line 2373 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_NULL;
			}
			break;

		case 280:
			/* Line 1792 of yacc.c  */
#line 2374 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_OID;
			}
			break;

		case 281:
			/* Line 1792 of yacc.c  */
#line 2375 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_REAL;
			}
			break;

		case 282:
			/* Line 1792 of yacc.c  */
#line 2376 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_ENUMERATED;
			}
			break;

		case 283:
			/* Line 1792 of yacc.c  */
#line 2377 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_SEQUENCE;
			}
			break;

		case 284:
			/* Line 1792 of yacc.c  */
#line 2378 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_SEQUENCEOF;
			}
			break;

		case 285:
			/* Line 1792 of yacc.c  */
#line 2379 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_SET;
			}
			break;

		case 286:
			/* Line 1792 of yacc.c  */
#line 2380 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_SETOF;
			}
			break;

		case 287:
			/* Line 1792 of yacc.c  */
#line 2381 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_CHOICE;
			}
			break;

		case 288:
			/* Line 1792 of yacc.c  */
#line 2382 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_ANY;
			}
			break;

		case 289:
			/* Line 1792 of yacc.c  */
#line 2383 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_ANYDEFINEDBY;
			}
			break;

		case 290:
			/* Line 1792 of yacc.c  */
#line 2384 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_LOCALTYPEREF;
			}
			break;

		case 291:
			/* Line 1792 of yacc.c  */
#line 2385 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_IMPORTTYPEREF;
			}
			break;

		case 292:
			/* Line 1792 of yacc.c  */
#line 2386 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_NUMERIC_STR;
			}
			break;

		case 293:
			/* Line 1792 of yacc.c  */
#line 2387 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_PRINTABLE_STR;
			}
			break;

		case 294:
			/* Line 1792 of yacc.c  */
#line 2388 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_IA5_STR;
			}
			break;

		case 295:
			/* Line 1792 of yacc.c  */
#line 2389 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_BMP_STR;
			}
			break;

		case 296:
			/* Line 1792 of yacc.c  */
#line 2390 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_UNIVERSAL_STR;
			}
			break;

		case 297:
			/* Line 1792 of yacc.c  */
#line 2391 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_UTF8_STR;
			}
			break;

		case 298:
			/* Line 1792 of yacc.c  */
#line 2392 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_T61_STR;
			}
			break;

		case 299:
			/* Line 1792 of yacc.c  */
#line 2393 "core\\parse-asn1.y"
			{
				(yyval.basicTypeChoiceIdEnum) = BASICTYPE_RELATIVE_OID;
			}
			break;

		case 300:
			/* Line 1792 of yacc.c  */
#line 2397 "core\\parse-asn1.y"
			{
				(yyval.cTypeIdEnum) = C_CHOICE;
			}
			break;

		case 301:
			/* Line 1792 of yacc.c  */
#line 2398 "core\\parse-asn1.y"
			{
				(yyval.cTypeIdEnum) = C_LIST;
			}
			break;

		case 302:
			/* Line 1792 of yacc.c  */
#line 2399 "core\\parse-asn1.y"
			{
				(yyval.cTypeIdEnum) = C_ANY;
			}
			break;

		case 303:
			/* Line 1792 of yacc.c  */
#line 2400 "core\\parse-asn1.y"
			{
				(yyval.cTypeIdEnum) = C_ANYDEFINEDBY;
			}
			break;

		case 304:
			/* Line 1792 of yacc.c  */
#line 2401 "core\\parse-asn1.y"
			{
				(yyval.cTypeIdEnum) = C_LIB;
			}
			break;

		case 305:
			/* Line 1792 of yacc.c  */
#line 2402 "core\\parse-asn1.y"
			{
				(yyval.cTypeIdEnum) = C_STRUCT;
			}
			break;

		case 306:
			/* Line 1792 of yacc.c  */
#line 2403 "core\\parse-asn1.y"
			{
				(yyval.cTypeIdEnum) = C_TYPEDEF;
			}
			break;

		case 307:
			/* Line 1792 of yacc.c  */
#line 2404 "core\\parse-asn1.y"
			{
				(yyval.cTypeIdEnum) = C_TYPEREF;
			}
			break;

		case 308:
			/* Line 1792 of yacc.c  */
#line 2405 "core\\parse-asn1.y"
			{
				(yyval.cTypeIdEnum) = C_NO_TYPE;
			}
			break;

		case 309:
			/* Line 1792 of yacc.c  */
#line 2409 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = IsPDU;
			}
			break;

		case 310:
			/* Line 1792 of yacc.c  */
#line 2410 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = IsPtr;
			}
			break;

		case 311:
			/* Line 1792 of yacc.c  */
#line 2411 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = IsPtrForTypeDef;
			}
			break;

		case 312:
			/* Line 1792 of yacc.c  */
#line 2412 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = IsPtrForTypeRef;
			}
			break;

		case 313:
			/* Line 1792 of yacc.c  */
#line 2413 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = IsPtrInChoice;
			}
			break;

		case 314:
			/* Line 1792 of yacc.c  */
#line 2414 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = IsPtrForOpt;
			}
			break;

		case 315:
			/* Line 1792 of yacc.c  */
#line 2415 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = IsEncDec;
			}
			break;

		case 316:
			/* Line 1792 of yacc.c  */
#line 2416 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = GenTypeDef;
			}
			break;

		case 317:
			/* Line 1792 of yacc.c  */
#line 2417 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = GenPrintRoutine;
			}
			break;

		case 318:
			/* Line 1792 of yacc.c  */
#line 2418 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = GenEncodeRoutine;
			}
			break;

		case 319:
			/* Line 1792 of yacc.c  */
#line 2419 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = GenDecodeRoutine;
			}
			break;

		case 320:
			/* Line 1792 of yacc.c  */
#line 2420 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = GenFreeRoutine;
			}
			break;

		case 321:
			/* Line 1792 of yacc.c  */
#line 2421 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = IsBigInt;
			}
			break;

		case 322:
			/* Line 1792 of yacc.c  */
#line 2425 "core\\parse-asn1.y"
			{
				(yyval.boolVal) = TRUE;
			}
			break;

		case 323:
			/* Line 1792 of yacc.c  */
#line 2426 "core\\parse-asn1.y"
			{
				(yyval.boolVal) = FALSE;
			}
			break;

		case 324:
			/* Line 1792 of yacc.c  */
#line 2430 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = C_TypeName;
			}
			break;

		case 325:
			/* Line 1792 of yacc.c  */
#line 2431 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = C_FieldName;
			}
			break;

		case 326:
			/* Line 1792 of yacc.c  */
#line 2432 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = OptionalTestRoutineName;
			}
			break;

		case 327:
			/* Line 1792 of yacc.c  */
#line 2433 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = DefaultFieldName;
			}
			break;

		case 328:
			/* Line 1792 of yacc.c  */
#line 2434 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = PrintRoutineName;
			}
			break;

		case 329:
			/* Line 1792 of yacc.c  */
#line 2435 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = EncodeRoutineName;
			}
			break;

		case 330:
			/* Line 1792 of yacc.c  */
#line 2436 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = DecodeRoutineName;
			}
			break;

		case 331:
			/* Line 1792 of yacc.c  */
#line 2437 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = FreeRoutineName;
			}
			break;

		case 332:
			/* Line 1792 of yacc.c  */
#line 2438 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = ChoiceIdSymbol;
			}
			break;

		case 333:
			/* Line 1792 of yacc.c  */
#line 2439 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = ChoiceIdEnumName;
			}
			break;

		case 334:
			/* Line 1792 of yacc.c  */
#line 2440 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = ChoiceIdEnumFieldName;
			}
			break;

		case 335:
			/* Line 1792 of yacc.c  */
#line 2444 "core\\parse-asn1.y"
			{
				(yyval.directiveEnum) = ChoiceIdValue;
			}
			break;

		case 361:
			/* Line 1792 of yacc.c  */
#line 2482 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "OPERATION";
			}
			break;

		case 362:
			/* Line 1792 of yacc.c  */
#line 2483 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "ERROR";
			}
			break;

		case 363:
			/* Line 1792 of yacc.c  */
#line 2484 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "BIND";
			}
			break;

		case 364:
			/* Line 1792 of yacc.c  */
#line 2485 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "UNBIND";
			}
			break;

		case 365:
			/* Line 1792 of yacc.c  */
#line 2486 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "APPLICATION-SERVICE-ELEMENT";
			}
			break;

		case 366:
			/* Line 1792 of yacc.c  */
#line 2487 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "APPLICATION-CONTEXT";
			}
			break;

		case 367:
			/* Line 1792 of yacc.c  */
#line 2488 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "EXTENSION";
			}
			break;

		case 368:
			/* Line 1792 of yacc.c  */
#line 2489 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "EXTENSIONS";
			}
			break;

		case 369:
			/* Line 1792 of yacc.c  */
#line 2490 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "EXTENSION-ATTRIBUTE";
			}
			break;

		case 370:
			/* Line 1792 of yacc.c  */
#line 2491 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "TOKEN";
			}
			break;

		case 371:
			/* Line 1792 of yacc.c  */
#line 2492 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "TOKEN-DATA";
			}
			break;

		case 372:
			/* Line 1792 of yacc.c  */
#line 2493 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "SECURITY-CATEGORY";
			}
			break;

		case 373:
			/* Line 1792 of yacc.c  */
#line 2494 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "OBJECT";
			}
			break;

		case 374:
			/* Line 1792 of yacc.c  */
#line 2495 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "PORT";
			}
			break;

		case 375:
			/* Line 1792 of yacc.c  */
#line 2496 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "REFINE";
			}
			break;

		case 376:
			/* Line 1792 of yacc.c  */
#line 2497 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "ABSTRACT-BIND";
			}
			break;

		case 377:
			/* Line 1792 of yacc.c  */
#line 2498 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "ABSTRACT-UNBIND";
			}
			break;

		case 378:
			/* Line 1792 of yacc.c  */
#line 2499 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "ABSTRACT-OPERATION";
			}
			break;

		case 379:
			/* Line 1792 of yacc.c  */
#line 2500 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "ABSTRACT-ERROR";
			}
			break;

		case 380:
			/* Line 1792 of yacc.c  */
#line 2501 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "ALGORITHM";
			}
			break;

		case 381:
			/* Line 1792 of yacc.c  */
#line 2502 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "ENCRYPTED";
			}
			break;

		case 382:
			/* Line 1792 of yacc.c  */
#line 2503 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "SIGNED";
			}
			break;

		case 383:
			/* Line 1792 of yacc.c  */
#line 2504 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "SIGNATURE";
			}
			break;

		case 384:
			/* Line 1792 of yacc.c  */
#line 2505 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "PROTECTED";
			}
			break;

		case 385:
			/* Line 1792 of yacc.c  */
#line 2506 "core\\parse-asn1.y"
			{
				(yyval.charPtr) = "OBJECT-TYPE";
			}
			break;

		case 386:
			/* Line 1792 of yacc.c  */
#line 2515 "core\\parse-asn1.y"
			{
				(yyval.typePtr) = (yyvsp[(2) - (2)].typePtr);
			}
			break;

		case 387:
			/* Line 1792 of yacc.c  */
#line 2520 "core\\parse-asn1.y"
			{
				RosOperationMacroType* r;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_ROSOPERATION, myLineNoG);
				r = (yyval.typePtr)->basicType->a.macroType->a.rosOperation = MT(RosOperationMacroType);
				r->arguments = (yyvsp[(1) - (4)].namedTypePtr);
				r->result = (yyvsp[(2) - (4)].namedTypePtr);
				r->errors = (yyvsp[(3) - (4)].typeOrValueListPtr);
				r->linkedOps = (yyvsp[(4) - (4)].typeOrValueListPtr);
			}
			break;

		case 388:
			/* Line 1792 of yacc.c  */
#line 2535 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = (yyvsp[(2) - (2)].namedTypePtr);
			}
			break;

		case 389:
			/* Line 1792 of yacc.c  */
#line 2536 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = NULL;
			}
			break;

		case 390:
			/* Line 1792 of yacc.c  */
#line 2540 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = (yyvsp[(2) - (2)].namedTypePtr);
			}
			break;

		case 391:
			/* Line 1792 of yacc.c  */
#line 2541 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = NULL;
			}
			break;

		case 393:
			/* Line 1792 of yacc.c  */
#line 2547 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = NULL;
			}
			break;

		case 394:
			/* Line 1792 of yacc.c  */
#line 2553 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = (yyvsp[(3) - (4)].typeOrValueListPtr);
			}
			break;

		case 395:
			/* Line 1792 of yacc.c  */
#line 2556 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = NULL;
			}
			break;

		case 396:
			/* Line 1792 of yacc.c  */
#line 2563 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = (yyvsp[(3) - (4)].typeOrValueListPtr);
			}
			break;

		case 397:
			/* Line 1792 of yacc.c  */
#line 2566 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = NULL;
			}
			break;

		case 398:
			/* Line 1792 of yacc.c  */
#line 2579 "core\\parse-asn1.y"
			{
				RosErrorMacroType* r;
				/*
				 * defines error macro type
				 */
				SetupMacroType(&(yyval.typePtr), MACROTYPE_ROSERROR, myLineNoG);
				r = (yyval.typePtr)->basicType->a.macroType->a.rosError = MT(RosErrorMacroType);
				r->parameter = (yyvsp[(2) - (2)].namedTypePtr);
			}
			break;

		case 399:
			/* Line 1792 of yacc.c  */
#line 2592 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = (yyvsp[(2) - (2)].namedTypePtr);
			}
			break;

		case 400:
			/* Line 1792 of yacc.c  */
#line 2593 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = NULL;
			}
			break;

		case 401:
			/* Line 1792 of yacc.c  */
#line 2603 "core\\parse-asn1.y"
			{
				RosBindMacroType* r;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_ROSBIND, myLineNoG);

				r = (yyval.typePtr)->basicType->a.macroType->a.rosBind = MT(RosBindMacroType);
				r->argument = (yyvsp[(2) - (4)].namedTypePtr);
				r->result = (yyvsp[(3) - (4)].namedTypePtr);
				r->error = (yyvsp[(4) - (4)].namedTypePtr);
			}
			break;

		case 402:
			/* Line 1792 of yacc.c  */
#line 2616 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = (yyvsp[(2) - (2)].namedTypePtr);
			}
			break;

		case 403:
			/* Line 1792 of yacc.c  */
#line 2617 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = NULL;
			}
			break;

		case 404:
			/* Line 1792 of yacc.c  */
#line 2622 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = (yyvsp[(2) - (2)].namedTypePtr);
			}
			break;

		case 405:
			/* Line 1792 of yacc.c  */
#line 2623 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = NULL;
			}
			break;

		case 406:
			/* Line 1792 of yacc.c  */
#line 2628 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = (yyvsp[(2) - (2)].namedTypePtr);
			}
			break;

		case 407:
			/* Line 1792 of yacc.c  */
#line 2629 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = NULL;
			}
			break;

		case 408:
			/* Line 1792 of yacc.c  */
#line 2639 "core\\parse-asn1.y"
			{
				RosBindMacroType* r;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_ROSUNBIND, myLineNoG);

				r = (yyval.typePtr)->basicType->a.macroType->a.rosUnbind = MT(RosBindMacroType);
				r->argument = (yyvsp[(2) - (4)].namedTypePtr);
				r->result = (yyvsp[(3) - (4)].namedTypePtr);
				r->error = (yyvsp[(4) - (4)].namedTypePtr);
			}
			break;

		case 409:
			/* Line 1792 of yacc.c  */
#line 2653 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = (yyvsp[(2) - (2)].namedTypePtr);
			}
			break;

		case 410:
			/* Line 1792 of yacc.c  */
#line 2654 "core\\parse-asn1.y"
			{
				(yyval.namedTypePtr) = NULL;
			}
			break;

		case 411:
			/* Line 1792 of yacc.c  */
#line 2664 "core\\parse-asn1.y"
			{
				RosAseMacroType* r;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_ROSASE, myLineNoG);
				r = (yyval.typePtr)->basicType->a.macroType->a.rosAse = MT(RosAseMacroType);
				r->operations = (yyvsp[(2) - (2)].valueListPtr);
			}
			break;

		case 412:
			/* Line 1792 of yacc.c  */
#line 2672 "core\\parse-asn1.y"
			{
				RosAseMacroType* r;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_ROSASE, myLineNoG);
				r = (yyval.typePtr)->basicType->a.macroType->a.rosAse = MT(RosAseMacroType);
				r->consumerInvokes = (yyvsp[(2) - (3)].valueListPtr);
				r->supplierInvokes = (yyvsp[(3) - (3)].valueListPtr);
			}
			break;

		case 413:
			/* Line 1792 of yacc.c  */
#line 2685 "core\\parse-asn1.y"
			{
				(yyval.valueListPtr) = (yyvsp[(3) - (4)].valueListPtr);
			}
			break;

		case 414:
			/* Line 1792 of yacc.c  */
#line 2693 "core\\parse-asn1.y"
			{
				(yyval.valueListPtr) = (yyvsp[(3) - (4)].valueListPtr);
			}
			break;

		case 415:
			/* Line 1792 of yacc.c  */
#line 2696 "core\\parse-asn1.y"
			{
				(yyval.valueListPtr) = NULL;
			}
			break;

		case 416:
			/* Line 1792 of yacc.c  */
#line 2702 "core\\parse-asn1.y"
			{
				(yyval.valueListPtr) = (yyvsp[(3) - (4)].valueListPtr);
			}
			break;

		case 417:
			/* Line 1792 of yacc.c  */
#line 2705 "core\\parse-asn1.y"
			{
				(yyval.valueListPtr) = NULL;
			}
			break;

		case 419:
			/* Line 1792 of yacc.c  */
#line 2725 "core\\parse-asn1.y"
			{
				RosAcMacroType* r;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_ROSAC, myLineNoG);
				r = (yyval.typePtr)->basicType->a.macroType->a.rosAc = MT(RosAcMacroType);
				r->nonRoElements = (yyvsp[(2) - (8)].valueListPtr);
				r->bindMacroType = (yyvsp[(4) - (8)].typePtr);
				r->unbindMacroType = (yyvsp[(6) - (8)].typePtr);
				r->remoteOperations = (yyvsp[(7) - (8)].valuePtr);
				r->operationsOf = rosAcSymmetricAsesG;
				r->initiatorConsumerOf = rosAcInitiatorConsumerOfG;
				r->responderConsumerOf = rosAcResponderConsumerOfG;
				r->abstractSyntaxes = (yyvsp[(8) - (8)].oidListPtr);
			}
			break;

		case 420:
			/* Line 1792 of yacc.c  */
#line 2744 "core\\parse-asn1.y"
			{
				(yyval.valueListPtr) = (yyvsp[(3) - (4)].valueListPtr);
			}
			break;

		case 421:
			/* Line 1792 of yacc.c  */
#line 2753 "core\\parse-asn1.y"
			{
				(yyval.valuePtr) = (yyvsp[(4) - (7)].valuePtr);
			}
			break;

		case 422:
			/* Line 1792 of yacc.c  */
#line 2757 "core\\parse-asn1.y"
			{
				(yyval.valuePtr) = NULL;
				rosAcSymmetricAsesG = NULL;
				rosAcInitiatorConsumerOfG = NULL;
				rosAcResponderConsumerOfG = NULL;
			}
			break;

		case 423:
			/* Line 1792 of yacc.c  */
#line 2767 "core\\parse-asn1.y"
			{
				rosAcSymmetricAsesG = (yyvsp[(4) - (5)].valueListPtr);
			}
			break;

		case 424:
			/* Line 1792 of yacc.c  */
#line 2770 "core\\parse-asn1.y"
			{
				rosAcSymmetricAsesG = NULL;
			}
			break;

		case 426:
			/* Line 1792 of yacc.c  */
#line 2779 "core\\parse-asn1.y"
			{
				rosAcInitiatorConsumerOfG = (yyvsp[(5) - (6)].valueListPtr);
			}
			break;

		case 427:
			/* Line 1792 of yacc.c  */
#line 2782 "core\\parse-asn1.y"
			{
				rosAcInitiatorConsumerOfG = NULL;
			}
			break;

		case 428:
			/* Line 1792 of yacc.c  */
#line 2787 "core\\parse-asn1.y"
			{
				rosAcResponderConsumerOfG = (yyvsp[(5) - (6)].valueListPtr);
			}
			break;

		case 429:
			/* Line 1792 of yacc.c  */
#line 2790 "core\\parse-asn1.y"
			{
				rosAcResponderConsumerOfG = NULL;
			}
			break;

		case 430:
			/* Line 1792 of yacc.c  */
#line 2795 "core\\parse-asn1.y"
			{
				(yyval.oidListPtr) = (yyvsp[(3) - (4)].oidListPtr);
			}
			break;

		case 431:
			/* Line 1792 of yacc.c  */
#line 2798 "core\\parse-asn1.y"
			{
				(yyval.oidListPtr) = NULL;
			}
			break;

		case 432:
			/* Line 1792 of yacc.c  */
#line 2804 "core\\parse-asn1.y"
			{
				(yyval.oidListPtr) = NEWLIST();
				APPEND((yyvsp[(1) - (1)].oidPtr), (yyval.oidListPtr));
			}
			break;

		case 433:
			/* Line 1792 of yacc.c  */
#line 2809 "core\\parse-asn1.y"
			{
				APPEND((yyvsp[(3) - (3)].oidPtr), (yyvsp[(1) - (3)].oidListPtr));
				(yyval.oidListPtr) = (yyvsp[(1) - (3)].oidListPtr);
			}
			break;

		case 434:
			/* Line 1792 of yacc.c  */
#line 2823 "core\\parse-asn1.y"
			{
				MtsasExtensionsMacroType* m;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_MTSASEXTENSIONS, myLineNoG);
				m = (yyval.typePtr)->basicType->a.macroType->a.mtsasExtensions = MT(MtsasExtensionsMacroType);
				m->extensions = (yyvsp[(5) - (6)].valueListPtr);
			}
			break;

		case 436:
			/* Line 1792 of yacc.c  */
#line 2836 "core\\parse-asn1.y"
			{
				(yyval.valueListPtr) = NULL;
			}
			break;

		case 437:
			/* Line 1792 of yacc.c  */
#line 2841 "core\\parse-asn1.y"
			{
				(yyval.valueListPtr) = NEWLIST();
				APPEND((yyvsp[(1) - (1)].valuePtr), (yyval.valueListPtr));
			}
			break;

		case 438:
			/* Line 1792 of yacc.c  */
#line 2846 "core\\parse-asn1.y"
			{
				APPEND((yyvsp[(3) - (3)].valuePtr), (yyvsp[(1) - (3)].valueListPtr));
				(yyval.valueListPtr) = (yyvsp[(1) - (3)].valueListPtr);
			}
			break;

		case 440:
			/* Line 1792 of yacc.c  */
#line 2854 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = NULL;
			}
			break;

		case 441:
			/* Line 1792 of yacc.c  */
#line 2859 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = NEWLIST();
				APPEND((yyvsp[(1) - (1)].typeOrValuePtr), (yyval.typeOrValueListPtr));
			}
			break;

		case 442:
			/* Line 1792 of yacc.c  */
#line 2864 "core\\parse-asn1.y"
			{
				APPEND((yyvsp[(3) - (3)].typeOrValuePtr), (yyvsp[(1) - (3)].typeOrValueListPtr));
				(yyval.typeOrValueListPtr) = (yyvsp[(1) - (3)].typeOrValueListPtr);
			}
			break;

		case 443:
			/* Line 1792 of yacc.c  */
#line 2872 "core\\parse-asn1.y"
			{
				(yyval.typeOrValuePtr) = MT(TypeOrValue);
				(yyval.typeOrValuePtr)->choiceId = TYPEORVALUE_TYPE;
				(yyval.typeOrValuePtr)->a.type = (yyvsp[(1) - (1)].typePtr);
			}
			break;

		case 444:
			/* Line 1792 of yacc.c  */
#line 2878 "core\\parse-asn1.y"
			{
				(yyval.typeOrValuePtr) = MT(TypeOrValue);
				(yyval.typeOrValuePtr)->choiceId = TYPEORVALUE_VALUE;
				(yyval.typeOrValuePtr)->a.value = (yyvsp[(1) - (1)].valuePtr);
			}
			break;

		case 445:
			/* Line 1792 of yacc.c  */
#line 2891 "core\\parse-asn1.y"
			{
				MtsasExtensionMacroType* m;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_MTSASEXTENSION, myLineNoG);
				m = (yyval.typePtr)->basicType->a.macroType->a.mtsasExtension = MT(MtsasExtensionMacroType);
				m->elmtType = (yyvsp[(2) - (4)].namedTypePtr);
				m->defaultValue = (yyvsp[(3) - (4)].valuePtr);
				m->criticalForSubmission = mtsasCriticalForSubmissionG;
				m->criticalForTransfer = mtsasCriticalForTransferG;
				m->criticalForDelivery = mtsasCriticalForDeliveryG;

				mtsasCriticalForSubmissionG = NULL; /* set up for next parse */
				mtsasCriticalForTransferG = NULL;
				mtsasCriticalForDeliveryG = NULL;
			}
			break;

		case 446:
			/* Line 1792 of yacc.c  */
#line 2908 "core\\parse-asn1.y"
			{
				SetupMacroType(&(yyval.typePtr), MACROTYPE_MTSASEXTENSION, myLineNoG);
				(yyval.typePtr)->basicType->a.macroType->a.mtsasExtension = MT(MtsasExtensionMacroType);
				/*
				 * all fields are NULL in the MtsasExtensionsMacroType
				 * for this production
				 */
			}
			break;

		case 447:
			/* Line 1792 of yacc.c  */
#line 2920 "core\\parse-asn1.y"
			{
				(yyval.valuePtr) = (yyvsp[(2) - (2)].valuePtr);
			}
			break;

		case 448:
			/* Line 1792 of yacc.c  */
#line 2921 "core\\parse-asn1.y"
			{
				(yyval.valuePtr) = NULL;
			}
			break;

		case 453:
			/* Line 1792 of yacc.c  */
#line 2937 "core\\parse-asn1.y"
			{
				mtsasCriticalForSubmissionG = MT(AsnBool);
				*mtsasCriticalForSubmissionG = TRUE;
			}
			break;

		case 454:
			/* Line 1792 of yacc.c  */
#line 2942 "core\\parse-asn1.y"
			{
				mtsasCriticalForTransferG = MT(AsnBool);
				*mtsasCriticalForTransferG = TRUE;
			}
			break;

		case 455:
			/* Line 1792 of yacc.c  */
#line 2947 "core\\parse-asn1.y"
			{
				mtsasCriticalForDeliveryG = MT(AsnBool);
				*mtsasCriticalForDeliveryG = TRUE;
			}
			break;

		case 456:
			/* Line 1792 of yacc.c  */
#line 2961 "core\\parse-asn1.y"
			{
				MtsasExtensionAttributeMacroType* m;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_MTSASEXTENSIONATTRIBUTE, myLineNoG);
				m = (yyval.typePtr)->basicType->a.macroType->a.mtsasExtensionAttribute = MT(MtsasExtensionAttributeMacroType);
				m->type = NULL;
			}
			break;

		case 457:
			/* Line 1792 of yacc.c  */
#line 2970 "core\\parse-asn1.y"
			{
				MtsasExtensionAttributeMacroType* m;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_MTSASEXTENSIONATTRIBUTE, myLineNoG);
				m = (yyval.typePtr)->basicType->a.macroType->a.mtsasExtensionAttribute = MT(MtsasExtensionAttributeMacroType);
				m->type = (yyvsp[(2) - (2)].typePtr);
			}
			break;

		case 458:
			/* Line 1792 of yacc.c  */
#line 2986 "core\\parse-asn1.y"
			{
				MtsasTokenMacroType* m;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_MTSASTOKEN, myLineNoG);
				m = (yyval.typePtr)->basicType->a.macroType->a.mtsasToken = MT(MtsasTokenMacroType);
				m->type = NULL;
			}
			break;

		case 459:
			/* Line 1792 of yacc.c  */
#line 2994 "core\\parse-asn1.y"
			{
				MtsasTokenMacroType* m;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_MTSASTOKEN, myLineNoG);
				m = (yyval.typePtr)->basicType->a.macroType->a.mtsasToken = MT(MtsasTokenMacroType);
				m->type = (yyvsp[(2) - (2)].typePtr);
			}
			break;

		case 460:
			/* Line 1792 of yacc.c  */
#line 3009 "core\\parse-asn1.y"
			{
				MtsasTokenDataMacroType* m;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_MTSASTOKENDATA, myLineNoG);
				m = (yyval.typePtr)->basicType->a.macroType->a.mtsasTokenData = MT(MtsasTokenDataMacroType);
				m->type = NULL;
			}
			break;

		case 461:
			/* Line 1792 of yacc.c  */
#line 3018 "core\\parse-asn1.y"
			{
				MtsasTokenDataMacroType* m;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_MTSASTOKENDATA, myLineNoG);
				m = (yyval.typePtr)->basicType->a.macroType->a.mtsasTokenData = MT(MtsasTokenDataMacroType);
				m->type = (yyvsp[(2) - (2)].typePtr);
			}
			break;

		case 462:
			/* Line 1792 of yacc.c  */
#line 3034 "core\\parse-asn1.y"
			{
				MtsasSecurityCategoryMacroType* m;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_MTSASSECURITYCATEGORY, myLineNoG);
				m = (yyval.typePtr)->basicType->a.macroType->a.mtsasSecurityCategory = MT(MtsasSecurityCategoryMacroType);
				m->type = NULL;
			}
			break;

		case 463:
			/* Line 1792 of yacc.c  */
#line 3043 "core\\parse-asn1.y"
			{
				MtsasSecurityCategoryMacroType* m;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_MTSASSECURITYCATEGORY, myLineNoG);
				m = (yyval.typePtr)->basicType->a.macroType->a.mtsasSecurityCategory = MT(MtsasSecurityCategoryMacroType);
				m->type = (yyvsp[(2) - (2)].typePtr);
			}
			break;

		case 464:
			/* Line 1792 of yacc.c  */
#line 3065 "core\\parse-asn1.y"
			{
				AsnObjectMacroType* a;
				SetupMacroType(&(yyval.typePtr), MACROTYPE_ASNOBJECT, myLineNoG);
				a = (yyval.typePtr)->basicType->a.macroType->a.asnObject = MT(AsnObjectMacroType);
				a->ports = (yyvsp[(2) - (2)].asnPortListPtr);
			}
			break;

		case 465:
			/* Line 1792 of yacc.c  */
#line 3075 "core\\parse-asn1.y"
			{
				(yyval.asnPortListPtr) = (yyvsp[(3) - (4)].asnPortListPtr);
			}
			break;

		case 466:
			/* Line 1792 of yacc.c  */
#line 3078 "core\\parse-asn1.y"
			{
				(yyval.asnPortListPtr) = NULL;
			}
			break;

		case 467:
			/* Line 1792 of yacc.c  */
#line 3083 "core\\parse-asn1.y"
			{
				(yyval.asnPortListPtr) = NEWLIST();
				APPEND((yyvsp[(1) - (1)].asnPortPtr), (yyval.asnPortListPtr));
			}
			break;

		case 468:
			/* Line 1792 of yacc.c  */
#line 3088 "core\\parse-asn1.y"
			{
				APPEND((yyvsp[(3) - (3)].asnPortPtr), (yyvsp[(1) - (3)].asnPortListPtr));
				(yyval.asnPortListPtr) = (yyvsp[(1) - (3)].asnPortListPtr);
			}
			break;

		case 469:
			/* Line 1792 of yacc.c  */
#line 3096 "core\\parse-asn1.y"
			{
				(yyval.asnPortPtr) = MT(AsnPort);
				(yyval.asnPortPtr)->portValue = (yyvsp[(1) - (2)].valuePtr);
				(yyval.asnPortPtr)->portType = (yyvsp[(2) - (2)].intVal);
			}
			break;

		case 470:
			/* Line 1792 of yacc.c  */
#line 3105 "core\\parse-asn1.y"
			{
				/* [C] consumer */
				(yyval.intVal) = CONSUMER_PORT;
			}
			break;

		case 471:
			/* Line 1792 of yacc.c  */
#line 3110 "core\\parse-asn1.y"
			{
				/* [S] supplier */
				(yyval.intVal) = SUPPLIER_PORT;
			}
			break;

		case 472:
			/* Line 1792 of yacc.c  */
#line 3115 "core\\parse-asn1.y"
			{
				/* symmetric */
				(yyval.intVal) = SYMMETRIC_PORT;
			}
			break;

		case 473:
			/* Line 1792 of yacc.c  */
#line 3128 "core\\parse-asn1.y"
			{
				AsnPortMacroType* a;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_ASNPORT, myLineNoG);
				a = (yyval.typePtr)->basicType->a.macroType->a.asnPort = MT(AsnPortMacroType);
				a->abstractOps = (yyvsp[(2) - (2)].typeOrValueListPtr);
				a->consumerInvokes = asnConsumerG;
				a->supplierInvokes = asnSupplierG;
			}
			break;

		case 474:
			/* Line 1792 of yacc.c  */
#line 3138 "core\\parse-asn1.y"
			{
				SetupMacroType(&(yyval.typePtr), MACROTYPE_ASNPORT, myLineNoG);
				(yyval.typePtr)->basicType->a.macroType->a.asnPort = MT(AsnPortMacroType);
			}
			break;

		case 475:
			/* Line 1792 of yacc.c  */
#line 3147 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = (yyvsp[(3) - (4)].typeOrValueListPtr);
			}
			break;

		case 476:
			/* Line 1792 of yacc.c  */
#line 3151 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = NULL;
				asnConsumerG = (yyvsp[(1) - (1)].typeOrValueListPtr);
				asnSupplierG = NULL;
			}
			break;

		case 477:
			/* Line 1792 of yacc.c  */
#line 3157 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = NULL;
				asnConsumerG = (yyvsp[(1) - (1)].typeOrValueListPtr);
				asnSupplierG = NULL;
			}
			break;

		case 478:
			/* Line 1792 of yacc.c  */
#line 3163 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = NULL;
				asnConsumerG = (yyvsp[(1) - (2)].typeOrValueListPtr);
				asnSupplierG = NULL;
			}
			break;

		case 479:
			/* Line 1792 of yacc.c  */
#line 3169 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = NULL;
				asnConsumerG = (yyvsp[(1) - (2)].typeOrValueListPtr);
				asnSupplierG = NULL;
			}
			break;

		case 480:
			/* Line 1792 of yacc.c  */
#line 3178 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = (yyvsp[(3) - (4)].typeOrValueListPtr);
			}
			break;

		case 481:
			/* Line 1792 of yacc.c  */
#line 3185 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = (yyvsp[(3) - (4)].typeOrValueListPtr);
			}
			break;

		case 482:
			/* Line 1792 of yacc.c  */
#line 3201 "core\\parse-asn1.y"
			{
				SetupType(&(yyval.typePtr), BASICTYPE_UNKNOWN, myLineNoG);
			}
			break;

		case 490:
			/* Line 1792 of yacc.c  */
#line 3227 "core\\parse-asn1.y"
			{
				(yyval.intVal) = 0; /* just to quiet yacc warning */
			}
			break;

		case 495:
			/* Line 1792 of yacc.c  */
#line 3245 "core\\parse-asn1.y"
			{
				(yyval.intVal) = 0; /* just to quiet yacc warning */
			}
			break;

		case 496:
			/* Line 1792 of yacc.c  */
#line 3258 "core\\parse-asn1.y"
			{
				AsnAbstractBindMacroType* a;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_ASNABSTRACTBIND, myLineNoG);
				a = (yyval.typePtr)->basicType->a.macroType->a.asnAbstractBind = MT(AsnAbstractBindMacroType);
				a->ports = (yyvsp[(2) - (2)].asnPortListPtr);
			}
			break;

		case 497:
			/* Line 1792 of yacc.c  */
#line 3267 "core\\parse-asn1.y"
			{
				AsnAbstractBindMacroType* a;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_ASNABSTRACTBIND, myLineNoG);
				a = (yyval.typePtr)->basicType->a.macroType->a.asnAbstractBind = MT(AsnAbstractBindMacroType);
				a->ports = (yyvsp[(2) - (3)].asnPortListPtr);
				a->type = (yyvsp[(3) - (3)].typePtr);
			}
			break;

		case 498:
			/* Line 1792 of yacc.c  */
#line 3280 "core\\parse-asn1.y"
			{
				(yyval.asnPortListPtr) = (yyvsp[(3) - (4)].asnPortListPtr);
			}
			break;

		case 499:
			/* Line 1792 of yacc.c  */
#line 3283 "core\\parse-asn1.y"
			{
				(yyval.asnPortListPtr) = NULL;
			}
			break;

		case 500:
			/* Line 1792 of yacc.c  */
#line 3294 "core\\parse-asn1.y"
			{
				AsnAbstractBindMacroType* a;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_ASNABSTRACTUNBIND, myLineNoG);
				a = (yyval.typePtr)->basicType->a.macroType->a.asnAbstractUnbind = MT(AsnAbstractBindMacroType);

				a->ports = (yyvsp[(2) - (2)].asnPortListPtr);
			}
			break;

		case 501:
			/* Line 1792 of yacc.c  */
#line 3304 "core\\parse-asn1.y"
			{
				AsnAbstractBindMacroType* a;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_ASNABSTRACTUNBIND, myLineNoG);
				a = (yyval.typePtr)->basicType->a.macroType->a.asnAbstractUnbind = MT(AsnAbstractBindMacroType);

				a->ports = (yyvsp[(2) - (3)].asnPortListPtr);
				a->type = (yyvsp[(3) - (3)].typePtr);
			}
			break;

		case 502:
			/* Line 1792 of yacc.c  */
#line 3318 "core\\parse-asn1.y"
			{
				(yyval.asnPortListPtr) = (yyvsp[(3) - (4)].asnPortListPtr);
			}
			break;

		case 503:
			/* Line 1792 of yacc.c  */
#line 3321 "core\\parse-asn1.y"
			{
				(yyval.asnPortListPtr) = NULL;
			}
			break;

		case 504:
			/* Line 1792 of yacc.c  */
#line 3331 "core\\parse-asn1.y"
			{
				(yyval.typePtr) = (yyvsp[(2) - (2)].typePtr);
				(yyvsp[(2) - (2)].typePtr)->basicType->a.macroType->choiceId = MACROTYPE_ASNABSTRACTOPERATION;
			}
			break;

		case 505:
			/* Line 1792 of yacc.c  */
#line 3343 "core\\parse-asn1.y"
			{
				SetupMacroType(&(yyval.typePtr), MACROTYPE_ASNABSTRACTERROR, myLineNoG);
				(yyval.typePtr)->basicType->a.macroType->a.asnAbstractError = MT(RosErrorMacroType);
				(yyval.typePtr)->basicType->a.macroType->a.asnAbstractError->parameter = (yyvsp[(2) - (2)].namedTypePtr);
			}
			break;

		case 506:
			/* Line 1792 of yacc.c  */
#line 3356 "core\\parse-asn1.y"
			{
				SetupMacroType(&(yyval.typePtr), MACROTYPE_AFALGORITHM, myLineNoG);
				(yyval.typePtr)->basicType->a.macroType->a.afAlgorithm = (yyvsp[(3) - (3)].typePtr);
			}
			break;

		case 507:
			/* Line 1792 of yacc.c  */
#line 3367 "core\\parse-asn1.y"
			{
				SetupMacroType(&(yyval.typePtr), MACROTYPE_AFENCRYPTED, myLineNoG);
				(yyval.typePtr)->basicType->a.macroType->a.afEncrypted = (yyvsp[(2) - (2)].typePtr);
			}
			break;

		case 508:
			/* Line 1792 of yacc.c  */
#line 3379 "core\\parse-asn1.y"
			{
				SetupMacroType(&(yyval.typePtr), MACROTYPE_AFSIGNED, myLineNoG);
				(yyval.typePtr)->basicType->a.macroType->a.afSigned = (yyvsp[(2) - (2)].typePtr);
			}
			break;

		case 509:
			/* Line 1792 of yacc.c  */
#line 3390 "core\\parse-asn1.y"
			{
				SetupMacroType(&(yyval.typePtr), MACROTYPE_AFSIGNATURE, myLineNoG);
				(yyval.typePtr)->basicType->a.macroType->a.afSignature = (yyvsp[(2) - (2)].typePtr);
			}
			break;

		case 510:
			/* Line 1792 of yacc.c  */
#line 3404 "core\\parse-asn1.y"
			{
				SetupMacroType(&(yyval.typePtr), MACROTYPE_AFPROTECTED, myLineNoG);
				(yyval.typePtr)->basicType->a.macroType->a.afProtected = (yyvsp[(2) - (2)].typePtr);
			}
			break;

		case 511:
			/* Line 1792 of yacc.c  */
#line 3421 "core\\parse-asn1.y"
			{
				SnmpObjectTypeMacroType* s;

				SetupMacroType(&(yyval.typePtr), MACROTYPE_SNMPOBJECTTYPE, myLineNoG);
				s = (yyval.typePtr)->basicType->a.macroType->a.snmpObjectType = MT(SnmpObjectTypeMacroType);

				s->syntax = (yyvsp[(3) - (11)].typePtr);
				s->access = (yyvsp[(5) - (11)].intVal);
				s->status = (yyvsp[(7) - (11)].intVal);
				s->description = (yyvsp[(8) - (11)].valuePtr);
				s->reference = (yyvsp[(9) - (11)].valuePtr);
				s->index = (yyvsp[(10) - (11)].typeOrValueListPtr);
				s->defVal = (yyvsp[(11) - (11)].valuePtr);
			}
			break;

		case 512:
			/* Line 1792 of yacc.c  */
#line 3440 "core\\parse-asn1.y"
			{
				if (strcmp((yyvsp[(1) - (1)].charPtr), "read-only") == 0)
					(yyval.intVal) = SNMP_READ_ONLY;
				else if (strcmp((yyvsp[(1) - (1)].charPtr), "read-write") == 0)
					(yyval.intVal) = SNMP_READ_WRITE;
				else if (strcmp((yyvsp[(1) - (1)].charPtr), "write-only") == 0)
					(yyval.intVal) = SNMP_WRITE_ONLY;
				else if (strcmp((yyvsp[(1) - (1)].charPtr), "not-accessible") == 0)
					(yyval.intVal) = SNMP_NOT_ACCESSIBLE;
				else
				{
					yyerror("ACCESS field of SNMP OBJECT-TYPE MACRO can only be one of \"read-write\", \"write-only\" or \"not-accessible\"");
					(yyval.intVal) = -1;
					modulePtrG->status = MOD_ERROR;
				}
				Free((yyvsp[(1) - (1)].charPtr));
			}
			break;

		case 513:
			/* Line 1792 of yacc.c  */
#line 3462 "core\\parse-asn1.y"
			{
				if (strcmp((yyvsp[(1) - (1)].charPtr), "mandatory") == 0)
					(yyval.intVal) = SNMP_MANDATORY;
				else if (strcmp((yyvsp[(1) - (1)].charPtr), "optional") == 0)
					(yyval.intVal) = SNMP_OPTIONAL;
				else if (strcmp((yyvsp[(1) - (1)].charPtr), "obsolete") == 0)
					(yyval.intVal) = SNMP_OBSOLETE;
				else if (strcmp((yyvsp[(1) - (1)].charPtr), "deprecated") == 0)
					(yyval.intVal) = SNMP_DEPRECATED;
				else
				{
					yyerror("STATUS field of SNMP OBJECT-TYPE MACRO can only be one of \"optional\", \"obsolete\" or \"deprecated\"");
					(yyval.intVal) = -1;
					modulePtrG->status = MOD_ERROR;
				}
				Free((yyvsp[(1) - (1)].charPtr));
			}
			break;

		case 514:
			/* Line 1792 of yacc.c  */
#line 3482 "core\\parse-asn1.y"
			{
				(yyval.valuePtr) = (yyvsp[(2) - (2)].valuePtr);
			}
			break;

		case 515:
			/* Line 1792 of yacc.c  */
#line 3483 "core\\parse-asn1.y"
			{
				(yyval.valuePtr) = NULL;
			}
			break;

		case 516:
			/* Line 1792 of yacc.c  */
#line 3487 "core\\parse-asn1.y"
			{
				(yyval.valuePtr) = (yyvsp[(2) - (2)].valuePtr);
			}
			break;

		case 517:
			/* Line 1792 of yacc.c  */
#line 3488 "core\\parse-asn1.y"
			{
				(yyval.valuePtr) = NULL;
			}
			break;

		case 518:
			/* Line 1792 of yacc.c  */
#line 3493 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = (yyvsp[(3) - (4)].typeOrValueListPtr);
			}
			break;

		case 519:
			/* Line 1792 of yacc.c  */
#line 3496 "core\\parse-asn1.y"
			{
				(yyval.typeOrValueListPtr) = NULL;
			}
			break;

		case 520:
			/* Line 1792 of yacc.c  */
#line 3501 "core\\parse-asn1.y"
			{
				(yyval.valuePtr) = (yyvsp[(3) - (4)].valuePtr);
			}
			break;

		case 521:
			/* Line 1792 of yacc.c  */
#line 3504 "core\\parse-asn1.y"
			{
				(yyval.valuePtr) = NULL;
			}
			break;

			/* Line 1792 of yacc.c  */
#line 6989 "core\\parse-asn1.c"
		default:
			break;
	}
	/* User semantic actions sometimes alter yychar, and that requires
	   that yytoken be updated with the new translation.  We take the
	   approach of translating immediately before every use of yytoken.
	   One alternative is translating here after every semantic action,
	   but that translation would be missed if the semantic action invokes
	   YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
	   if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
	   incorrect destructor might then be invoked immediately.  In the
	   case of YYERROR or YYBACKUP, subsequent parser actions might lead
	   to an incorrect destructor call or verbose syntax error message
	   before the lookahead is translated.  */
	YY_SYMBOL_PRINT("-> $$ =", yyr1[yyn], &yyval, &yyloc);

	YYPOPSTACK(yylen);
	yylen = 0;
	YY_STACK_PRINT(yyss, yyssp);

	*++yyvsp = yyval;

	/* Now `shift' the result of the reduction.  Determine what state
	   that goes to, based on the state we popped back to and the rule
	   number reduced by.  */

	yyn = yyr1[yyn];

	yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
	if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
		yystate = yytable[yystate];
	else
		yystate = yydefgoto[yyn - YYNTOKENS];

	goto yynewstate;

	/*------------------------------------.
	| yyerrlab -- here on detecting error |
	`------------------------------------*/
yyerrlab:
	/* Make sure we have latest lookahead translation.  See comments at
	   user semantic actions for why this is necessary.  */
	yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE(yychar);

	/* If not already recovering from an error, report this error.  */
	if (!yyerrstatus)
	{
		++yynerrs;
#if !YYERROR_VERBOSE
		yyerror(YY_("syntax error"));
#else
#define YYSYNTAX_ERROR yysyntax_error(&yymsg_alloc, &yymsg, yyssp, yytoken)
		{
			char const* yymsgp = YY_("syntax error");
			int yysyntax_error_status;
			yysyntax_error_status = YYSYNTAX_ERROR;
			if (yysyntax_error_status == 0)
				yymsgp = yymsg;
			else if (yysyntax_error_status == 1)
			{
				if (yymsg != yymsgbuf)
					YYSTACK_FREE(yymsg);
				yymsg = (char*)YYSTACK_ALLOC(yymsg_alloc);
				if (!yymsg)
				{
					yymsg = yymsgbuf;
					yymsg_alloc = sizeof yymsgbuf;
					yysyntax_error_status = 2;
				}
				else
				{
					yysyntax_error_status = YYSYNTAX_ERROR;
					yymsgp = yymsg;
				}
			}
			yyerror(yymsgp);
			if (yysyntax_error_status == 2)
				goto yyexhaustedlab;
		}
#undef YYSYNTAX_ERROR
#endif
	}

	if (yyerrstatus == 3)
	{
		/* If just tried and failed to reuse lookahead token after an
	   error, discard it.  */

		if (yychar <= YYEOF)
		{
			/* Return failure if at end of input.  */
			if (yychar == YYEOF)
				YYABORT;
		}
		else
		{
			yydestruct("Error: discarding", yytoken, &yylval);
			yychar = YYEMPTY;
		}
	}

	/* Else will try to reuse lookahead token after shifting the error
	   token.  */
	goto yyerrlab1;

	///*---------------------------------------------------.
	//| yyerrorlab -- error raised explicitly by YYERROR.  |
	//`---------------------------------------------------*/
	// yyerrorlab:
	//
	//  /* Pacify compilers like GCC when the user code never invokes
	//     YYERROR and the label yyerrorlab therefore never appears in user
	//     code.  */
	//  if (/*CONSTCOND*/ 0)
	//     goto yyerrorlab;
	//
	//  /* Do not reclaim the symbols of the rule which action triggered
	//     this YYERROR.  */
	//  YYPOPSTACK (yylen);
	//  yylen = 0;
	//  YY_STACK_PRINT (yyss, yyssp);
	//  yystate = *yyssp;
	//  goto yyerrlab1;

	/*-------------------------------------------------------------.
	| yyerrlab1 -- common code for both syntax error and YYERROR.  |
	`-------------------------------------------------------------*/
yyerrlab1:
	yyerrstatus = 3; /* Each real token shifted decrements this.  */

	for (;;)
	{
		yyn = yypact[yystate];
		if (!yypact_value_is_default(yyn))
		{
			yyn += YYTERROR;
			if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
			{
				yyn = yytable[yyn];
				if (0 < yyn)
					break;
			}
		}

		/* Pop the current state because it cannot handle the error token.  */
		if (yyssp == yyss)
			YYABORT;

		yydestruct("Error: popping", yystos[yystate], yyvsp);
		YYPOPSTACK(1);
		yystate = *yyssp;
		YY_STACK_PRINT(yyss, yyssp);
	}

	YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
	*++yyvsp = yylval;
	YY_IGNORE_MAYBE_UNINITIALIZED_END

	/* Shift the error token.  */
	YY_SYMBOL_PRINT("Shifting", yystos[yyn], yyvsp, yylsp);

	yystate = yyn;
	goto yynewstate;

	/*-------------------------------------.
	| yyacceptlab -- YYACCEPT comes here.  |
	`-------------------------------------*/
yyacceptlab:
	yyresult = 0;
	goto yyreturn;

	/*-----------------------------------.
	| yyabortlab -- YYABORT comes here.  |
	`-----------------------------------*/
yyabortlab:
	yyresult = 1;
	goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
	/*-------------------------------------------------.
	| yyexhaustedlab -- memory exhaustion comes here.  |
	`-------------------------------------------------*/
yyexhaustedlab:
	yyerror(YY_("memory exhausted"));
	yyresult = 2;
	/* Fall through.  */
#endif

yyreturn:
	if (yychar != YYEMPTY)
	{
		/* Make sure we have latest lookahead translation.  See comments at
		   user semantic actions for why this is necessary.  */
		yytoken = YYTRANSLATE(yychar);
		yydestruct("Cleanup: discarding lookahead", yytoken, &yylval);
	}
	/* Do not reclaim the symbols of the rule which action triggered
	   this YYABORT or YYACCEPT.  */
	YYPOPSTACK(yylen);
	YY_STACK_PRINT(yyss, yyssp);
	while (yyssp != yyss)
	{
		yydestruct("Cleanup: popping", yystos[*yyssp], yyvsp);
		YYPOPSTACK(1);
	}
#ifndef yyoverflow
	if (yyss != yyssa)
		YYSTACK_FREE(yyss);
#endif
#if YYERROR_VERBOSE
	if (yymsg != yymsgbuf)
		YYSTACK_FREE(yymsg);
#endif
	/* Make sure YYID is used.  */
	return YYID(yyresult);
}

/* Line 2055 of yacc.c  */
#line 3507 "core\\parse-asn1.y"

void yyerror(char* s)
{
	fprintf(errFileG, "%s(%ld) : %s at symbol \"%s\"\n\n", modulePtrG->asn1SrcFileName, myLineNoG, s, yytext);
}

/*
 * given a Module*, the file name associated witht the open
 * FILE *fPtr, InitAsn1Parser sets up the yacc/lex parser
 * to parse an ASN.1 module read from fPtr and write the
 * parse results into the given Module *mod.
 */
int InitAsn1Parser PARAMS((mod, fileName, fPtr), Module* mod _AND_ const char* fileName _AND_ FILE* fPtr)
{
	yyin = fPtr;

	/*
	 * reset lexical analyzer input file ptr
	 * (only do this on succesive calls ow yyrestart seg faults
	 */
#ifdef FLEX_IN_USE
	if (!firstTimeThroughG)
		yyrestart(fPtr);

	firstTimeThroughG = FALSE;
#endif

	/*
	 * init modulePtr
	 */
	memset(mod, 0, sizeof(Module));
	modulePtrG = mod;
	mod->asn1SrcFileName = fileName;
	mod->status = MOD_NOT_LINKED;
	mod->hasAnys = FALSE;

	/* init lists to empty */
	mod->typeDefs = AsnListNew(sizeof(void*));
	mod->valueDefs = AsnListNew(sizeof(void*));

	/*
	 * init export list stuff
	 */
	exportListG = NULL;
	exportsParsedG = FALSE;

	/*
	 * reset line number to 1
	 */
	myLineNoG = 1;

	/*
	 * reset error count
	 */
	parseErrCountG = 0;

	/*
	 * set up list to hold values defined in parsed oids
 // REN -- 9/23/02
	 oidElmtValDefsG = AsnListNew (sizeof (void *));
	 */

	smallErrG = 0;

	return 0;

} /* InitAsn1Parser */

/*
 * puts the applicatin tag code, tagCode, and line number it was
 * parsed at into the applTagsG list.  If the APPLICATION tag code
 * is already in the applTagsG list then an error is printed.
 * and the smallErrG flag set to prevent code production.
 */
void PushApplTag PARAMS((tagCode, lineNo), unsigned long tagCode _AND_ unsigned long lineNo)
{
	ApplTag* l;
	ApplTag* new;
	int wasDefined = 0;

	/* make sure not already in list */
	for (l = applTagsG; l != NULL; l = l->next)
	{
		if (l->tagCode == tagCode)
		{
			PrintErrLoc(modulePtrG->asn1SrcFileName, (long)lineNo);
			fprintf(errFileG, "ERROR - APPLICATION tags can be used only once per ASN.1 module.  The tag \"[APPLICATION %ld]\" was previously used on line %ld.\n", tagCode, l->lineNo);
			wasDefined = 1;
			smallErrG = 1;
		}
	}
	if (!wasDefined)
	{
		new = MT(ApplTag);
		new->lineNo = lineNo;
		new->tagCode = tagCode;
		new->next = applTagsG;
		applTagsG = new;
	}
} /* PushApplTag */

/*
 * Empties the applTagsG list.  Usually done between modules.
 */
void FreeApplTags()
{
	ApplTag* l;
	ApplTag* lTmp;

	for (l = applTagsG; l != NULL;)
	{
		lTmp = l->next;
		Free(l);
		l = lTmp;
	}
	applTagsG = NULL;
} /* FreeApplTags */

#if defined(_MSC_VER)
#pragma warning(default : 28182)
#endif
