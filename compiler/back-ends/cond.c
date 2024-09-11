/*
 * compiler/back_ends/cond.c - generate conditional include for C(++) hdr files
 *
 * MS 92
 * Copyright (C) 1991, 1992 Michael Sample
 *            and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/back-ends/cond.c,v 1.1.1.1 2005/04/14 14:59:42 \ste Exp $
 * $Log: cond.c,v $
 * Revision 1.1.1.1  2005/04/14 14:59:42  \ste
 * no message
 *
 * Revision 1.4  2003/07/07 14:54:24  nicholar
 * Eliminated headers and cleaned up include references
 *
 * Revision 1.3  2002/06/21 15:18:44  leonberp
 * fixed exception bugs
 *
 * Revision 1.2  2000/10/24 14:54:43  rwc
 * Updated to remove high-level warnings (level 4 on MSVC++) for an easier build.
 * SOME warnings persist due to difficulty in modifying the SNACC compiler to
 * properly build clean source; also some files are built by Lex/Yacc.
 *
 * Revision 1.1.1.1  2000/08/21 20:36:04  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.1  1995/07/25  19:13:49  rj
 * PrintConditionalIncludeOpen() and PrintConditionalIncludeClose() moved from back-ends/c-gen/gen-code.[ch].
 *
 */

#include <string.h>
#include "../../c-lib/include/asn-incl.h"
#include "str-util.h"

void PrintConditionalIncludeOpen(FILE* f, char* fileName)
{
	char hdrFileDefSym[257];
	int i;

	strcpy_s(hdrFileDefSym, 256, RemovePath(fileName));
	for (i = 0; i < (int)strlen(hdrFileDefSym); i++)
		if (hdrFileDefSym[i] == '-' || hdrFileDefSym[i] == '.' || hdrFileDefSym[i] == '\\' || hdrFileDefSym[i] == '/')
			hdrFileDefSym[i] = '_';

	fprintf(f, "#ifndef _%s_\n", hdrFileDefSym);
	fprintf(f, "#define _%s_\n\n", hdrFileDefSym);
} /* PrintConditionalIncludeOpen */

void PrintConditionalIncludeClose(FILE* f, char* fileName)
{
	fprintf(f, "\n#endif\n");

} /* PrintConditionalIncludeClose */
