/*
 * asn_config.h - configures the ANSI/non ansi, defines
 *                decoder alloc routines and buffer routines
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

#ifndef _asn_config_h_
#define _asn_config_h_

#include <stdio.h>
#include <setjmp.h> /* for jmp_buf type, setjmp and longjmp */
#include <stdlib.h>

/* for pow() used in asn_real.c - must include to avoid casting err on pow */
/* #include <math.h> */

#include "../../snacc.h"


/* used to test if optionals are present */
#define NOT_NULL( ptr)			((ptr) != NULL)

#ifndef _WIN32
// Replaces the microsoft specific "secure" string

#ifndef strcpy_s
	#define strcpy_s(dest, len, source) strncpy(dest, source, len)
#endif

#ifndef _strdup
	#define _strdup(arg) strdup(arg)
#endif

#ifndef strncpy_s
	#define strncpy_s(dest, len, source, amount) strncpy(dest, source, amount)
#endif

#ifndef strcat_s
	#define strcat_s(dest, len, source) strcat(dest, source)
#endif

#ifndef sprintf_s
	#define sprintf_s(target, size, format, ...) snprintf(target, size, format, __VA_ARGS__)
#endif

#ifndef _mkdir
	#include <sys/stat.h>
	#define _mkdir(path) mkdir(path, 777)
#endif

#ifndef _isatty
	#include <unistd.h>
	#define _isatty(fd) isatty(fd)
#endif

#ifndef _fileno
	#include <stdio.h>
	#define _fileno(stream) fileno(stream)
#endif

#ifndef _MAX_PATH
	#define _MAX_PATH 4096
#endif

#ifndef _strlwr_s
	#include "platform-functions.h"
	#define _strlwr_s(buffer, size) mytolower(buffer)
#endif

#ifndef fopen_s
	#include "platform-functions.h"
	#define fopen_s(pFile, filename, mode) myfopen(pFile, filename, mode)
#endif

#endif // _WIN32

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Asn1Error (char *str) - configure error handler
 */
void Asn1Error PROTO ((char* str));


/*
 * Asn1Warning (char *str) - configure warning mechanism
 * (currently never called)
 */
void Asn1Warning PROTO ((char* str));

/*
 * Asn1ErrorHandler - procedure to call upon Asn1Warning (severity 0)
 * and Asn1Error (severity 1).
 */
typedef void (*Asn1ErrorHandler) PROTO ((char* str, int severity));

/*
 * Asn1InstallErrorHandler - installs new error handler, returns former one
 */
Asn1ErrorHandler Asn1InstallErrorHandler PROTO ((Asn1ErrorHandler handler));

/*
 * configure memory scheme used by decoder to allocate memory
 * for the decoded value.
 * The Asn1Free will be called in the optionally generated
 * hierachical free routines.
 *
 * nibble_alloc allocs from a single buffer and EVERYTHING
 * is freed by a single fcn call. Individual elmts cannot be freed
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef USE_NIBBLE_MEMORY

#include "nibble-alloc.h"

#define Asn1Alloc( size)		NibbleAlloc (size)
#define Asn1Free( ptr)			/* empty */
#define CheckAsn1Alloc( ptr, env)	\
	if ((ptr) == NULL)\
	  longjmp (env, -27)

#else /* !USE_NIBBLE_MEMORY */

#include "mem.h"

#define Asn1Alloc( size)		Malloc (size)
#define Asn1Free( ptr)			Free (ptr)
#define CheckAsn1Alloc( ptr, env)	\
	if ((ptr) == NULL)\
	  longjmp (env, -27)

#endif /* USE_NIBBLE_MEMORY */

#define ENV_TYPE jmp_buf





/*
 * NOTE: for use with tables, I defined the (slower)
 *  GenBuf type that is more flexible (_ la ISODE and XDR).
 *  This allows the encode/decode libs to support other
 *  buffer types dynamically instead of having different
 *  libs for each buffer type.
 *  The GenBufs are not provided for the compiled code
 *  (ie the c_lib directory) but could easily be added
 *  (I don't have time, tho).  Tables tools are
 *  around 4x slower than the compiled version so a
 *  the GenBufs aren't such a big performance hit for table stuff.
 *
 */
#include "gen-buf.h"

//#define BUF_TYPE			GenBuf *
#define BufGetByte( b)			GenBufGetByte (b)
#define BufGetSeg( b, lenPtr)		GenBufGetSeg (b, lenPtr)
#define BufCopy( dst, b, len)		GenBufCopy (dst, b, len)
#define BufSkip( b, len)		GenBufSkip (b, len)
#define BufPeekByte( b)			GenBufPeekByte (b)
#define BufPeekSeg( b, lenPtr)		GenBufPeekSeg (b, lenPtr)
#define BufPeekCopy( dst, b, len)	GenBufPeekCopy (dst, b, len)
#define BufPutByteRvs( b, byte)		GenBufPutByteRvs (b, byte)
#define BufPutSegRvs( b, data, len)	GenBufPutSegRvs (b, data, len)
#define BufReadError( b)		GenBufReadError (b)
#define BufWriteError( b)		GenBufWriteError (b)
#define BufFreeBufAndData( b)           GenBufFreeBufAndData(*(b))
#define BufResetInWriteRvsMode( b)      GenBufResetInWriteRvsMode(*(b))
#define BufResetInReadMode( b)          GenBufResetInReadMode((b))
#define BufSetWriteError(b, value)     GenBufSetWriteError(b, value)



#include "print.h"  /* for printing set up */

#endif /* conditional include */

