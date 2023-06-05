// file: .../c++-lib/src/hash.C
//
// This was borrowed from Don Acton and Terry Coatta's Raven Code.
// It has been modified somewhat.
//    - Mike Sample 92
//
// This is a set or routines that implements an extensible hashing
// algorithm. At the moment it assumes that all the hash codes are unique
// (ie. there are no collisions). For the way hash codes are currently being
// supplied this is not a bad assumption.
// The extensible hashing routine used is based on a multiway tree with
// each node in the tree being a fixed array of (2^n) size. At a given
// level, i, in the tree with the first level being level 0, bits
// i*n through i*n through (i+1)*n-1 are used as the index into the table.
// Each entry in the table is either NULL (unused) or a pointer to an
// object of type entry. The entry contains all the information about a
// hash entry. The entry also contains a field indicating whether or not this
// is a leaf node. If an entry isn't a leaf node then it references a table at
// at the next level and not a value. With the current implementation
// a 32 hash value is used and table sizes are 256. The algorithm used
// here is the same as the one used in the Set class of the Raven
// class system.
//
// Copyright (C) 1992 the University of British Columbia
//
// This library is free software; you can redistribute it and/or
// modify it provided that this copyright/license information is retained
// in original form.
//
// If you modify this file, you must clearly indicate your changes.
//
// This source code is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "../include/asn-incl.h"

_BEGIN_SNACC_NAMESPACE

/*
 *
 * From sdbm, an ndbm work-alike hashed database library
 * Author: oz@nexus.yorku.ca
 * Status: public domain.
 *
 * polynomial conversion ignoring overflows
 * [this seems to work remarkably well, in fact better
 * then the ndbm hash function. Replace at your own risk]
 * use: 65599   nice.
 *      65587   even better.
 *
 * [In one experiment, this function hashed 84165 symbols (English words
 * plus symbol table values) with no collisions. -bjb]
 *
 */

Hash MakeHash(const char* str, size_t len)
{
	Hash n = 0;

#define HASHC n = *str++ + 65587 * n

	if (len > 0)
	{
		int loop;
		loop = ((int)len + 8 - 1) >> 3;
		switch (len & (8 - 1))
		{
			case 0: /* very strange! - switch labels in do loop */
				do
				{
					HASHC;
					case 7:
						HASHC;
					case 6:
						HASHC;
					case 5:
						HASHC;
					case 4:
						HASHC;
					case 3:
						HASHC;
					case 2:
						HASHC;
					case 1:
						HASHC;
				} while (--loop);
		}
	}
	return n;
}

/* Creates and clears a new hash slot */
static HashSlot* NewHashSlot()
{
	HashSlot* foo;

	foo = new HashSlot;
	if (foo == NULL)
		return NULL;
	memset(foo, 0, sizeof(HashSlot));
	return foo;
}

/* Create a new cleared hash table */
static Table* NewTable()
{
	Table* new_table;

	//  new_table = new Table;
	// whose bug is it that gcc won't compile the above line?
	new_table = (Table*)new Table;
	if (new_table == NULL)
		return NULL;
	memset(new_table, 0, sizeof(Table));
	return new_table;
}

/* This routine is used to initialize the hash tables. When it is called
 * it returns a value which is used to identify which hash table
 * a particular request is to operate on.
 */
Table* InitHash()
{
	Table* table;
	table = NewTable();
	if (table == NULL)
		return 0;
	else
		return table;
}

/* When a hash collision occurs at a leaf slot this routine is called to
 * split the entry and add a new level to the tree at this point.
 */
static int SplitAndInsert(HashSlot* entry, void* element, Hash hash_value)
{

	if (((entry->table = NewTable()) == NULL) || !Insert(entry->table, entry->value, entry->hash >> INDEXSHIFT) || !Insert(entry->table, element, hash_value >> INDEXSHIFT))
		return false;

	entry->leaf = false;
	return true;
}

/* This routine takes a hash table identifier, an element (value) and the
 * coresponding hash value for that element and enters it into the table
 * assuming it isn't already there.
 */
int Insert(Table* table, void* element, Hash hash_value)
{
	HashSlot* entry;

	entry = (HashSlot*)(*table)[hash_value & INDEXMASK];

	if (entry == NULL)
	{
		/* Need to add this element here */
		entry = NewHashSlot();
		if (entry == NULL)
			return false;
		entry->leaf = true;
		entry->value = element;
		entry->hash = hash_value;
		(*table)[hash_value & INDEXMASK] = entry;
		return true;
	}

	if (hash_value == entry->hash)
		return false;

	if (entry->leaf)
		return SplitAndInsert(entry, element, hash_value);

	return Insert(entry->table, element, hash_value >> INDEXSHIFT);
}

/* This routine looks to see if a particular hash value is already stored in
 * the table. It returns true if it is and false otherwise.
 */
int CheckFor(Table* table, Hash hash)
{
	HashSlot* entry;

	entry = (HashSlot*)table[hash & INDEXMASK];

	if (entry == NULL)
		return false;
	if (entry->leaf)
		return entry->hash == hash;
	return CheckFor(entry->table, hash >> INDEXSHIFT);
}

/* In addition to checking for a hash value in the tree this function also
 * returns the coresponding element value into the space pointed to by
 * the value parameter. If the hash value isn't found false is returned
 * the the space pointed to by value is not changed.
 */
int CheckForAndReturnValue(Table* table, Hash hash, void** value)
{
	HashSlot* entry;
	if (table)
	{
		entry = (HashSlot*)(*table)[hash & INDEXMASK];

		if (entry == NULL)
			return false;

		if (entry->leaf)
		{
			if (entry->hash == hash)
			{
				*value = entry->value;
				return true;
			}
			else
				return false;
		}
		return CheckForAndReturnValue(entry->table, hash >> INDEXSHIFT, value);
	}
	else
		return false;
}

_END_SNACC_NAMESPACE
