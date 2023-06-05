/*
 * asn_enum.c - routines for the ASN.1 ENUMERATED type
 *
 * MS 92
 * Copyright (C) 1992 Michael Sample and the University of British Columbia
 *
 * This library is free software; you can redistribute it and/or
 * modify it provided that this copyright/license information is retained
 * in original form.
 *
 * If you modify this file, you must clearly indicate your changes.
 *
 * This source code is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/c-lib/src/asn-enum.c,v 1.1.1.1 2005/04/14 14:59:42 \ste Exp $
 * $Log: asn-enum.c,v $
 * Revision 1.1.1.1  2005/04/14 14:59:42  \ste
 * no message
 *
 * Revision 1.3  2003/12/17 19:05:03  gronej
 * SNACC baseline merged with PER v1_7 tag
 *
 * Revision 1.2.2.1  2003/11/05 14:58:53  gronej
 * working PER code merged with esnacc_1_6
 *
 * Revision 1.2  2002/10/23 10:23:37  mcphersc
 * Changed BUF_TYPE to AsnBuf
 *
 * Revision 1.1.1.1  2000/08/21 20:35:52  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.3  1995/07/24 21:04:50  rj
 * changed `_' to `-' in file names.
 *
 * Revision 1.2  1994/09/01  01:04:38  rj
 * reduce the risk of unwanted surprises with macro expansion by properly separating the C tokens.
 *
 * Revision 1.1  1994/08/28  09:45:52  rj
 * first check-in. for a list of changes to the snacc-1.1 distribution please refer to the ChangeLog.
 *
 */

#include "../include/asn-config.h"
#include "../include/asn-len.h"
#include "../include/asn-tag.h"
#include "../include/asn-int.h"
#include "../include/asn-enum.h"

/*
 * encodes universal TAG LENGTH and Contents of and ASN.1 ENUMERATED
 */
AsnLen BEncAsnEnum PARAMS((b, data), GenBuf* b _AND_ AsnEnum* data)
{
	AsnLen len;

	len = BEncAsnEnumContent(b, data);
	len += BEncDefLen(b, len);
	len += BEncTag1(b, UNIV, PRIM, ENUM_TAG_CODE);
	return len;
} /* BEncAsnEnum */

/*
 * decodes universal TAG LENGTH and Contents of and ASN.1 ENUMERATED
 */
void BDecAsnEnum PARAMS((b, result, bytesDecoded, env), GenBuf* b _AND_ AsnEnum* result _AND_ AsnLen* bytesDecoded _AND_ jmp_buf env)
{
	AsnTag tag;
	AsnLen elmtLen;

	if ((tag = BDecTag(b, bytesDecoded, env)) != MAKE_TAG_ID(UNIV, PRIM, ENUM_TAG_CODE))
	{
		Asn1Error("BDecAsnInt: ERROR wrong tag on ENUMERATED.\n");
		longjmp(env, -40);
	}

	elmtLen = BDecLen(b, bytesDecoded, env);
	BDecAsnEnumContent(b, tag, elmtLen, result, bytesDecoded, env);

} /* BDecAsnEnum */
