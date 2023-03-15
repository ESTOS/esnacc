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

#include "../../snacc.h"

#if STDC_HEADERS
#include <stdlib.h>
#else
#include <string.h>
#include <malloc.h>
#endif

#include <stdio.h>

#include "../include/mem.h"
#include <memory.h>

void*
Malloc PARAMS ((size), size_t size)
{
    void *retVal = malloc (size);

    if (retVal == NULL)
    {
        fprintf (stderr, "out of memory! bye!\n");
        fprintf (stderr, "tried to allocate %zd byes\n", size);
        exit (1);
    }

    memset(retVal, 0, size);
    return retVal;

}  /* Malloc */

void *Realloc PARAMS ((ptr, newsize),
    void *ptr _AND_
	size_t newsize)
{
    void *retval = realloc (ptr, newsize);

    if (retval == NULL)
    {
        fprintf (stderr, "out of memory! bye!\n");
        fprintf (stderr, "tried to reallocate %zd byes\n", newsize);
        exit (1);
    }

    return retval;
}

void Free PARAMS ((ptr),
    void *ptr)
{
    free (ptr);
}
