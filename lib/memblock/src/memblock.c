/*
 * Procedures for maintaining information about logical memory blocks.
 *
 * Peter Bergner, IBM Corp.	June 2001.
 * Copyright (C) 2001 Peter Bergner.
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 */
#include <base/string.h>
#include <base/common.h>

#include <memblock/memblock.h>

void __init_memblock memblock_init(struct memblock *mb)
{
	memset(mb->memory.regions, 0, sizeof (mb->memory.regions));
	mb->memory.cnt = 1;
	mb->memory.max = INIT_MEMBLOCK_REGIONS;
	strcpy(mb->memory.name, "memory");

	memset(mb->reserved.regions, 0, sizeof (mb->reserved.regions));
	mb->reserved.cnt = 1;
	mb->reserved.max = INIT_MEMBLOCK_REGIONS;
	strcpy(mb->reserved.name, "reserved");

	mb->bottom_up = false;

	mb->current_limit = MEMBLOCK_ALLOC_ANYWHERE;
}

enum memblock_flags __init_memblock	choose_memblock_flags(void)
{
	return MEMBLOCK_NONE;
}
