/*
 * compiler/core/define.h
 *
 * Copyright (C) 1991, 1992 Michael Sample
 *            and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/core/define.h,v 1.1.1.1 2005/04/14 14:59:43 \ste Exp $
 * $Log: define.h,v $
 * Revision 1.1.1.1  2005/04/14 14:59:43  \ste
 * no message
 *
 * Revision 1.4  2004/03/31 20:03:19  leonberp
 * resolved many gcc compile warnings
 *
 * Revision 1.3  2001/07/12 19:34:27  leonberp
 * Changed namespace to SNACC and added compiler options: -ns and -nons.  Also removed dead code.
 *
 * Revision 1.2  2000/10/24 14:54:49  rwc
 * Updated to remove high-level warnings (level 4 on MSVC++) for an easier build.
 * SOME warnings persist due to difficulty in modifying the SNACC compiler to
 * properly build clean source; also some files are built by Lex/Yacc.
 *
 * Revision 1.1.1.1  2000/08/21 20:35:59  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.2  1994/10/08 03:48:35  rj
 * since i was still irritated by cpp standing for c++ and not the C preprocessor, i renamed them to cxx (which is one known suffix for C++ source files). since the standard #define is __cplusplus, cplusplus would have been the more obvious choice, but it is a little too long.
 *
 * Revision 1.1  1994/08/28  09:48:59  rj
 * first check-in. for a list of changes to the snacc-1.1 distribution please refer to the ChangeLog.
 *
 */
#ifndef _DefinedObjInclude
#define _DefinedObjInclude

typedef struct DefinedObj
{
	void* obj;
	struct DefinedObj* next;
} DefinedObj;


typedef int (*CmpObjsRoutine)(void* obj1, void* obj2);
typedef int (*FreeObjRoutine)(void* obj);

int StrObjCmp(void* s1, void* s2);

int IntObjCmp(void* s1, void* s2);

int OidObjCmp(void* o1, void* o2);

int ObjPtrCmp(void* s1, void* s2);

DefinedObj* NewObjList();

void DefineObj(DefinedObj** l, const void* obj);

void UndefineObj(DefinedObj** l, void* obj, CmpObjsRoutine cmpRoutine);

int ObjIsDefined(DefinedObj* l, void* obj, CmpObjsRoutine cmp);

void FreeDefinedObjs(DefinedObj** l);

void FreeDefinedObjsAndContent(DefinedObj** l);

#endif
