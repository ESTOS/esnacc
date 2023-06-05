/*
 * compiler/core/oid.c - routines for:
 *          converting an arc number list to an ENC_OID
 *          converting an ENC_OID to an arc number list
 *          arcName mapping routine
 *
 *       does not handle OID's with unresolved valueRefs instead of arcNums
 *
 *  MS 91
 *
 * Copyright (C) 1991, 1992 Michael Sample
 *            and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/core/oid.c,v 1.1.1.1 2005/04/14 14:59:43 \ste Exp $
 * $Log: oid.c,v $
 * Revision 1.1.1.1  2005/04/14 14:59:43  \ste
 * no message
 *
 * Revision 1.7  2004/03/25 19:20:17  gronej
 * fixed some linux warnings
 *
 * Revision 1.6  2003/07/07 14:50:14  nicholar
 * Eliminated headers and cleaned up include references
 *
 * Revision 1.5  2002/10/21 17:15:19  mcphersc
 * fixed long int
 *
 * Revision 1.4  2002/09/16 16:50:17  mcphersc
 * Fixed warnings
 *
 * Revision 1.3  2002/02/26 14:39:56  nicholar
 * Added itu-t and joint-iso-itu-t arcs to OID arc name table.
 *
 * Revision 1.2  2000/10/24 14:54:53  rwc
 * Updated to remove high-level warnings (level 4 on MSVC++) for an easier build.
 * SOME warnings persist due to difficulty in modifying the SNACC compiler to
 * properly build clean source; also some files are built by Lex/Yacc.
 *
 * Revision 1.1.1.1  2000/08/21 20:36:01  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.3  1995/07/25 19:41:41  rj
 * changed `_' to `-' in file names.
 *
 * Revision 1.2  1994/09/01  00:41:33  rj
 * snacc_config.h removed; oid.h includet.
 *
 * Revision 1.1  1994/08/28  09:49:26  rj
 * first check-in. for a list of changes to the snacc-1.1 distribution please refer to the ChangeLog.
 *
 */

#include <string.h>
#include "../../c-lib/include/asn-incl.h"

typedef struct ArcNameMapElmt
{
	const char* arcName;
	int arcNum;
} ArcNameMapElmt;

/*
 * these are the CCITT and ISO pre-defined arc names for the
 * OBJECT IDENTIFIER tree.
 * Ref: CCITT X.208 1988 - Annexes B C and D
 *
 * NOTE: the last entry must have a NULL string and a
 *       -1 arcnumber to indicate the end of the array.
 */
ArcNameMapElmt oidArcNameMapG[14] = {{"itu-t", 0},
									 {"iso", 1},
									 {"joint-iso-itu-t", 2},
									 {"standard", 0},
									 {"registration-authority", 1},
									 {"member-body", 2},
									 {"identified-organization", 3},
									 {"recommendation", 0},
									 {"question", 1},
									 {"administration", 2},
									 {"network-operator", 3},
									 {"ccitt", 0},			 /* synonym for itu-t */
									 {"joint-iso-ccitt", 2}, /* synonym for joint-iso-itu-t */
									 {NULL, -1}};

/*
 * returns the arcnum (>0) of the given name if it
 * is a defined oid arc name like "iso" or "ccitt"
 * returns -1 if the name was  not found
 *
 * name must be null terminated.
 */
int OidArcNameToNum PARAMS((name), char* name)
{
	int i;
	for (i = 0; oidArcNameMapG[i].arcName != NULL; i++)
		if (strcmp(name, oidArcNameMapG[i].arcName) == 0)
			return oidArcNameMapG[i].arcNum;
	return -1;
} /* OidArcNameToNum */

/*
 * Takes and OBJECT IDENTIFER in the linked format
 * (produced by parser) and returns the number of octets
 * that are needed to hold the encoded version of that
 * OBJECT IDENTIFIER.
 */
unsigned long EncodedOidLen PARAMS((oid), OID* oid)
{
	unsigned long totalLen;
	unsigned long headArcNum;
	unsigned long tmpArcNum;
	OID* tmpOid;

	/*
	 * oid must have at least 2 elmts
	 */
	if (oid->next == NULL)
		return 0;

	headArcNum = (oid->arcNum * 40) + oid->next->arcNum;

	/*
	 * figure out total encoded length of oid
	 */
	tmpArcNum = headArcNum;
	for (totalLen = 1; (tmpArcNum >>= 7) != 0; totalLen++)
		;
	for (tmpOid = oid->next->next; tmpOid != NULL; tmpOid = tmpOid->next)
	{
		totalLen++;
		tmpArcNum = tmpOid->arcNum;
		for (; (tmpArcNum >>= 7) != 0; totalLen++)
			;
	}

	return totalLen;

} /* EncodedOidLen */

/*
 * Given an oid arc number list and a pre-allocated ENC_OID
 * (use EncodedOidLen to figure out byte length needed)
 * fills the ENC_OID with a BER encoded version
 * of the oid.
 */
void BuildEncodedOid PARAMS((oid, result), OID* oid _AND_ AsnOid* result)
{
	unsigned long len;
	unsigned long headArcNum;
	unsigned long tmpArcNum;
	char* buf;
	int i;
	OID* tmpOid;

	buf = result->octs;

	/*
	 * oid must have at least 2 elmts
	 */
	if (oid->next == NULL)
		return;
	/*
	 * munge together first two arcNum
	 * note first arcnum must be <= 2
	 * and second must be < 39 if first = 0 or 1
	 * see (X.209) for ref to this stupidity
	 */
	headArcNum = (oid->arcNum * 40) + oid->next->arcNum;

	tmpArcNum = headArcNum;

	/*
	 * calc # bytes needed for head arc num
	 */
	for (len = 0; (tmpArcNum >>= 7) != 0; len++)
		;

	/*
	 * write more signifcant bytes (if any) of head arc num
	 * with 'more' bit set
	 */
	for (i = 0; i < (int)len; i++)
		*(buf++) = (char)(0x80 | (headArcNum >> ((len - i) * 7)));

	/*
	 * write least significant byte of head arc num
	 */
	*(buf++) = (char)(0x7f & headArcNum);

	/*
	 * write following arc nums, if any
	 */
	for (tmpOid = oid->next->next; tmpOid != NULL; tmpOid = tmpOid->next)
	{
		/*
		 * figure out encoded length -1 of this arcNum
		 */
		tmpArcNum = tmpOid->arcNum;
		for (len = 0; (tmpArcNum >>= 7) != 0; len++)
			;

		/*
		 * write more signifcant bytes (if any)
		 * with 'more' bit set
		 */
		for (i = 0; i < (int)len; i++)
			*(buf++) = (char)(0x80 | (tmpOid->arcNum >> ((len - i) * 7)));

		/*
		 * write least significant byte
		 */
		*(buf++) = (char)(0x7f & tmpOid->arcNum);
	}

} /* BuildEncodedOid */

/*
 * Given an ENC_OID, this routine converts it into a
 * linked oid (OID).
 */
void UnbuildEncodedOid PARAMS((eoid, result), AsnOid* eoid _AND_ OID** result)
{
	OID** nextOid;
	OID* headOid;
	int arcNum;
	int i;
	int firstArcNum;
	int secondArcNum;

	for (arcNum = 0, i = 0; (i < (int)(eoid->octetLen)) && (eoid->octs[i] & 0x80); i++)
		arcNum = (arcNum << 7) + (eoid->octs[i] & 0x7f);

	arcNum = (arcNum << 7) + (eoid->octs[i] & 0x7f);
	i++;

	firstArcNum = arcNum / 40;
	if (firstArcNum > 2)
		firstArcNum = 2;

	secondArcNum = arcNum - (firstArcNum * 40);

	headOid = (OID*)Malloc(sizeof(OID));
	headOid->arcNum = firstArcNum;
	headOid->next = (OID*)Malloc(sizeof(OID));
	headOid->next->arcNum = secondArcNum;
	nextOid = &headOid->next->next;

	for (; i < (int)(eoid->octetLen);)
	{
		for (arcNum = 0; (i < (int)(eoid->octetLen)) && (eoid->octs[i] & 0x80); i++)
			arcNum = (arcNum << 7) + (eoid->octs[i] & 0x7f);

		arcNum = (arcNum << 7) + (eoid->octs[i] & 0x7f);
		i++;
		*nextOid = (OID*)Malloc(sizeof(OID));
		(*nextOid)->arcNum = arcNum;
		nextOid = &(*nextOid)->next;
	}

	*result = headOid;

} /* UnbuildEncodedOid */
