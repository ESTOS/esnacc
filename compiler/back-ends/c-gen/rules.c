/*
 * compiler/back-ends/c-gen/rules.c - initialized c rule structure
 *           inits a table that contains info about
 *           converting each ASN.1 type to C type
 * Copyright (C) 1991, 1992 Michael Sample
 *            and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/back-ends/c-gen/rules.c,v 1.1.1.1 2005/04/14 14:59:43 \ste Exp $
 * $Log: rules.c,v $
 * Revision 1.1.1.1  2005/04/14 14:59:43  \ste
 * no message
 *
 * Revision 1.9  2004/03/12 18:51:20  gronej
 * updated c-library to error on extension additions as it does with untagged elements
 *
 * Revision 1.8  2004/01/14 19:07:53  gronej
 * Updated Compiler to accept and process relative-oid's
 *
 * Revision 1.7  2003/07/30 00:52:34  colestor
 * Modified "UTCTIme" reference to "UTCTime" to properly reference built-in type.
 * (Failed on "C" builds).
 *
 * Revision 1.6  2003/07/29 11:26:55  nicholar
 * Fixed EXTERNAL rules
 *
 * Revision 1.5  2003/07/07 14:53:38  nicholar
 * Eliminated headers and cleaned up include references
 *
 * Revision 1.4  2003/04/29 21:00:59  leonberp
 * integerated Deepak's changes for IOB support
 *
 * Revision 1.3  2002/05/20 19:39:12  leonberp
 * fixed typo
 *
 * Revision 1.2  2002/05/20 13:20:34  leonberp
 * removed bogus copy constructors, operator=, and made constructors inline for C++ String classes
 *
 * Revision 1.1.1.1  2000/08/21 20:36:05  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.3  1995/07/25 18:46:34  rj
 * file name has been shortened for redundant part: c-gen/c-rules -> c-gen/rules.
 *
 * Revision 1.2  1994/09/01  00:24:35  rj
 * snacc_config.h removed.
 *
 * Revision 1.1  1994/08/28  09:48:35  rj
 * first check-in. for a list of changes to the snacc-1.1 distribution please refer to the ChangeLog.
 *
 */

#include "../../../c-lib/include/asn-incl.h"
#include "../../core/asn1module.h"
#include "rules.h"

/*
 *  (see rule.h and asn1module.h)
 *
 */

// Deepak: ~~~~~~~~~~~~~ Notes 30/Nov/2002 ~~~~~~~~~~~~~~~
// Any new initialization should be added to the following whenever a new type is added to struct BasicType.
// so whatever is added here, should be at the same place as in the BasicType initialization in asn1module.h

CRules cRulesG = {4,
				  "choiceId",
				  "ChoiceId",
				  "a",
				  "ChoiceUnion",
				  TRUE,
				  "Print",
				  "Enc",
				  "Dec",
				  "Free",
				  {{BASICTYPE_UNKNOWN, C_NO_TYPE, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, "NOT_NULL", "unknown", NULL, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},
				   {BASICTYPE_BOOLEAN, C_LIB, "AsnBool", FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, "NOT_NULL", "bool", "PrintAsnBool", "EncAsnBool", "DecAsnBool", "FreeAsnBool", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_INTEGER, C_LIB, "AsnInt", FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, "NOT_NULL", "int", "PrintAsnInt", "EncAsnInt", "DecAsnInt", "FreeAsnInt", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_BITSTRING, C_LIB, "AsnBits", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNBITS_PRESENT", "bits", "PrintAsnBits", "EncAsnBits", "DecAsnBits", "FreeAsnBits", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_OCTETSTRING, C_LIB, "AsnOcts", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncAsnOcts", "DecAsnOcts", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_NULL, C_LIB, "AsnNull", FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, "NOT_NULL", "null", "PrintAsnNull", "EncAsnNull", "DecAsnNull", "FreeAsnNull", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_OID, C_LIB, "AsnOid", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOID_PRESENT", "oid", "PrintAsnOid", "EncAsnOid", "DecAsnOid", "FreeAsnOid", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_REAL, C_LIB, "AsnReal", FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, "NOT_NULL", "real", "PrintAsnReal", "EncAsnReal", "DecAsnReal", "FreeAsnReal", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_ENUMERATED, C_LIB, "enum", FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, "NOT_NULL", "enum", "PrintAsnEnum", "EncAsnEnum", "DecAsnEnum", "FreeAsnEnum", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_SEQUENCE, C_STRUCT, "struct", FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, "NOT_NULL", "seq", NULL, NULL, NULL, NULL, TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_SEQUENCEOF, C_LIST, "AsnList", FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, "NOT_NULL", "list", NULL, NULL, NULL, NULL, TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_SET, C_STRUCT, "struct", FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, "NOT_NULL", "set", NULL, NULL, NULL, NULL, TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_SETOF, C_LIST, "AsnList", FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, "NOT_NULL", "list", NULL, NULL, NULL, NULL, TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_CHOICE, C_CHOICE, NULL, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, "NOT_NULL", "choice", NULL, NULL, NULL, NULL, TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_SELECTION, C_NO_TYPE, NULL, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, "NOT_NULL", "selection", NULL, NULL, NULL, NULL, TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_COMPONENTSOF, C_NO_TYPE, NULL, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, "NOT_NULL", "compsOf", NULL, NULL, NULL, NULL, TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_ANY, C_ANY, "AsnAny", FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, "NOT_NULL", "any", "PrintAsnAny", "EncAsnAny", "DecAsnAny", "FreeAsnAny", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_ANYDEFINEDBY, C_ANYDEFINEDBY, "AsnAnyDefinedBy", FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, "NOT_NULL", "anyDefBy", "PrintAsnAnyDefinedBy", "EncAsnAnyDefinedBy", "DecAsnAnyDefinedBy", "FreeAsnAnyDefinedBy", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_LOCALTYPEREF, C_TYPEREF, NULL, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, "NOT_NULL", "t", NULL, NULL, NULL, NULL, TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_IMPORTTYPEREF, C_TYPEREF, NULL, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, "NOT_NULL", "t", NULL, NULL, NULL, NULL, TRUE, TRUE, TRUE, TRUE, TRUE},
				   {
					   BASICTYPE_MACROTYPE,
					   C_MACROTYPE, // C_NO_TYPE,	// Deepak: 17/Apr/2003
					   NULL,
					   FALSE,
					   FALSE,
					   FALSE,
					   FALSE,
					   FALSE,
					   FALSE,
					   "NOT_NULL",
					   "macroType",
					   NULL,
					   NULL,
					   NULL,
					   NULL,
					   TRUE, // FALSE,			// Deepak: 17/Apr/2003
					   TRUE, // FALSE,			// Deepak: 17/Apr/2003
					   TRUE, // FALSE,			// Deepak: 17/Apr/2003
					   TRUE, // FALSE,			// Deepak: 17/Apr/2003
					   TRUE	 // FALSE			// Deepak: 17/Apr/2003
				   },
				   {BASICTYPE_MACRODEF, C_NO_TYPE, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, "NOT_NULL", "macroDef", NULL, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},
				   {BASICTYPE_NUMERIC_STR, C_LIB, "NumericString", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncNumericString", "DecNumericString", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_PRINTABLE_STR, C_LIB, "PrintableString", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncPrintableString", "DecPrintableString", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_UNIVERSAL_STR, C_LIB, "UniversalString", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncUniversalString", "DecUniversalString", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_IA5_STR, C_LIB, "IA5String", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncIA5String", "DecIA5String", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_BMP_STR, C_LIB, "BMPString", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncBMPString", "DecBMPString", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_UTF8_STR, C_LIB, "UTF8String", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncUTF8String", "DecUTF8String", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   /* UTCTime */
				   {BASICTYPE_UTCTIME, C_LIB, "UTCTime", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncUTCTime", "DecUTCTime", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   /* GeneralizedTime */
				   {BASICTYPE_GENERALIZEDTIME, C_LIB, "GeneralizedTime", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncGeneralizedTime", "DecGeneralizedTime", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   /* GraphicString */
				   {BASICTYPE_GRAPHIC_STR, C_LIB, "GraphicString", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncGraphicString", "DecGraphicString", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   /* VisibleString */
				   {BASICTYPE_VISIBLE_STR, C_LIB, "VisibleString", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncVisibleString", "DecVisibleString", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   /* GeneralString */
				   {BASICTYPE_GENERAL_STR, C_LIB, "GeneralString", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncGeneralString", "DecGeneralString", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   /* ObjectDescriptor */
				   {BASICTYPE_OBJECTDESCRIPTOR, C_LIB, "ObjectDescriptor", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncObjectDescriptor", "DecObjectDescriptor", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   /* VideotexString */
				   {BASICTYPE_VIDEOTEX_STR, C_LIB, "VideotexString", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncVideotexString", "DecVideotexString", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   /* TeletexString */
				   {BASICTYPE_T61_STR, C_LIB, "TeletexString", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncTeletexString", "DecTeletexString", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   /* EXTERNAL */
				   {BASICTYPE_EXTERNAL, C_LIB, "EXTERNAL", FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, "NOT_NULL", "ext", "PrintEXTERNAL", "EncEXTERNAL", "DecEXTERNAL", "FreeEXTERNAL", TRUE, TRUE, TRUE, TRUE, TRUE},
				   /* REN -- 3 July 2003 -- This next type will need C back-end modifications
											Added to complete array */
				   {BASICTYPE_OCTETCONTAINING, C_LIB, "AsnOcts", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNOCTS_PRESENT", "octs", "PrintAsnOcts", "EncAsnOcts", "DecAsnOcts", "FreeAsnOcts", TRUE, TRUE, TRUE, TRUE, TRUE},
				   /* REN -- 3 July 2003 -- This next type will need C back-end modifications.
											Added to complete array */
				   {BASICTYPE_BITCONTAINING, C_LIB, "AsnBits", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNBITS_PRESENT", "bits", "PrintAsnBits", "EncAsnBits", "DecAsnBits", "FreeAsnBits", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_RELATIVE_OID, C_LIB, "AsnRelativeOid", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNRELATIVEOID_PRESENT", "oid", "PrintAsnRelativeOid", "EncAsnRelativeOid", "DecAsnRelativeOid", "FreeAsnRelativeOid", TRUE, TRUE, TRUE, TRUE, TRUE},
				   {BASICTYPE_EXTENSION, C_LIB, "AsnExtension", FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, "ASNEXTENSION_PRESENT", "extension", "PrintAsnExtension", "EncAsnExtension", "DecAsnExtension", "FreeAsnExtension", TRUE, TRUE, TRUE, TRUE, TRUE},
				   // Deepak: 30/Nov/2002, read Note above
				   {BASICTYPE_SEQUENCET, C_STRUCT, "struct", FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, "NOT_NULL", "seq", NULL, NULL, NULL, NULL, TRUE, TRUE, TRUE, TRUE, TRUE},
				   {// Deepak: 11/Dec/2002
					BASICTYPE_OBJECTCLASS,
					C_OBJECTCLASS, // C_NO_TYPE,
					"struct",	   // NULL,
					FALSE,		   // isPdu
					FALSE,		   // isEncDec
					FALSE,		   // isPtrForTypeDef
					TRUE,		   // TRUE,		// isPtrForTypeRef
					TRUE,		   // FALSE,		// isPtrInChoice
					TRUE,		   // FALSE,		// isPtrForOpt;
					"NOT_NULL",
					"objectclass",
					NULL,
					NULL,
					NULL,
					NULL,
					FALSE,
					FALSE,
					FALSE,
					FALSE,
					TRUE}, //*/
				   {	   // Deepak: 04/Feb/2003
					BASICTYPE_OBJECTCLASSFIELDTYPE,
					C_OBJECTCLASSFIELDTYPE,
					"AsnOcts", // 31/Mar/2003	NULL,
					FALSE,
					TRUE,
					FALSE,
					FALSE,
					FALSE,
					FALSE,
					"NOT_NULL",
					"octs",			//	31/Mar/2003 "objectClassFieldType",
					"PrintAsnOcts", //	31/Mar/2003 NULL,
					"EncAsnOcts",	//	31/Mar/2003 NULL,
					"DecAsnOcts",	//	31/Mar/2003 NULL,
					"FreeAsnOcts",	//	31/Mar/2003 NULL,
					TRUE,
					TRUE,
					TRUE,
					TRUE,
					TRUE}}};
