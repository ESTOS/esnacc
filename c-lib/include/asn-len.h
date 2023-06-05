/*
 * asn_len.h
 *
 * Warning: many of these routines are MACROs for performance reasons
 *          - be carful where you use them.  Don't use more than one per
 *          assignment statement -
 *          (eg itemLen += BEncEoc (b) + BEncFoo (b) ..; this
 *           will break the code)
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

#ifndef _asn_len_h_
#define _asn_len_h_

#ifdef __cplusplus
extern "C"
{
#endif

	typedef unsigned long AsnLen;

	/*
	 * BER Encoding/Decoding routines
	 */

	/* max unsigned value  - used for internal rep of indef len */
#define INDEFINITE_LEN ~0L

#ifdef USE_INDEF_LEN

#define BEncEocIfNec(b) BEncEoc(b)

/*
 * include len for EOC  (2 must be first due to BEncIndefLen
 * - ack! ugly macros!)
 */
#define BEncConsLen(b, len) 2 + BEncIndefLen(b)

#else /* use definite length - faster?/smaller encodings */

/* do nothing since only using definite lens */
#define BEncEocIfNec(b)

#define BEncConsLen(b, len) BEncDefLen(b, len)

#endif

/*
 * writes indefinite length byte to buffer. 'returns' encoded len (1)
 */
#define BEncIndefLen(b)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
	1;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
	BufPutByteRvs(b, 0x80);

#ifndef _DEBUG
#define BEncEoc(b)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
	2;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
	BufPutByteRvs(b, 0);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           \
	BufPutByteRvs(b, 0);
#endif

	/*
	 * use if you know the encoded length will be 0 >= len <= 127
	 * Eg for booleans, nulls, any resonable integers and reals
	 *
	 * NOTE: this particular Encode Routine does NOT return the length
	 * encoded (1).
	 */
#define BEncDefLenTo127(b, len) BufPutByteRvs(b, (unsigned char)len)

#define BDEC_2ND_EOC_OCTET(b, bytesDecoded, env)                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
	{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
		if ((BufGetByte(b) != 0) || BufReadError(b))                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
		{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
			Asn1Error("ERROR - second octet of EOC not zero\n");                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
			longjmp(env, -28);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
		}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
		(*bytesDecoded)++;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
	}

	AsnLen BEncDefLen PROTO((GenBuf * b, AsnLen len));
	AsnLen BEncDefLen2 PROTO((GenBuf * b, long len));
	AsnLen BDecLen PROTO((GenBuf * b, AsnLen* bytesDecoded, ENV_TYPE env));

#ifdef _DEBUG
	AsnLen BEncEoc PROTO((GenBuf * b));
#endif
	void BDecEoc PROTO((GenBuf * b, AsnLen* bytesDecoded, ENV_TYPE env));

#if TTBL
	int PeekEoc PROTO((GenBuf * b));
#endif

	/*
	 * DER Encoding/Decoding routines
	 */

	/* We always use Definite length encoders */

	/* do nothing since only using definite lens */
#define DEncEocIfNec(b)

#define DEncConsLen DEncDefLen

/*
 * use if you know the encoded length will be 0 >= len <= 127
 * Eg for booleans, nulls, any resonable integers and reals
 *
 * NOTE: this particular Encode Routine does NOT return the length
 * encoded (1).
 */
#define DEncDefLenTo127(b, len) BufPutByteRvs(b, (unsigned char)len)

#define DEncDefLen BEncDefLen

	AsnLen DDecLen PROTO((GenBuf * b, AsnLen* bytesDecoded, ENV_TYPE env));

	/* Error conditions */
#define DDecEoc(a, b, env) longjmp(env, -666)

/* Should never happen */
#define DDEC_2ND_EOC_OCTET(a, b, env)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* conditional include */
