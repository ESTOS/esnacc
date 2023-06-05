/*
 * asn-der.h
 *
 * Dean Povey 97/08
 * Copyright (C) 1997 Dean Povey and the Distributed Systems Technology Centre
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

#ifndef _asn_der_h
#define _asn_der_h
#include "exp-buf.h"
#include "gen-buf.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct EncodedElmt
	{
		GenBuf* b;
		unsigned long len;
	} EncodedElmt;

	int EncodedElmtCmp PROTO((const void* a1, const void* b));

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _asn_der_h */
