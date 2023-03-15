/*
 * .../c-lib/src/nibble-alloc.c - fast mem allocation for decoded values
 *
 * MS Dec 31/91
 *
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

#include <string.h>
#include "../include/asn-config.h"
#include "../include/nibble-alloc.h"


NibbleMem *nmG = NULL;

void
InitNibbleMem PARAMS ((initialSize, incrementSize),
    unsigned long initialSize _AND_
    unsigned long incrementSize)
{
    NibbleMem *nm;

    nm = (NibbleMem*) malloc (sizeof (NibbleMem));
    nm->incrementSize = incrementSize;

    nm->currNibbleBuf = nm->firstNibbleBuf = (NibbleBuf*)malloc (sizeof (NibbleBuf));
    nm->firstNibbleBuf->curr = nm->firstNibbleBuf->start = (char*) malloc (initialSize);
    nm->firstNibbleBuf->end = nm->firstNibbleBuf->start + initialSize;
    nm->firstNibbleBuf->next = NULL;
    memset(nm->currNibbleBuf->start, 0, initialSize);

    nmG = nm;/* set global */

}  /* InitNibbleAlloc */


/*
 * alloc new nibble buf, link in, reset to curr nibble buf
 */
void
ServiceNibbleFault PARAMS ((size),
    unsigned long size)
{
    NibbleMem *nm;
    unsigned long newBufSize;

    nm = nmG;

    if (size > nm->incrementSize)
        newBufSize = size;
    else
        newBufSize = nm->incrementSize;

    nm->currNibbleBuf->next = (NibbleBuf*) malloc (sizeof (NibbleBuf));
    nm->currNibbleBuf = nm->currNibbleBuf->next;
    nm->currNibbleBuf->curr = nm->currNibbleBuf->start = (char*) malloc (newBufSize);
    nm->currNibbleBuf->end = nm->currNibbleBuf->start + newBufSize;
    nm->currNibbleBuf->next = NULL;
    memset(nm->currNibbleBuf->start, 0, newBufSize);
} /* serviceNibbleFault */



/*
 * returns requested space filled with zeros
 */
void*
NibbleAlloc PARAMS ((size),
    unsigned long size)
{
    NibbleMem *nm;
    char *retVal;
    unsigned long ndiff;

    nm = nmG;
    
    /* DAD - although error checking on the mallocs during
     * ServiceNibbleFault could be more robust, for now i'm
     * just going to avoid allocating really huge amounts
     * of memory (which can occur if the ASN.1 data has
     * become corrupted, and you were trying to decode).
     */
    if(size > 1024*1024)	/* say roughly a meg for now */
    	return(0);

    if ((nm->currNibbleBuf->end - nm->currNibbleBuf->curr) < (int)size)
         ServiceNibbleFault (size);

    retVal = nm->currNibbleBuf->curr;

    /*
     * maintain word alignment
     */
    ndiff = size % sizeof (long);
    if (ndiff != 0)
    {
        nm->currNibbleBuf->curr += size + sizeof (long) - ndiff;

        /*
         * this is a fix from Terry Sullivan <FCLTPS@nervm.nerdc.ufl.edu>
         *
         * makes sure curr does not go past the end ptr
         */
        if (nm->currNibbleBuf->curr > nm->currNibbleBuf->end)
            nm->currNibbleBuf->curr = nm->currNibbleBuf->end;
    }
    else
        nm->currNibbleBuf->curr += size;

    return retVal;
}  /* NibbleAlloc */



/*
 * frees all nibble buffers except the first,
 * resets the first to empty and zero's it
 */
void
ResetNibbleMem()
{
    NibbleMem *nm;
    NibbleBuf *tmp;
    NibbleBuf *nextTmp;

    nm = nmG;

    /*
     * reset first nibble buf
     */
    memset(nm->firstNibbleBuf->start, 0, nm->firstNibbleBuf->curr - nm->firstNibbleBuf->start);

    nm->firstNibbleBuf->curr = nm->firstNibbleBuf->start;

    /*
     * free incrementally added nibble bufs
     */
    for (tmp = nm->firstNibbleBuf->next; tmp != NULL; )
    {
        free (tmp->start);
        nextTmp = tmp->next;
        free (tmp);
        tmp = nextTmp;
    }

    /* From ftp://ftp.cs.ubc.ca/pub/local/src/snacc/bugs-in-1.1 */
    nm->firstNibbleBuf->next = NULL;
    nm->currNibbleBuf = nm->firstNibbleBuf;

} /* ResetNibbleMem */


/*
 * frees all nibble buffers, closing this
 * NibbleMem completely
 */
void
ShutdownNibbleMem()
{
    NibbleMem *nm;
    NibbleBuf *tmp;
    NibbleBuf *nextTmp;

    nm = nmG;
    nmG = NULL;
    /*
     * free nibble bufs
     */
	if (nm == NULL)
		return;
    for (tmp = nm->firstNibbleBuf; tmp != NULL; )
    {
        free (tmp->start);
        nextTmp = tmp->next;
        free (tmp);
        tmp = nextTmp;
    }

    free (nm);
} /* ShutdownNibbleMem */
