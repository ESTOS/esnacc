/*
 * hash.h
 *
 * Based on hashing stuff from UBC Raven Code (Terry Coatta & Don Acton)
 *
 * MS 92
 * Copyright (C) 1992 the University of British Columbia
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

#ifndef _asn_hash_h_
#define _asn_hash_h_

#ifdef __cplusplus
extern "C" {
#endif


#define TABLESIZE 256
#define INDEXMASK 0xFF
#define INDEXSHIFT 8

typedef void *Table[TABLESIZE];

typedef unsigned int Hash;

typedef struct HashSlot
{
  int    leaf;
  Hash   hash;
  void  *value;
  Table *table;
} HashSlot;

Hash MakeHash PROTO ((char *str, size_t len));

Table *InitHash();

int Insert PROTO ((Table *table, void *element, Hash hash));

int CheckFor PROTO ((Table *table, Hash hash));

int CheckForAndReturnValue PROTO ((Table *table, Hash hash, void **value));

#ifdef __cplusplus
}
#endif

#endif /* conditional include */

