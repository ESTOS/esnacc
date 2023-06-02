/*
 * compiler/core/mem.c - used for allocating the components of the Module
 *         data structure.  The program expects 0'ed memory
 *         to be returned by Malloc - this initializes ptrs
 *         to NULL.
 *
 *         If there is not enough memory the Malloc exits
 *         (Callers of Malloc will never get a NULL return value)
 *
 * Copyright (C) 1991, 1992 Michael Sample
 *            and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "snacc.h"

extern FILE* errFileG;		// Defined in snacc.c


void*
Malloc PARAMS((size), size_t size)
{
	void* retVal = malloc(size);

	if (retVal == NULL)
	{
		fprintf(errFileG, "out of memory! bye!\n");
		fprintf(errFileG, "tried to allocate %zd byes\n", size);
		exit(1);
	}

	memset(retVal, 0, size);
	return retVal;

}  /* Malloc */

void* Realloc PARAMS((ptr, size_t newsize),
	void* ptr _AND_
	int newsize)
{
	void* retval = realloc(ptr, newsize);

	if (retval == NULL)
	{
		fprintf(errFileG, "out of memory! bye!\n");
		fprintf(errFileG, "tried to reallocate %d byes\n", newsize);
		exit(1);
	}

	return retval;
}

void Free PARAMS((ptr),
	void* ptr)
{
	free(ptr);
}
