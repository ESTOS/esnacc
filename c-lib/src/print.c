/*
 * print.c - library routines for printing ASN.1 values.
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
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/c-lib/src/print.c,v 1.1.1.1 2005/04/14 14:59:42 \ste Exp $
 *
 */

#include "../include/asn-config.h"
#include "../include/print.h"

void
Indent PARAMS((f, i),
	FILE* f _AND_
	unsigned int i)
{
	for (; i > 0; i--)
		fputs("   ", f);
}

void Asn1DefaultErrorHandler PARAMS((str, severity),
	char* str _AND_
	int severity)
{
	/*  fprintf(stderr,"%s",str); DAD - temp removing for now*/
}

static Asn1ErrorHandler asn1CurrentErrorHandler = Asn1DefaultErrorHandler;

void
Asn1Error PARAMS((str),
	char* str)
{
	(*asn1CurrentErrorHandler)(str, 1);
}

void
Asn1Warning PARAMS((str),
	char* str)
{
	(*asn1CurrentErrorHandler)(str, 0);
}

Asn1ErrorHandler
Asn1InstallErrorHandler PARAMS((handler),
	Asn1ErrorHandler handler)
{
	Asn1ErrorHandler former = asn1CurrentErrorHandler;
	asn1CurrentErrorHandler = handler;
	return former;
}

