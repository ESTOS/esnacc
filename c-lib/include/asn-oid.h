/*
 * asn_oid.h
 *
 *  this file depends on asn_octs.h
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

#ifndef _asn_oid_h_
#define _asn_oid_h_

#include "asn-octs.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef AsnOcts AsnOid; /* standard oid type  */

#define ASNOID_PRESENT(aoid) ASNOCTS_PRESENT(aoid)

	AsnLen BEncAsnOid PROTO((GenBuf * b, AsnOid* data));

	void BDecAsnOid PROTO((GenBuf * b, AsnOid* result, AsnLen* bytesDecoded, ENV_TYPE env));

#define BEncAsnOidContent(b, oid) BEncAsnOctsContent(b, oid)

	void BDecAsnOidContent PROTO((GenBuf * b, AsnTag tag, AsnLen len, AsnOid* result, AsnLen* bytesDecoded, ENV_TYPE env));

#define FreeAsnOid FreeAsnOcts

	void PrintAsnOid PROTO((FILE * f, AsnOid* b, unsigned int indent));

#define AsnOidsEquiv(o1, o2) AsnOctsEquiv(o1, o2)

	/* linked oid type that may be easier to use in some circumstances */
#define NULL_OID_ARCNUM -1
	typedef struct OID
	{
		struct OID* next;
		long arcNum;
#if COMPILER || TTBL
		struct Value* valueRef;
#endif
	} OID;

	AsnLen EncodedOidLen PROTO((OID * oid));

	void BuildEncodedOid PROTO((OID * oid, AsnOid* result));

	void UnbuildEncodedOid PROTO((AsnOid * eoid, OID** result));

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* conditional include */
