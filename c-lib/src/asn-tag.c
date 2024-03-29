/*
 * asn_tag.c - BER encode, decode and untility routines for ASN.1 Tags.
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
 * $Header: /develop30/common/esnacc1.7/SNACC/c-lib/src/asn-tag.c,v 1.1.1.1 2005/04/14 14:59:42 \ste Exp $
 * $Log: asn-tag.c,v $
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
 * Revision 1.5  1997/09/03 12:11:41  wan
 * Patch to tag decoding for tags > 2^14 (thanks to Enrico Badella)
 * Patch to TblEncTag to emit final 0x00 if previous octet signals continuation
 *
 * Revision 1.4  1997/03/13 09:15:18  wan
 * Improved dependency generation for stupid makedepends.
 * Corrected PeekTag to peek into buffer only as far as necessary.
 * Added installable error handler.
 * Fixed small glitch in idl-code generator (Markku Savela <msa@msa.tte.vtt.fi>).
 *
 * Revision 1.3  1997/02/28 13:39:50  wan
 * Modifications collected for new version 1.3: Bug fixes, tk4.2.
 *
 * Revision 1.2  1995/07/27 09:01:25  rj
 * merged PeekTag(), a function used only by the type table code.
 *
 * changed `_' to `-' in file names.
 *
 * Revision 1.1  1994/08/28  09:46:01  rj
 * first check-in. for a list of changes to the snacc-1.1 distribution please refer to the ChangeLog.
 *
 */

#include "../include/asn-config.h"
#include "../include/asn-len.h"
#include "../include/asn-tag.h"

/*
 * Returns an AsnTag.  An AsnTag is simply an encoded tag
 * shifted to fill up an unsigned long int (first tag byte
 * in most sig byte of long int)
 * This rep permits easy case stmt comparison of tags.
 * NOTE: The unsigned long rep for tag BREAKS if the
 *       the tag's code is over 2^21 (very unlikely)
 *
 * RETURNS 0 if decoded a 0 byte (ie first byte of an EOC)
 */
AsnTag BDecTag PARAMS((b, bytesDecoded, env), GenBuf* b _AND_ AsnLen* bytesDecoded _AND_ jmp_buf env)
{
	AsnTag tagId;
	AsnTag tmpTagId;
	int i;

	tagId = ((AsnTag)BufGetByte(b)) << ((sizeof(AsnTag) - 1) * 8);
	(*bytesDecoded)++;

	/* check if long tag format (ie code > 31) */
	if ((tagId & (((AsnTag)0x1f) << ((sizeof(AsnTag) - 1) * 8))) == (((AsnTag)0x1f) << ((sizeof(AsnTag) - 1) * 8)))
	{
		i = 2;
		do
		{
			tmpTagId = (AsnTag)BufGetByte(b);
			tagId |= (tmpTagId << ((sizeof(AsnTag) - i) * 8));
			(*bytesDecoded)++;
			i++;
		} while ((tmpTagId & (AsnTag)0x80) && (i <= sizeof(AsnTag)));

		/*
		 * check for tag that is too long
		 */
		if (i > (sizeof(AsnTag) + 1))
		{
			Asn1Error("BDecTag: ERROR - tag value overflow\n");
			longjmp(env, -25);
		}
	}

	if (BufReadError(b))
	{
		Asn1Error("BDecTag: ERROR - decoded past the end of data\n");
		longjmp(env, -26);
	}

	return tagId;

} /* BDecTag */

#if TTBL
AsnTag PeekTag PARAMS((b, env), GenBuf* b _AND_ ENV_TYPE env)
{
	AsnTag tagId, tmpTagId;
	int i;
	unsigned char buf[sizeof(AsnTag)];
	unsigned char* p = buf;

	/*
	 * peek/copy the next (max size of tag) bytes
	 * to get the tag info.  The Peek buffer routines
	 * were added to the standard set for this function.
	 */

	BufPeekCopy((char*)buf, b, 1);
	tagId = ((AsnTag)*p++) << ((sizeof(AsnTag) - 1) * 8);

	/* check if long tag format (ie code > 31) */
	if ((tagId & (((AsnTag)0x1f) << ((sizeof(AsnTag) - 1) * 8))) == (((AsnTag)0x1f) << ((sizeof(AsnTag) - 1) * 8)))
	{
		i = 2;
		do
		{
			BufPeekCopy((char*)buf, b, i);
			tmpTagId = (AsnTag)*p++;
			tagId |= (tmpTagId << ((sizeof(AsnTag) - i) * 8));
			i++;
		} while ((tmpTagId & (AsnTag)0x80) && (i <= sizeof(AsnTag)));

		/*
		 * check for tag that is too long
		 */
		if (i > (sizeof(AsnTag) + 1))
		{
			Asn1Error("BDecTag: ERROR - tag value overflow\n");
			longjmp(env, -1004);
		}
	}

	return tagId;

} /* PeekTag */
#endif /* TTBL */
