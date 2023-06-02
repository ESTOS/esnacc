/*
 * asn_enum.h
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
 */

#ifndef _asn_enum_h_
#define _asn_enum_h_

#ifdef __cplusplus
extern "C" {
#endif


	typedef AsnInt AsnEnum;

	/*
	 * ENUMERATED have a UNIVERSAL tag that is diff from INTEGERS
	 * so need diff encoding routine tho content stuff is the same
	 */
	AsnLen BEncAsnEnum PROTO((GenBuf* b, AsnEnum* data));

	void BDecAsnEnum PROTO((GenBuf* b, AsnEnum* result, AsnLen* bytesDecoded, ENV_TYPE env));

	/* DAD - modified the two defines here so that enum Ptr's can
	 * be resolved to the same size dest as what AsnInt gets
	 * defined to be.
	 */
#define BEncAsnEnumContent(a,b) BEncAsnIntContent((a),((AsnInt *) (b)))

#define BDecAsnEnumContent(a,b,c,d,e,f)  BDecAsnIntContent((a),(b),(c),((AsnInt*)(d)),(e),(f))

#define FreeAsnEnum FreeAsnInt

#define PrintAsnEnum PrintAsnInt

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* conditional include */

