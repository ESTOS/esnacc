/*
 * compiler/core/snacc_util.h
 *
 * Copyright (C) 1992 Michael Sample and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/core/snacc-util.h,v 1.2 2006/10/24 12:54:41 \ste Exp $
 * $Log: snacc-util.h,v $
 * Revision 1.2  2006/10/24 12:54:41  \ste
 * bit string updated
 *
 * Revision 1.1.1.1  2005/04/14 14:59:43  \ste
 * no message
 *
 * Revision 1.10  2004/03/31 20:03:20  leonberp
 * resolved many gcc compile warnings
 *
 * Revision 1.9  2004/01/29 21:21:45  gronej
 * Took all Init() calls out of Clear() functions in generated code
 *
 * Revision 1.8  2003/07/28 11:11:24  colestor
 * Changes to complete handing of the "--snacc namespace" compiler directive.
 * Also, updates to handle ASN.1 constant integer tag designations for C++/C.
 *
 * Revision 1.7  2003/07/07 21:01:19  nicholar
 * no message
 *
 * Revision 1.6  2003/07/07 14:50:14  nicholar
 * Eliminated headers and cleaned up include references
 *
 * Revision 1.5  2003/04/29 21:07:03  leonberp
 * integerated Deepak's changes for IOB support
 *
 * Revision 1.4  2003/01/27 16:50:45  leonberp
 * added GetLastNamedNumberValue()
 *
 * Revision 1.3  2002/10/21 17:15:19  mcphersc
 * fixed long int
 *
 * Revision 1.2  2001/07/12 19:34:34  leonberp
 * Changed namespace to SNACC and added compiler options: -ns and -nons.  Also removed dead code.
 *
 * Revision 1.1.1.1  2000/08/21 20:36:02  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.3  1995/07/25  19:41:46  rj
 * changed `_' to `-' in file names.
 *
 * Revision 1.2  1994/09/01  00:46:41  rj
 * snacc_config.h's 2nd last macro, PrintErrLoc(), got here.
 *
 * Revision 1.1  1994/08/28  09:49:41  rj
 * first check-in. for a list of changes to the snacc-1.1 distribution please refer to the ChangeLog.
 *
 */

#include "asn1module.h"

#define PrintErrLoc( fileName, lineNo)\
			fprintf (errFileG, "%s(%ld) : ", fileName, (lineNo))

/*
 * macro to allocate room for str & null & put in give STR*
 */
#define SETUP_STR( strPtr, string)\
    (strPtr)->str = Malloc (strlen (string) + 1);\
    strcpy ((strPtr)->str, string);\
    (strPtr)->len = strlen (string) + 1


/*
 * Create a new list type such that each elmt has space
 * to hold a pointer
 */
#define NEWLIST()	AsnListNew (sizeof (void *))

/*
 *    macro to append an element to the end of linked list
 *    - helps on left recursion when order must be maintained
 *
 *  be careful of calling context if list is null
 *  that is, make sure the change to list is not lost.
 */
#define APPEND( elmt, list) \
{\
    void **tmpPtr;\
    if ((list) == NULL)\
        (list) = NEWLIST();\
    tmpPtr = (void **) AsnListAppend ((AsnList *)list);\
    *tmpPtr = (void *) (elmt);\
}

/*
 * like APPEND except puts elmt at head of  list
 */
#define PREPEND( elmt, list) \
{\
    void **tmpPtr;\
    if ((list) == NULL)\
        (list) = NEWLIST();\
    tmpPtr = (void **)AsnListPrepend ((AsnList *)list);\
    *tmpPtr = (void *) (elmt);\
}

#define CONCAT( list1, list2)\
{\
    AsnListConcat(list1, list2);\
}

BasicValue* GetLastNamedNumberValue(NamedNumberList *valueList);

void SetupType(Type **t, enum BasicTypeChoiceId typeId, unsigned long lineNum);

void SetupMacroType(Type **t, enum MacroTypeChoiceId macroTypeId, unsigned long lineNum);

void SetupValue(Value **v, enum BasicValueChoiceId valId, unsigned long lineNum);


void AddPrivateImportElmt(Module *m, char *name, char *refModuleName, long lineNo);

ImportElmt *LookupImportElmtInModule(Module *m, char *name, ImportModule **importModule);

ImportElmt *LookupImportElmtInImportElmtList(ImportElmtList *importElmtList, char *name);

ImportModule *LookupImportModule(Module *m, char *importModuleName);

TypeDef *LookupType(TypeDefList *t, char *typeName);

// Deepak: 04/Feb/2003
NamedType *LookupObjectClassFieldType(NamedTypeList *n, char *typeName);

// Deepak: 05/Mar/2003
WithSyntax* LookupObjectClassFieldTypeWithSyntax(WithSyntaxList *withSyntaxList, char *typeName, char* bError);

// Deepak: 05/Mar/2003
ObjectAssignment* LookupObjectClassObjectAssignment(ObjectAssignmentList *objAssignmentList, char *objName);

// Deepak: 11/Mar/2003
ObjectSetAssignment* LookupObjectClassObjectSetAssignment(ObjectSetAssignmentList *objSetAssignmentList, char *objSetName);

Module *LookupModule(ModuleList *m, char *modName, OID *oid);

NamedType* LookupFieldInType(Type* t, const char* fieldName);

Type* ResolveImportedType(Type *t);

Type* ParanoidGetType(Type *t);

enum BasicTypeChoiceId GetBuiltinType(Type *t);
enum BasicTypeChoiceId ParanoidGetBuiltinType(Type *t);


NamedNumberList *GetNamedElmts(Type *t);

NamedNumberList *GetAllNamedElmts(Type *t);

Type *GetParentS(Type *ancestor, Type *child);

ValueDef *LookupValue(ValueDefList *v, char *valueName);

Value *GetValue(Value *v);

int CompareOids(OID *oid1, OID *oid2);

int HasNamedElmts(Type *t);

int GetNumNamedElmts(Type *t);

int TagsAreIdentical(TagList *t1, TagList *t2);

int HasDefaultTag(Type *t);

int IsPrimitiveByDefOrRef(Type *t);

int IsPrimitiveByDef(Type *t);

int IsDefinedByLibraryType(Type *t);

int IsTypeRef(Type *t);

int IsNewType(Type *t);

int IsTailOptional(NamedTypeList *e);

int NextIsTailOptional(NamedTypeList *e);

int AllElmtsOptional(NamedTypeList *e);

AnyRefList **GetAnyRefListHndl(Type *t);

void AppendSubtype(Subtype **s, Subtype *newSubtype, enum SubtypeChoiceId op);


extern FILE* errFileG;		// Defined in snacc.c
