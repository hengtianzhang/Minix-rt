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

/* adjust *@size so that (@base + *@size) doesn't overflow, return new size */
static inline phys_addr_t memblock_cap_size(phys_addr_t base, phys_addr_t *size)
{
	return *size = min(*size, PHYS_ADDR_MAX - base);
}

/*
 * Address comparison utilities
 */
static u64 __init_memblock memblock_addrs_overlap(phys_addr_t base1, phys_addr_t size1,
				       phys_addr_t base2, phys_addr_t size2)
{
	return ((base1 < (base2 + size2)) && (base2 < (base1 + size1)));
}

bool __init_memblock memblock_overlaps_region(struct memblock_type *type,
					phys_addr_t base, phys_addr_t size)
{
	u64 i;

	for (i = 0; i < type->cnt; i++)
		if (memblock_addrs_overlap(base, size, type->regions[i].base,
					   type->regions[i].size))
			break;
	return i < type->cnt;
}
