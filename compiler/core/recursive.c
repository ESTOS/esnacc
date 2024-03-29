/*
 * compiler/core/recursive.c - finds and marks the recursive types in a module.
 *
 * ALSO:
 * prints msgs for infinitely recursive types (ie recursive component
 * is not OPTIONAL, nor a CHOICE elmt, nor a SET OF nor a SEQ OF elmt.
 * (OPTIONALs can be left out, CHOICE elements have alternatives (hopefully),
 * and SET OF and SEQUENCE OF values can have zero elements)
 *
 * prints msg for recursive types that hold no real information
 *     Foo ::= SET OF Foo (sets of sets of .... of empty sets)
 *
 * finds bogus recursive types (hold no info) (same as above)
 *  A ::= B
 *  B ::= C
 *  D ::= A
 *
 * MS 92
 *
 * Copyright (C) 1991, 1992 Michael Sample
 *            and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/core/recursive.c,v 1.1.1.1 2005/04/14 14:59:43 \ste Exp $
 * $Log: recursive.c,v $
 * Revision 1.1.1.1  2005/04/14 14:59:43  \ste
 * no message
 *
 * Revision 1.7  2004/04/06 15:13:41  gronej
 * *** empty log message ***
 *
 * Revision 1.6  2004/03/25 19:20:17  gronej
 * fixed some linux warnings
 *
 * Revision 1.5  2003/07/07 14:50:14  nicholar
 * Eliminated headers and cleaned up include references
 *
 * Revision 1.4  2002/09/16 16:50:23  mcphersc
 * Fixed warnings
 *
 * Revision 1.3  2002/09/04 18:31:39  vracarl
 * got rid of c++ comments
 *
 * Revision 1.2  2000/10/24 14:54:55  rwc
 * Updated to remove high-level warnings (level 4 on MSVC++) for an easier build.
 * SOME warnings persist due to difficulty in modifying the SNACC compiler to
 * properly build clean source; also some files are built by Lex/Yacc.
 *
 * Revision 1.1.1.1  2000/08/21 20:36:01  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.3  1995/07/25 19:41:43  rj
 * changed `_' to `-' in file names.
 *
 * Revision 1.2  1994/09/01  00:43:10  rj
 * snacc_config.h removed; recursive.h includet.
 *
 * Revision 1.1  1994/08/28  09:49:35  rj
 * first check-in. for a list of changes to the snacc-1.1 distribution please refer to the ChangeLog.
 *
 */

#include "../../c-lib/include/asn-incl.h"
#include "asn1module.h"
#include "snacc-util.h"

void MkRecTypeDef PROTO((Module * m, TypeDef* td));
void MkRecType PROTO((Module * m, TypeDef* td, Type* t, int optional, int empty));

extern FILE* errFileG; // Defined in snacc.c

void MarkRecursiveTypes PARAMS((m), Module* m)
{
	TypeDef* td;

	/* first set all typedef as un-visited */
	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		td->visited = FALSE;
		td->tmpRefCount = 0;
	}

	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		MkRecTypeDef(m, td);
	}
} /* MarkRecursiveTypes */

void MkRecTypeDef PARAMS((m, td), Module* m _AND_ TypeDef* td)
{
	MkRecType(m, td, td->type, 0, 1);
} /* MkRecTypeDef */

/*
 * cruise through aggregate types and type refs looking for
 * a type ref to the original type def, td.  If is a ref to
 * the td, then mark the td as recusive.
 *
 * the optional flag is set if the current type branch is
 * optional via an OPTIONAL SET/SEQ elmt, CHOICE elmt, SET OF elmt
 * or SEQ OF elmt.
 *
 * the empty flag is initially TRUE and remains true until a
 * non-type reference type is encountered
 */
void MkRecType PARAMS((m, td, t, optional, empty), Module* m _AND_ TypeDef* td _AND_ Type* t _AND_ int optional _AND_ int empty)
{
	int newOptional;
	NamedType* e;

	switch (t->basicType->choiceId)
	{
		case BASICTYPE_CHOICE:
			if (AsnListCount(t->basicType->a.choice) > 1)
			{
				empty = 0;
				optional = 1;
			}
			FOR_EACH_LIST_ELMT(e, t->basicType->a.choice)
			{
				MkRecType(m, td, e->type, optional, empty);
			}
			break;

		case BASICTYPE_SET:
		case BASICTYPE_SEQUENCE:
			empty = 0;

			FOR_EACH_LIST_ELMT(e, t->basicType->a.set)
			{
				newOptional = optional || (e->type->optional) || (e->type->defaultVal != NULL);
				MkRecType(m, td, e->type, newOptional, empty);
			}
			break;

		case BASICTYPE_SETOF:
		case BASICTYPE_SEQUENCEOF:
			empty = 0;	  /* since an empty set is actual data */
			optional = 1; /* since SET OF and SEQ OF's can be empty */
			MkRecType(m, td, t->basicType->a.setOf, optional, empty);
			break;

		case BASICTYPE_LOCALTYPEREF:
		case BASICTYPE_IMPORTTYPEREF:

			/*
			 * check if ref to original type def & mark recursive if so.
			 */
			/*            if ((strcmp (t->basicType->a.localTypeRef->typeName, td->definedName) == 0) && (t->basicType->a.localTypeRef->module == m))
			 easier to just check ptrs!
			*/
			if (t->basicType->a.localTypeRef->link == td)
			{
				td->recursive = 1;
				if (empty)
				{
					PrintErrLoc(m->asn1SrcFileName, (long)td->type->lineNo);
					fprintf(errFileG, "WARNING: Type \"%s\" appears to be infinitely recursive and can hold no values! (circular type references)\n", td->definedName);
				}
				else if (!optional)
				{
					PrintErrLoc(m->asn1SrcFileName, (long)t->lineNo);
					fprintf(errFileG, "WARNING: Type \"%s\" appears to be infinitely recursive! (infinitely sized values)\n", td->definedName);
				}
			}

			/*
			 * else follow this type reference if we aren't in it already
			 * (ie another recursive type in td)
			 */
			else if (t->basicType->a.localTypeRef->link->tmpRefCount == 0)
			{
				/*
				 * mark this typedef as 'entered' to
				 * detect when looping in a recusive type that is contained
				 * in the original td (use tmpRefCount)
				 */
				t->basicType->a.localTypeRef->link->tmpRefCount = 1;

				newOptional = optional || (t->optional) || (t->defaultVal != NULL);
				MkRecType(m, td, t->basicType->a.localTypeRef->link->type, newOptional, empty);

				/*
				 * un-mark this type since finished with it
				 * for recursive ref's to td
				 */
				t->basicType->a.localTypeRef->link->tmpRefCount = 0;
			}
			break;

		default:
			break;
			/*
			 * default: other types are not aggregate and
			 * do not make recursive refs - they can be ignored
			 */
	}
} /* MkRecType */
