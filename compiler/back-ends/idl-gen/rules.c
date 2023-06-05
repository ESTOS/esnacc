/*
 * compiler/back_ends/idl_gen/rules.c - initialized c rule structure
 *           inits a table that contains info about
 *           converting each ASN.1 type to an IDL type
 *
 * Copyright (C) 1991, 1992 Michael Sample
 *           and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/back-ends/idl-gen/rules.c,v 1.1.1.1 2005/04/14 14:59:43 \ste Exp $
 * $Log: rules.c,v $
 * Revision 1.1.1.1  2005/04/14 14:59:43  \ste
 * no message
 *
 * Revision 1.2  2004/01/14 19:07:53  gronej
 * Updated Compiler to accept and process relative-oid's
 *
 * Revision 1.1.1.1  2000/08/21 20:36:04  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.1  1997/01/01 20:25:38  rj
 * first draft
 *
 * Revision 1.3  1994/10/08  03:47:49  rj
 */

#include "../../../c-lib/include/asn-incl.h"
#include "../../core/asn1module.h"
#include "rules.h"

IDLRules idlRulesG = {4,

					  "",
					  "_T",

					  "Choice",
					  "a",
					  "ChoiceUnion",
					  FALSE,
					  {{BASICTYPE_UNKNOWN, "???", FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, "NOT_NULL", "unknown"},
					   {BASICTYPE_BOOLEAN, "BOOLEAN", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "bool"},
					   {BASICTYPE_INTEGER, "INTEGER", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "integer"},
					   {BASICTYPE_BITSTRING, "BitString", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "bits"},
					   {BASICTYPE_OCTETSTRING, "OctetString", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "octs"},
					   {BASICTYPE_NULL, "NULL", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "null"},
					   {BASICTYPE_OID, "ObjectIdentifier", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "oid"},
					   {BASICTYPE_REAL, "REAL", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "real"},
					   {BASICTYPE_ENUMERATED, "???", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "enumeration"},
					   {BASICTYPE_SEQUENCE, NULL, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, "NOT_NULL", "seq"},
					   {BASICTYPE_SEQUENCEOF, "AsnList", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "seqOf"},
					   {BASICTYPE_SET, NULL, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, "NOT_NULL", "set"},
					   {BASICTYPE_SETOF, "AsnList", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "setOf"},
					   {BASICTYPE_CHOICE, NULL, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, "NOT_NULL", "choice"},
					   {BASICTYPE_SELECTION, NULL, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "foo"},
					   {BASICTYPE_COMPONENTSOF, NULL, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "bar"},
					   {BASICTYPE_ANY, "any", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "any"},
					   {BASICTYPE_ANYDEFINEDBY, "any", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "anyDefBy"},
					   {BASICTYPE_LOCALTYPEREF, NULL, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "foo"},
					   {BASICTYPE_IMPORTTYPEREF, NULL, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "bar"},
					   {BASICTYPE_MACROTYPE, NULL, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "foo"},
					   {BASICTYPE_MACRODEF, NULL, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "foo"},
					   {BASICTYPE_RELATIVE_OID, "RelativeOid", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, "NOT_NULL", "RelativeOid"}}};
