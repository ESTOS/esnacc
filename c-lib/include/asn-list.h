/*
 * asn_list.h
 *
 *  ---------
 *  | AsnList |
 *  |  last |-------------------------------------------|
 *  |  curr |--------------------------|                |
 *  |  first|--------|                 |                |
 *  ---------        |                 |                |
 *                   V                 V                V
 *                ---------        ---------        ---------
 *                |AsnListNode       |AsnListNode       |AsnListNode
 *                | next  |---...->|  next |--...-->| next  |-----|i.
 *         .i|----| prev  |<--...--|  prev |<--...--| prev  |
 *                | data  |        |  data |        | data  |
 *                ---------        ---------        ---------
 *
 * Originally by Murray Goldberg
 * Modified for ASN.1 use.
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

#ifndef _asn_list_h_
#define _asn_list_h_

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct AsnListNode
	{
		struct AsnListNode* prev;
		struct AsnListNode* next;
		void* data; /* this must be the last field of this structure  */
	} AsnListNode;

	typedef struct AsnList
	{
		AsnListNode* first;
		AsnListNode* last;
		AsnListNode* curr;
		int count;	  /* number of elements in list               */
		int dataSize; /* space required in each node for the data */
	} AsnList;

#define FOR_EACH_LIST_ELMT(elmt, al)                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
	if (al)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
		for ((al)->curr = (al)->first; (al)->curr && (((elmt) = (al)->curr->data) != NULL); (al)->curr = (al)->curr->next)

#define FOR_EACH_LIST_ELMT_NOITERATE(elmt, al)                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
	if (al)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
		for ((al)->curr = (al)->first; (al)->curr && (((elmt) = (al)->curr->data) != NULL);)

#define FOR_EACH_LIST_ELMT_NEW(elmt, al)                                                                                                                                                                                                                                                                                                                                                                                                                                                                           \
	if (al)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
		for ((al)->curr = (al)->first; (al)->curr && (((elmt) = (al)->curr->data) != NULL); (al)->curr = (al)->curr->next)                                                                                                                                                                                                                                                                                                                                                                                         \
		{

#define FOR_EACH_LIST_ELMT_RVS(elmt, al)                                                                                                                                                                                                                                                                                                                                                                                                                                                                           \
	if (al)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
		for ((al)->curr = (al)->last; (al)->curr && (((elmt) = (al)->curr->data) != NULL); (al)->curr = (al)->curr->prev)

#define FOR_REST_LIST_ELMT(elmt, al)                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
	if (al)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
		for (; (al)->curr && (((elmt) = (al)->curr->data) != NULL); (al)->curr = (al)->curr->next)

#define HAS_NEXT(list) if (list->curr->next && list->curr->next->data)

	/*
	 * The following macros return the pointer stored in the
	 * data part of the listNode.  The do not change the current
	 * list pointer.
	 */
#define CURR_LIST_ELMT(al) ((al)->curr->data)
#define NEXT_LIST_ELMT(al) ((al)->curr->next->data)
#define PREV_LIST_ELMT(al) ((al)->curr->prev->data)
#define LAST_LIST_ELMT(al) ((al)->last->data)
#define FIRST_LIST_ELMT(al) ((al)->first->data)
#define LIST_EMPTY(al) ((al)->count == 0)
#define LIST_COUNT(al) ((al)->count)

	/*
	 * list nodes are the parts of the list that contain ptrs/data
	 * to/of the list elmts.
	 */
#define CURR_LIST_NODE(al) ((al)->curr)
#define FIRST_LIST_NODE(al) ((al)->first)
#define LAST_LIST_NODE(al) ((al)->last)
#define PREV_LIST_NODE(al) ((al)->curr->prev)
#define NEXT_LIST_NODE(al) ((al)->curr->next)
#define SET_CURR_LIST_NODE(al, listNode) ((al)->curr = ((AsnListNode*)listNode))

	void AsnListRemove PROTO((AsnList*));
	void* AsnListAdd PROTO((AsnList*));
	void* AsnListInsert PROTO((AsnList*));
	void AsnListInit PROTO((AsnList * list, int dataSize));
	AsnList* AsnListNew PROTO((int));
	void* AsnListPrev PROTO((AsnList*));
	void* AsnListNext PROTO((AsnList*));
	void* AsnListLast PROTO((AsnList*));
	void* AsnListFirst PROTO((AsnList*));
	void* AsnListPrepend PROTO((AsnList*));
	void* AsnListAppend PROTO((AsnList*));
	void* AsnListCurr PROTO((AsnList*));
	AsnList* AsnListSort PROTO((AsnList * list, int((*cmp)(const void*, const void*))));
	int AsnListCount PROTO((AsnList*));
	AsnList* AsnListConcat PROTO((AsnList*, AsnList*));
	long GetAsnListElmtIndex PROTO((void* elmt, AsnList* list));
	void AsnListFree PROTO((AsnList*));
	void* GetAsnListElmt PROTO((AsnList * list, unsigned int index));

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* conditional include */
