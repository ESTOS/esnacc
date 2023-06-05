/*
 * exp_buf.h - read/write/alloc/free routines for a simple buffer structure
 *
 * MACROS are gross but execution speed is important
 *
 * NOTE: replacing the malloc and free with a allocs/frees
 *       from/to buffer pools or similar tuned/fixed size
 *       mem mgmt will improve performance.
 *
 *  You should tune the buffer management to your environment
 *  for best results
 *
 * MS 91
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

#ifndef _exp_buf_h_
#define _exp_buf_h_

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct ExpBuf
	{
		char* dataStart; /* points to first valid data byte */
		/* when empty, 1 byte past blk end (rvs write)*/
		char* dataEnd; /* pts to first byte AFTER last valid data byte*/
		char* curr;	   /* current location to read form */
		/* points to next byte to read */
		struct ExpBuf* next; /* next buf (NULL if no next buffer)*/
		struct ExpBuf* prev; /* prev buf (NULL if no prev buffer)*/
		char* blkStart;		 /* points to first byte of the blk */
		char* blkEnd;		 /* points the first byte AFTER blks last byte */
		int readError;		 /* non-zero is attempt to read past end of data*/
		int writeError;		 /* non-zero is attempt write fails (no mor bufs)*/
	} ExpBuf;

	/* init, alloc and free routines */

	void PutExpBufInGenBuf PROTO((ExpBuf * eb, GenBuf* gb));
	void ExpBuftoGenBuf PROTO((ExpBuf * eb, GenBuf** gb));

	void ExpBufInit PROTO((unsigned long dataBlkSize));
	ExpBuf* ExpBufAllocBuf();
	void ExpBufFreeBuf PROTO((ExpBuf * ptr));
	char* ExpBufAllocData();
	void ExpBufFreeData PROTO((char* ptr));
	void ExpBufFreeBufAndData PROTO((ExpBuf * b));

	ExpBuf* ExpBufNext PROTO((ExpBuf * b));
	ExpBuf* ExpBufPrev PROTO((ExpBuf * b));
	void ExpBufResetInReadMode PROTO((ExpBuf * *b));
	void ExpBufResetInWriteRvsMode PROTO((ExpBuf * b));

	int ExpBufAtEod PROTO((ExpBuf * b));
	int ExpBufFull PROTO((ExpBuf * b));
	int ExpBufHasNoData PROTO((ExpBuf * b));
	size_t ExpBufDataSize PROTO((ExpBuf * b));
	size_t ExpBufDataBlkSize PROTO((ExpBuf * b));
	char* ExpBufDataPtr PROTO((ExpBuf * b));

	extern unsigned long expBufDataBlkSizeG;

	int ExpBufReadError PROTO((ExpBuf * *b));
	int ExpBufWriteError PROTO((ExpBuf * *b));
	int ExpBufSetWriteError PROTO((ExpBuf * b, unsigned short Value));

	ExpBuf* ExpBufAllocBufAndData();
	void ExpBufInstallDataInBuf PROTO((ExpBuf * b, char* data, unsigned long len));
	void ExpBufFreeBufAndDataList PROTO((ExpBuf * b));
	ExpBuf* ExpBufListLastBuf PROTO((ExpBuf * b));
	ExpBuf* ExpBufListFirstBuf PROTO((ExpBuf * b));

	void ExpBufCopyToFile PROTO((ExpBuf * b, FILE* f));

	/* reading and writing routines */

	size_t ExpBufCopyAny PROTO((ExpBuf * *b, void* value, size_t* bytesDecoded, ENV_TYPE env));
	void ExpBufSkip PROTO((ExpBuf**, size_t len));
	size_t ExpBufCopy PROTO((char* dst, ExpBuf** b, size_t len));
	unsigned char ExpBufPeekByte PROTO((ExpBuf * *b));
	size_t ExpBufPeekCopy PROTO((char* dst, ExpBuf** b, size_t len));
	char* ExpBufPeekSeg PROTO((ExpBuf * *b, size_t* len));
	char* ExpBufGetSeg PROTO((ExpBuf * *b, size_t* len));
	void ExpBufPutSegRvs PROTO((ExpBuf * *b, char* data, size_t len));
	unsigned char ExpBufGetByte PROTO((ExpBuf * *b));
	void ExpBufPutByteRvs PROTO((ExpBuf * *b, unsigned char byte));

#ifdef __cplusplus
}
#endif

#endif /* conditional include */
