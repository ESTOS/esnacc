/*
 * compiler/core/exports.h -
 *
 *      ExportElmt list set up during  parse.
 *      (not kept in Module data struct)
 *
 *  SetExports runs through type, value & macro defs and sets the
 *  exports flag accordingly.
 *
 *  the exportsParsed boolean means whether the symbol "EXPORTS"
 *  was parsed - since if EXPORTS was parsed and the export list
 *  is empty, NOTHING is exported, otherwise if the "EXPORTS"
 *  symbol was not parsed (export list is empty) then EVERYTHING
 *  is exported
 *
 * Mike Sample
 * 91/09/04
 *
 * Copyright (C) 1991, 1992 Michael Sample
 *            and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/core/exports.h,v 1.1.1.1 2005/04/14 14:59:43 \ste Exp $
 * $Log: exports.h,v $
 * Revision 1.1.1.1  2005/04/14 14:59:43  \ste
 * no message
 *
 * Revision 1.2  2001/07/12 19:34:28  leonberp
 * Changed namespace to SNACC and added compiler options: -ns and -nons.  Also removed dead code.
 *
 * Revision 1.1.1.1  2000/08/21 20:36:00  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.2  1994/10/08 03:48:43  rj
 * since i was still irritated by cpp standing for c++ and not the C preprocessor, i renamed them to cxx (which is one known suffix for C++ source files). since the standard #define is __cplusplus, cplusplus would have been the more obvious choice, but it is a little too long.
 *
 * Revision 1.1  1994/08/28  09:49:09  rj
 * first check-in. for a list of changes to the snacc-1.1 distribution please refer to the ChangeLog.
 *
 */

typedef struct ExportElmt
{
	char* name;
	long int lineNo;
	struct ExportElmt* next;
} ExportElmt;

void SetExports PROTO((Module * m, ExportElmt* e, int exportsParsed));
