/*
 *  compiler/core/mem.h
 *
 * MS 91/08/03
 *
 * Copyright (C) 1991, 1992 Michael Sample
 *            and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _snacc_mem_h_
#define _snacc_mem_h_

#ifdef __cplusplus
extern "C" {
#endif

	void* Malloc PROTO((size_t size));
	void* Realloc PROTO((void* ptr, size_t newsize));
	void	Free PROTO((void* ptr));

	/* malloc type */
#define MT( type)	(type *)Malloc (sizeof (type))


#ifdef __cplusplus
}
#endif

#endif

