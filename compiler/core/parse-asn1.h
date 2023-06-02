/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C

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

#ifndef YY_YY_CORE_PARSE_ASN1_H_INCLUDED
# define YY_YY_CORE_PARSE_ASN1_H_INCLUDED
	  /* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
	  know about them.  */
enum yytokentype {
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


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
	/* Line 2058 of yacc.c  */
#line 183 "core\\parse-asn1.y"

	int              intVal;
	unsigned int     uintVal;
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
	CTypeId			cTypeIdEnum;
	AsnBool			boolVal;


	/* Line 2058 of yacc.c  */
#line 299 "core\\parse-asn1.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
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
