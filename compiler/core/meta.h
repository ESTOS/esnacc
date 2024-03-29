/*
 * file: compiler/core/meta.h
 *
 * Copyright 1994 1995 Robert Joop <rj@rainbow.in-berlin.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program and the associated libraries are distributed in the hope
 * that they will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License and GNU Library General
 * Public License for more details.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/core/meta.h,v 1.1.1.1 2005/04/14 14:59:43 \ste Exp $
 * $Log: meta.h,v $
 * Revision 1.1.1.1  2005/04/14 14:59:43  \ste
 * no message
 *
 * Revision 1.2  2001/07/12 19:34:31  leonberp
 * Changed namespace to SNACC and added compiler options: -ns and -nons.  Also removed dead code.
 *
 * Revision 1.1.1.1  2000/08/21 20:36:00  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.4  1995/09/07  19:14:26  rj
 * enum MetaNameStyle introduced.
 *
 * Revision 1.3  1995/08/17  15:00:12  rj
 * the PDU flag belongs to the metacode, not only to the tcl interface. (type and variable named adjusted)
 *
 * Revision 1.1  1995/07/27  10:54:11  rj
 * new file
 */

#if META

typedef enum
{
	META_off = 0,		/* metacode generation disabled */
	META_asn1_names,	/* names as defined in .asn1 file */
	META_backend_names, /* names as being used by the backend code */
} MetaNameStyle;

typedef struct
{
	const char* srcfn;
	FILE* srcfp;
} Meta;

typedef struct MetaPDU
{
	const char *module, *type;
	int used;

	struct MetaPDU* next;
} MetaPDU;

extern int isMetaPDU PROTO((const char* module, const char* type, MetaPDU* pdus));

#endif /* META */
