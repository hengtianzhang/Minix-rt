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
#include <base/errno.h>

#include <memalloc/memblock.h>

int memblock_debug __initdata_memblock;

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

/**
 * __memblock_find_range_bottom_up - find free area utility in bottom-up
 * @start: start of candidate range
 * @end: end of candidate range, can be %MEMBLOCK_ALLOC_ANYWHERE or
 *       %MEMBLOCK_ALLOC_ACCESSIBLE
 * @size: size of free area to find
 * @align: alignment of free area to find
 * @flags: pick from blocks based on memory attributes
 *
 * Utility called from __memblock_find_in_range(), find free area bottom-up.
 *
 * Return:
 * Found address on success, 0 on failure.
 */
static phys_addr_t __init_memblock
__memblock_find_range_bottom_up(struct memblock *mb, phys_addr_t start, phys_addr_t end,
				phys_addr_t size, phys_addr_t align,
				enum memblock_flags flags)
{
	phys_addr_t this_start, this_end, cand;
	u64 i;

	for_each_free_mem_range(mb, i, flags, &this_start, &this_end) {
		this_start = clamp(this_start, start, end);
		this_end = clamp(this_end, start, end);

		cand = round_up(this_start, align);
		if (cand < this_end && this_end - cand >= size)
			return cand;
	}

	return 0;
}

/**
 * __memblock_find_range_top_down - find free area utility, in top-down
 * @start: start of candidate range
 * @end: end of candidate range, can be %MEMBLOCK_ALLOC_ANYWHERE or
 *       %MEMBLOCK_ALLOC_ACCESSIBLE
 * @size: size of free area to find
 * @align: alignment of free area to find
 * @flags: pick from blocks based on memory attributes
 *
 * Utility called from __memblock_find_in_range(), find free area top-down.
 *
 * Return:
 * Found address on success, 0 on failure.
 */
static phys_addr_t __init_memblock
__memblock_find_range_top_down(struct memblock *mb, phys_addr_t start, phys_addr_t end,
			       phys_addr_t size, phys_addr_t align,
			       enum memblock_flags flags)
{
	phys_addr_t this_start, this_end, cand;
	u64 i;

	for_each_free_mem_range_reverse(mb, i, flags, &this_start, &this_end) {
		this_start = clamp(this_start, start, end);
		this_end = clamp(this_end, start, end);

		if (this_end < size)
			continue;

		cand = round_down(this_end - size, align);
		if (cand >= this_start)
			return cand;
	}

	return 0;
}

/**
 * __memblock_find_in_range - find free area in given range
 * @size: size of free area to find
 * @align: alignment of free area to find
 * @start: start of candidate range
 * @end: end of candidate range, can be %MEMBLOCK_ALLOC_ANYWHERE or
 *       %MEMBLOCK_ALLOC_ACCESSIBLE
 * @flags: pick from blocks based on memory attributes
 *
 * Find @size free area aligned to @align in the specified range.
 *
 * When allocation direction is bottom-up, the @start should be greater
 * than the end of the kernel image. Otherwise, it will be trimmed. The
 * reason is that we want the bottom-up allocation just near the kernel
 * image so it is highly likely that the allocated memory and the kernel
 * will reside.
 *
 * If bottom-up allocation failed, will try to allocate memory top-down.
 *
 * Return:
 * Found address on success, 0 on failure.
 */
phys_addr_t __init_memblock __memblock_find_in_range(struct memblock *mb,
					phys_addr_t size,
					phys_addr_t align, phys_addr_t start,
					phys_addr_t end,
					enum memblock_flags flags)
{
	phys_addr_t ret;

	/* pump up @end */
	if (end == MEMBLOCK_ALLOC_ACCESSIBLE)
		end = mb->current_limit;

	/* avoid allocating the first page */
	start = max_t(phys_addr_t, start, PAGE_SIZE);
	end = max(start, end);

	/*
	 * try bottom-up allocation only when bottom-up mode
	 * is set and @end is above the kernel image.
	 */
	if (memblock_bottom_up(mb)) {
		phys_addr_t bottom_up_start = start;

		/* ok, try bottom-up allocation first */
		ret = __memblock_find_range_bottom_up(mb, bottom_up_start, end,
						      size, align, flags);
		if (ret)
			return ret;
	}

	return __memblock_find_range_top_down(mb, start, end, size, align,
					      flags);
}

/**
 * memblock_find_in_range - find free area in given range
 * @start: start of candidate range
 * @end: end of candidate range, can be %MEMBLOCK_ALLOC_ANYWHERE or
 *       %MEMBLOCK_ALLOC_ACCESSIBLE
 * @size: size of free area to find
 * @align: alignment of free area to find
 *
 * Find @size free area aligned to @align in the specified range.
 *
 * Return:
 * Found address on success, 0 on failure.
 */
phys_addr_t __init_memblock memblock_find_in_range(struct memblock *mb,
					phys_addr_t start,
					phys_addr_t end, phys_addr_t size,
					phys_addr_t align)
{
	phys_addr_t ret;
	enum memblock_flags flags = choose_memblock_flags();

	ret = __memblock_find_in_range(mb, size, align, start, end, flags);

	return ret;
}

static void __init_memblock memblock_remove_region(struct memblock_type *type, u64 r)
{
	type->total_size -= type->regions[r].size;
	memmove(&type->regions[r], &type->regions[r + 1],
		(type->cnt - (r + 1)) * sizeof(type->regions[r]));
	type->cnt--;

	/* Special case for empty arrays */
	if (type->cnt == 0) {
		WARN_ON(type->total_size != 0);
		type->cnt = 1;
		type->regions[0].base = 0;
		type->regions[0].size = 0;
		type->regions[0].flags = 0;
	}
}

/**
 * memblock_merge_regions - merge neighboring compatible regions
 * @type: memblock type to scan
 *
 * Scan @type and merge neighboring compatible regions.
 */
static void __init_memblock memblock_merge_regions(struct memblock_type *type)
{
	int i = 0;

	/* cnt never goes below 1 */
	while (i < type->cnt - 1) {
		struct memblock_region *this = &type->regions[i];
		struct memblock_region *next = &type->regions[i + 1];

		if (this->base + this->size != next->base ||
		    this->flags != next->flags) {
			BUG_ON(this->base + this->size > next->base);
			i++;
			continue;
		}

		this->size += next->size;
		/* move forward from next + 1, index of which is i + 2 */
		memmove(next, next + 1, (type->cnt - (i + 2)) * sizeof(*next));
		type->cnt--;
	}
}

/**
 * memblock_insert_region - insert new memblock region
 * @type:	memblock type to insert into
 * @idx:	index for the insertion point
 * @base:	base address of the new region
 * @size:	size of the new region
 * @flags:	flags of the new region
 *
 * Insert new memblock region [@base, @base + @size) into @type at @idx.
 * @type must already have extra room to accommodate the new region.
 */
static void __init_memblock memblock_insert_region(struct memblock_type *type,
						   int idx, phys_addr_t base,
						   phys_addr_t size,
						   enum memblock_flags flags)
{
	struct memblock_region *rgn = &type->regions[idx];

	BUG_ON(type->cnt >= type->max);
	memmove(rgn + 1, rgn, (type->cnt - idx) * sizeof(*rgn));
	rgn->base = base;
	rgn->size = size;
	rgn->flags = flags;
	type->cnt++;
	type->total_size += size;
}

/**
 * memblock_add_range - add new memblock region
 * @type: memblock type to add new region into
 * @base: base address of the new region
 * @size: size of the new region
 * @flags: flags of the new region
 *
 * Add new memblock region [@base, @base + @size) into @type.  The new region
 * is allowed to overlap with existing ones - overlaps don't affect already
 * existing regions.  @type is guaranteed to be minimal (all neighbouring
 * compatible regions are merged) after the addition.
 *
 * Return:
 * 0 on success, -errno on failure.
 */
int __init_memblock memblock_add_range(struct memblock_type *type,
				phys_addr_t base, phys_addr_t size,
				enum memblock_flags flags)
{
	bool insert = false;
	phys_addr_t obase = base;
	phys_addr_t end = base + memblock_cap_size(base, &size);
	int idx, nr_new;
	struct memblock_region *rgn;

	if (!size)
		return 0;

	/* special case for empty array */
	if (type->regions[0].size == 0) {
		WARN_ON(type->cnt != 1 || type->total_size);
		type->regions[0].base = base;
		type->regions[0].size = size;
		type->regions[0].flags = flags;
		type->total_size = size;
		return 0;
	}
repeat:
	/*
	 * The following is executed twice.  Once with %false @insert and
	 * then with %true.  The first counts the number of regions needed
	 * to accommodate the new area.  The second actually inserts them.
	 */
	base = obase;
	nr_new = 0;

	for_each_memblock_type(idx, type, rgn) {
		phys_addr_t rbase = rgn->base;
		phys_addr_t rend = rbase + rgn->size;

		if (rbase >= end)
			break;
		if (rend <= base)
			continue;
		/*
		 * @rgn overlaps.  If it separates the lower part of new
		 * area, insert that portion.
		 */
		if (rbase > base) {
			WARN_ON(flags != rgn->flags);
			nr_new++;
			if (insert)
				memblock_insert_region(type, idx++, base,
						       rbase - base,
						       flags);
		}
		/* area below @rend is dealt with, forget about it */
		base = min(rend, end);
	}

	/* insert the remaining portion */
	if (base < end) {
		nr_new++;
		if (insert)
			memblock_insert_region(type, idx, base, end - base,
					    	flags);
	}

	if (!nr_new)
		return 0;

	/*
	 * If this was the first round, resize array and repeat for actual
	 * insertions; otherwise, merge and return.
	 */
	if (!insert) {
		while (type->cnt + nr_new > type->max) {
			printf("Memblock is full !\n");
			return -ENOMEM;
		}
		insert = true;
		goto repeat;
	} else {
		memblock_merge_regions(type);
		return 0;
	}
}

/**
 * memblock_add - add new memblock region
 * @base: base address of the new region
 * @size: size of the new region
 *
 * Add new memblock region [@base, @base + @size) to the "memory"
 * type. See memblock_add_range() description for mode details
 *
 * Return:
 * 0 on success, -errno on failure.
 */
int __init_memblock memblock_add(struct memblock *mb,
					phys_addr_t base, phys_addr_t size)
{
	phys_addr_t end = base + size - 1;

	memblock_dbg("memblock_add: [0x%016llx-0x%016llx] 0x%lx\n",
		     base, end, _RET_IP_);

	return memblock_add_range(&mb->memory, base, size, 0);
}

/**
 * memblock_isolate_range - isolate given range into disjoint memblocks
 * @type: memblock type to isolate range for
 * @base: base of range to isolate
 * @size: size of range to isolate
 * @start_rgn: out parameter for the start of isolated region
 * @end_rgn: out parameter for the end of isolated region
 *
 * Walk @type and ensure that regions don't cross the boundaries defined by
 * [@base, @base + @size).  Crossing regions are split at the boundaries,
 * which may create at most two more regions.  The index of the first
 * region inside the range is returned in *@start_rgn and end in *@end_rgn.
 *
 * Return:
 * 0 on success, -errno on failure.
 */
static int __init_memblock memblock_isolate_range(struct memblock_type *type,
					phys_addr_t base, phys_addr_t size,
					int *start_rgn, int *end_rgn)
{
	phys_addr_t end = base + memblock_cap_size(base, &size);
	int idx;
	struct memblock_region *rgn;

	*start_rgn = *end_rgn = 0;

	if (!size)
		return 0;

	/* we'll create at most two more regions */
	if (type->cnt + 2 > type->max) {
		printf("Memblock is full !\n");
		return -ENOMEM;
	}

	for_each_memblock_type(idx, type, rgn) {
		phys_addr_t rbase = rgn->base;
		phys_addr_t rend = rbase + rgn->size;

		if (rbase >= end)
			break;
		if (rend <= base)
			continue;

		if (rbase < base) {
			/*
			 * @rgn intersects from below.  Split and continue
			 * to process the next region - the new top half.
			 */
			rgn->base = base;
			rgn->size -= base - rbase;
			type->total_size -= base - rbase;
			memblock_insert_region(type, idx, rbase, base - rbase,
					       rgn->flags);
		} else if (rend > end) {
			/*
			 * @rgn intersects from above.  Split and redo the
			 * current region - the new bottom half.
			 */
			rgn->base = end;
			rgn->size -= end - rbase;
			type->total_size -= end - rbase;
			memblock_insert_region(type, idx--, rbase, end - rbase,
					       rgn->flags);
		} else {
			/* @rgn is fully contained, record it */
			if (!*end_rgn)
				*start_rgn = idx;
			*end_rgn = idx + 1;
		}
	}

	return 0;
}

static int __init_memblock memblock_remove_range(struct memblock_type *type,
					  phys_addr_t base, phys_addr_t size)
{
	int start_rgn, end_rgn;
	int i, ret;

	ret = memblock_isolate_range(type, base, size, &start_rgn, &end_rgn);
	if (ret)
		return ret;

	for (i = end_rgn - 1; i >= start_rgn; i--)
		memblock_remove_region(type, i);
	return 0;
}

int __init_memblock memblock_remove(struct memblock *mb,
						phys_addr_t base, phys_addr_t size)
{
	phys_addr_t end = base + size - 1;

	memblock_dbg("memblock_remove: [0x%016llx-0x%016llx] 0x%lx\n",
		     base, end, _RET_IP_);

	return memblock_remove_range(&mb->memory, base, size);
}

/**
 * memblock_free - free boot memory block
 * @base: phys starting address of the  boot memory block
 * @size: size of the boot memory block in bytes
 *
 * Free boot memory block previously allocated by memblock_alloc_xx() API.
 * The freeing memory will not be released to the buddy allocator.
 */
int __init_memblock memblock_free(struct memblock *mb,
					phys_addr_t base, phys_addr_t size)
{
	phys_addr_t end = base + size - 1;

	memblock_dbg("   memblock_free: [0x%016llx-0x%016llx] 0x%lx\n",
		     base, end, _RET_IP_);

	return memblock_remove_range(&mb->reserved, base, size);
}

int __init_memblock memblock_reserve(struct memblock *mb,
					phys_addr_t base, phys_addr_t size)
{
	phys_addr_t end = base + size - 1;

	memblock_dbg("memblock_reserve: [0x%016llx-0x%016llx] 0x%lx\n",
		     base, end, _RET_IP_);

	return memblock_add_range(&mb->reserved, base, size, 0);
}

/**
 * memblock_setclr_flag - set or clear flag for a memory region
 * @base: base address of the region
 * @size: size of the region
 * @set: set or clear the flag
 * @flag: the flag to udpate
 *
 * This function isolates region [@base, @base + @size), and sets/clears flag
 *
 * Return: 0 on success, -errno on failure.
 */
static int __init_memblock memblock_setclr_flag(struct memblock *mb,
				phys_addr_t base,
				phys_addr_t size, int set, int flag)
{
	struct memblock_type *type = &mb->memory;
	int i, ret, start_rgn, end_rgn;

	ret = memblock_isolate_range(type, base, size, &start_rgn, &end_rgn);
	if (ret)
		return ret;

	for (i = start_rgn; i < end_rgn; i++)
		if (set)
			memblock_set_region_flags(&type->regions[i], flag);
		else
			memblock_clear_region_flags(&type->regions[i], flag);

	memblock_merge_regions(type);
	return 0;
}

/**
 * memblock_mark_nomap - Mark a memory region with flag MEMBLOCK_NOMAP.
 * @base: the base phys addr of the region
 * @size: the size of the region
 *
 * Return: 0 on success, -errno on failure.
 */
int __init_memblock memblock_mark_nomap(struct memblock *mb,
				phys_addr_t base, phys_addr_t size)
{
	return memblock_setclr_flag(mb, base, size, 1, MEMBLOCK_NOMAP);
}

/**
 * memblock_clear_nomap - Clear flag MEMBLOCK_NOMAP for a specified region.
 * @base: the base phys addr of the region
 * @size: the size of the region
 *
 * Return: 0 on success, -errno on failure.
 */
int __init_memblock memblock_clear_nomap(struct memblock *mb,
				phys_addr_t base, phys_addr_t size)
{
	return memblock_setclr_flag(mb, base, size, 0, MEMBLOCK_NOMAP);
}

int __init_memblock memblock_mark_dma(struct memblock *mb,
				phys_addr_t base, phys_addr_t size)
{
	return memblock_setclr_flag(mb, base, size, 1, MEMBLOCK_DMA);
}

int __init_memblock memblock_clear_dma(struct memblock *mb,
					phys_addr_t base, phys_addr_t size)
{
	return memblock_setclr_flag(mb, base, size, 0, MEMBLOCK_DMA);
}

int __init_memblock memblock_mark_movable(struct memblock *mb,
					phys_addr_t base, phys_addr_t size)
{
	return memblock_setclr_flag(mb, base, size, 1, MEMBLOCK_MOVABLE);
}

int __init_memblock memblock_clear_movable(struct memblock *mb,
					phys_addr_t base, phys_addr_t size)
{
	return memblock_setclr_flag(mb, base, size, 0, MEMBLOCK_MOVABLE);
}

/**
 * __next_reserved_mem_region - next function for for_each_reserved_region()
 * @idx: pointer to u64 loop variable
 * @out_start: ptr to phys_addr_t for start address of the region, can be %NULL
 * @out_end: ptr to phys_addr_t for end address of the region, can be %NULL
 *
 * Iterate over all reserved memory regions.
 */
void __init_memblock __next_reserved_mem_region(struct memblock *mb,
					   u64 *idx,
					   phys_addr_t *out_start,
					   phys_addr_t *out_end)
{
	struct memblock_type *type = &mb->reserved;

	if (*idx < type->cnt) {
		struct memblock_region *r = &type->regions[*idx];
		phys_addr_t base = r->base;
		phys_addr_t size = r->size;

		if (out_start)
			*out_start = base;
		if (out_end)
			*out_end = base + size - 1;

		*idx += 1;
		return;
	}

	/* signal end of iteration */
	*idx = ULLONG_MAX;
}

/**
 * __next__mem_range - next function for for_each_free_mem_range() etc.
 * @idx: pointer to u64 loop variable
 * @flags: pick from blocks based on memory attributes
 * @type_a: pointer to memblock_type from where the range is taken
 * @type_b: pointer to memblock_type which excludes memory from being taken
 * @out_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @out_end: ptr to phys_addr_t for end address of the range, can be %NULL
 *
 *
 *	0:[0-16), 1:[32-48), 2:[128-130)
 *
 * The upper 32bit indexes the following regions.
 *
 *	0:[0-0), 1:[16-32), 2:[48-128), 3:[130-MAX)
 *
 * As both region arrays are sorted, the function advances the two indices
 * in lockstep and returns each intersection.
 */
void __init_memblock __next_mem_range(u64 *idx,
				      enum memblock_flags flags,
				      struct memblock_type *type_a,
				      struct memblock_type *type_b,
				      phys_addr_t *out_start,
				      phys_addr_t *out_end)
{
	int idx_a = *idx & 0xffffffff;
	int idx_b = *idx >> 32;

	for (; idx_a < type_a->cnt; idx_a++) {
		struct memblock_region *m = &type_a->regions[idx_a];

		phys_addr_t m_start = m->base;
		phys_addr_t m_end = m->base + m->size;

		if (flags != m->flags)
			continue;

		if (!type_b) {
			if (out_start)
				*out_start = m_start;
			if (out_end)
				*out_end = m_end;
			idx_a++;
			*idx = (u32)idx_a | (u64)idx_b << 32;
			return;
		}

		/* scan areas before each reservation */
		for (; idx_b < type_b->cnt + 1; idx_b++) {
			struct memblock_region *r;
			phys_addr_t r_start;
			phys_addr_t r_end;

			r = &type_b->regions[idx_b];
			r_start = idx_b ? r[-1].base + r[-1].size : 0;
			r_end = idx_b < type_b->cnt ?
				r->base : PHYS_ADDR_MAX;

			/*
			 * if idx_b advanced past idx_a,
			 * break out to advance idx_a
			 */
			if (r_start >= m_end)
				break;
			/* if the two regions intersect, we're done */
			if (m_start < r_end) {
				if (out_start)
					*out_start =
						max(m_start, r_start);
				if (out_end)
					*out_end = min(m_end, r_end);
				/*
				 * The region which ends first is
				 * advanced for the next iteration.
				 */
				if (m_end <= r_end)
					idx_a++;
				else
					idx_b++;
				*idx = (u32)idx_a | (u64)idx_b << 32;
				return;
			}
		}
	}

	/* signal end of iteration */
	*idx = ULLONG_MAX;
}

/**
 * __next_mem_range_rev - generic next function for for_each_*_range_rev()
 *
 * @idx: pointer to u64 loop variable
 * @flags: pick from blocks based on memory attributes
 * @type_a: pointer to memblock_type from where the range is taken
 * @type_b: pointer to memblock_type which excludes memory from being taken
 * @out_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @out_end: ptr to phys_addr_t for end address of the range, can be %NULL
 *
 * Finds the next range from type_a which is not marked as unsuitable
 * in type_b.
 *
 * Reverse of __next_mem_range().
 */
void __init_memblock __next_mem_range_rev(u64 *idx,
					  enum memblock_flags flags,
					  struct memblock_type *type_a,
					  struct memblock_type *type_b,
					  phys_addr_t *out_start,
					  phys_addr_t *out_end)
{
	int idx_a = *idx & 0xffffffff;
	int idx_b = *idx >> 32;

	if (*idx == (u64)ULLONG_MAX) {
		idx_a = type_a->cnt - 1;
		if (type_b != NULL)
			idx_b = type_b->cnt;
		else
			idx_b = 0;
	}

	for (; idx_a >= 0; idx_a--) {
		struct memblock_region *m = &type_a->regions[idx_a];

		phys_addr_t m_start = m->base;
		phys_addr_t m_end = m->base + m->size;

		if (flags != m->flags)
			continue;

		if (!type_b) {
			if (out_start)
				*out_start = m_start;
			if (out_end)
				*out_end = m_end;
			idx_a--;
			*idx = (u32)idx_a | (u64)idx_b << 32;
			return;
		}

		/* scan areas before each reservation */
		for (; idx_b >= 0; idx_b--) {
			struct memblock_region *r;
			phys_addr_t r_start;
			phys_addr_t r_end;

			r = &type_b->regions[idx_b];
			r_start = idx_b ? r[-1].base + r[-1].size : 0;
			r_end = idx_b < type_b->cnt ?
				r->base : PHYS_ADDR_MAX;
			/*
			 * if idx_b advanced past idx_a,
			 * break out to advance idx_a
			 */

			if (r_end <= m_start)
				break;
			/* if the two regions intersect, we're done */
			if (m_end > r_start) {
				if (out_start)
					*out_start = max(m_start, r_start);
				if (out_end)
					*out_end = min(m_end, r_end);
				if (m_start >= r_start)
					idx_a--;
				else
					idx_b--;
				*idx = (u32)idx_a | (u64)idx_b << 32;
				return;
			}
		}
	}
	/* signal end of iteration */
	*idx = ULLONG_MAX;
}

/**
 * __next_reserved_mem_region - next function for for_each_reserved_region()
 * @idx: pointer to u64 loop variable
 * @out_start: ptr to phys_addr_t for start address of the region, can be %NULL
 * @out_end: ptr to phys_addr_t for end address of the region, can be %NULL
 *
 * Iterate over all reserved memory regions.
 */
void __init_memblock __next_mem_pfn_range(struct memblock *mb,
					   int *idx,
					   phys_addr_t *out_start_pfn,
					   phys_addr_t *out_end_pfn)
{
	struct memblock_type *type = &mb->memory;
	struct memblock_region *r;

	while (++*idx < type->cnt) {
		r = &type->regions[*idx];

		if (PFN_UP(r->base) >= PFN_DOWN(r->base + r->size))
			continue;
		break;
	}
	if (*idx >= type->cnt) {
		*idx = -1;
		return;
	}

	if (out_start_pfn)
		*out_start_pfn = PFN_UP(r->base);
	if (out_end_pfn)
		*out_end_pfn = PFN_DOWN(r->base + r->size);
}

static phys_addr_t __init __memblock_alloc_range(struct memblock *mb,
					phys_addr_t size,
					phys_addr_t align, phys_addr_t start,
					phys_addr_t end,
					enum memblock_flags flags)
{
	phys_addr_t found;

	if (!align) {
		WARN(1, "Memblock align is Zero!\n");
		return 0;
	}

	found = __memblock_find_in_range(mb, size, align, start, end,
					    flags);
	if (found && !memblock_reserve(mb, found, size)) {
		return found;
	}

	return 0;
}

phys_addr_t __init memblock_alloc_range(struct memblock *mb,
					phys_addr_t size, phys_addr_t align,
					phys_addr_t start, phys_addr_t end,
					enum memblock_flags flags)
{
	return __memblock_alloc_range(mb, size, align, start, end,
					flags);
}

phys_addr_t __init memblock_alloc_base(struct memblock *mb,
					phys_addr_t size,
					phys_addr_t align, phys_addr_t max_addr,
					enum memblock_flags flags)
{
	return __memblock_alloc_range(mb, size, align, 0, max_addr, flags);
}

phys_addr_t __init memblock_phys_alloc(struct memblock *mb,
						phys_addr_t size, phys_addr_t align)
{
	enum memblock_flags flags = choose_memblock_flags();
	phys_addr_t ret;

	ret = memblock_alloc_base(mb, size, align, MEMBLOCK_ALLOC_ACCESSIBLE,
				    	flags);

	return ret;
}

/**
 * memblock_alloc_internal - allocate boot memory block
 * @size: size of memory block to be allocated in bytes
 * @align: alignment of the region and block's size
 * @min_addr: the lower bound of the memory region to allocate (phys address)
 * @max_addr: the upper bound of the memory region to allocate (phys address)
 *
 *
 * The allocation is performed from memory region limited by
 * memblock.current_limit if @max_addr == %MEMBLOCK_ALLOC_ACCESSIBLE.
 *
 * The phys address of allocated boot memory block is converted to virtual and
 * allocated memory is reset to 0.
 *
 * In addition, function sets the min_count to 0 using kmemleak_alloc for
 * allocated boot memory block, so that it is never reported as leaks.
 *
 * Return:
 * Virtual address of allocated memory block on success, NULL on failure.
 */
static phys_addr_t __init memblock_alloc_internal(struct memblock *mb,
				phys_addr_t size, phys_addr_t align,
				phys_addr_t min_addr, phys_addr_t max_addr)
{
	phys_addr_t alloc;
	enum memblock_flags flags = choose_memblock_flags();

	if (!align) {
		WARN(1, "Memblock align is Zero!\n");
		return 0;
	}

	if (max_addr > mb->current_limit)
		max_addr = mb->current_limit;
again:
	alloc = __memblock_find_in_range(mb, size, align, min_addr, max_addr,
					    	flags);
	if (alloc && !memblock_reserve(mb, alloc, size))
		goto done;

	if (min_addr) {
		min_addr = 0;
		goto again;
	}

	return 0;
done:
	return alloc;
}

/**
 * memblock_alloc_try_raw - allocate boot memory block without zeroing
 * memory and without panicking
 * @size: size of memory block to be allocated in bytes
 * @align: alignment of the region and block's size
 * @min_addr: the lower bound of the memory region from where the allocation
 *	  is preferred (phys address)
 * @max_addr: the upper bound of the memory region from where the allocation
 *	      is preferred (phys address), or %MEMBLOCK_ALLOC_ACCESSIBLE to
 *	      allocate only from memory limited by memblock.current_limit value
 *
 * Public function, provides additional debug information (including caller
 * info), if enabled. Does not zero allocated memory, does not panic if request
 * cannot be satisfied.
 *
 * Return:
 * Virtual address of allocated memory block on success, NULL on failure.
 */
phys_addr_t __init memblock_alloc_try_raw(struct memblock *mb,
			phys_addr_t size, phys_addr_t align,
			phys_addr_t min_addr, phys_addr_t max_addr)
{
	phys_addr_t addr;

	memblock_dbg("%s: %llu bytes align=0x%llx from=0x%016llx max_addr=0x%016llx 0x%lx\n",
		     __func__, (u64)size, (u64)align, min_addr,
		     max_addr, _RET_IP_);

	addr = memblock_alloc_internal(mb, size, align,
					   min_addr, max_addr);

	return addr;
}

/*
 * Remaining API functions
 */
phys_addr_t __init_memblock memblock_phys_mem_size(struct memblock *mb)
{
	return mb->memory.total_size;
}

phys_addr_t __init_memblock memblock_reserved_size(struct memblock *mb)
{
	return mb->reserved.total_size;
}

phys_addr_t __init memblock_mem_size(struct memblock *mb, u64 limit_pfn)
{
	u64 pages = 0;
	struct memblock_region *r;
	u64 start_pfn, end_pfn;

	for_each_memblock(mb, memory, r) {
		start_pfn = memblock_region_memory_base_pfn(r);
		end_pfn = memblock_region_memory_end_pfn(r);
		start_pfn = min_t(u64, start_pfn, limit_pfn);
		end_pfn = min_t(u64, end_pfn, limit_pfn);
		pages += end_pfn - start_pfn;
	}

	return PFN_PHYS(pages);
}

/* lowest address */
phys_addr_t __init_memblock memblock_start_of_DRAM(struct memblock *mb)
{
	return mb->memory.regions[0].base;
}

phys_addr_t __init_memblock memblock_end_of_DRAM(struct memblock *mb)
{
	int idx = mb->memory.cnt - 1;

	return (mb->memory.regions[idx].base + mb->memory.regions[idx].size);
}

static phys_addr_t __init_memblock __find_max_addr(struct memblock *mb,
						phys_addr_t limit)
{
	phys_addr_t max_addr = PHYS_ADDR_MAX;
	struct memblock_region *r;

	/*
	 * translate the memory @limit size into the max address within one of
	 * the memory memblock regions, if the @limit exceeds the total size
	 * of those regions, max_addr will keep original value PHYS_ADDR_MAX
	 */
	for_each_memblock(mb, memory, r) {
		if (limit <= r->size) {
			max_addr = r->base + limit;
			break;
		}
		limit -= r->size;
	}

	return max_addr;
}

void __init memblock_enforce_memory_limit(struct memblock *mb, phys_addr_t limit)
{
	phys_addr_t max_addr = PHYS_ADDR_MAX;

	if (!limit)
		return;

	max_addr = __find_max_addr(mb, limit);

	/* @limit exceeds the total size of the memory, do nothing */
	if (max_addr == PHYS_ADDR_MAX)
		return;

	/* truncate both memory and reserved regions */
	memblock_remove_range(&mb->memory, max_addr,
			      PHYS_ADDR_MAX);
	memblock_remove_range(&mb->reserved, max_addr,
			      PHYS_ADDR_MAX);
}

void __init memblock_cap_memory_range(struct memblock *mb,
						phys_addr_t base, phys_addr_t size)
{
	int start_rgn, end_rgn;
	int i, ret;

	if (!size)
		return;

	ret = memblock_isolate_range(&mb->memory, base, size,
						&start_rgn, &end_rgn);
	if (ret)
		return;

	/* remove all the MAP regions */
	for (i = mb->memory.cnt - 1; i >= end_rgn; i--)
		if (!memblock_is_nomap(&mb->memory.regions[i]))
			memblock_remove_region(&mb->memory, i);

	for (i = start_rgn - 1; i >= 0; i--)
		if (!memblock_is_nomap(&mb->memory.regions[i]))
			memblock_remove_region(&mb->memory, i);

	/* truncate the reserved regions */
	memblock_remove_range(&mb->reserved, 0, base);
	memblock_remove_range(&mb->reserved,
			base + size, PHYS_ADDR_MAX);
}

void __init memblock_mem_limit_remove_map(struct memblock *mb, phys_addr_t limit)
{
	phys_addr_t max_addr;

	if (!limit)
		return;

	max_addr = __find_max_addr(mb, limit);

	/* @limit exceeds the total size of the memory, do nothing */
	if (max_addr == PHYS_ADDR_MAX)
		return;

	memblock_cap_memory_range(mb, 0, max_addr);
}

static int memblock_search(struct memblock_type *type, phys_addr_t addr)
{
	unsigned int left = 0, right = type->cnt;

	do {
		unsigned int mid = (right + left) / 2;

		if (addr < type->regions[mid].base)
			right = mid;
		else if (addr >= (type->regions[mid].base +
				  type->regions[mid].size))
			left = mid + 1;
		else
			return mid;
	} while (left < right);
	return -1;
}

bool memblock_is_reserved(struct memblock *mb, phys_addr_t addr)
{
	return memblock_search(&mb->reserved, addr) != -1;
}

bool memblock_is_memory(struct memblock *mb, phys_addr_t addr)
{
	return memblock_search(&mb->memory, addr) != -1;
}

bool memblock_is_map_memory(struct memblock *mb, phys_addr_t addr)
{
	int i = memblock_search(&mb->memory, addr);

	if (i == -1)
		return false;
	return !memblock_is_nomap(&mb->memory.regions[i]);
}

/**
 * memblock_is_region_memory - check if a region is a subset of memory
 * @base: base of region to check
 * @size: size of region to check
 *
 * Check if the region [@base, @base + @size) is a subset of a memory block.
 *
 * Return:
 * 0 if false, non-zero if true
 */
bool __init_memblock memblock_is_region_memory(struct memblock *mb, phys_addr_t base, phys_addr_t size)
{
	int idx = memblock_search(&mb->memory, base);
	phys_addr_t end = base + memblock_cap_size(base, &size);

	if (idx == -1)
		return false;
	return (mb->memory.regions[idx].base +
		 mb->memory.regions[idx].size) >= end;
}

/**
 * memblock_is_region_reserved - check if a region intersects reserved memory
 * @base: base of region to check
 * @size: size of region to check
 *
 * Check if the region [@base, @base + @size) intersects a reserved
 * memory block.
 *
 * Return:
 * True if they intersect, false if not.
 */
bool __init_memblock memblock_is_region_reserved(struct memblock *mb, phys_addr_t base, phys_addr_t size)
{
	memblock_cap_size(base, &size);
	return memblock_overlaps_region(&mb->reserved, base, size);
}

void __init_memblock memblock_trim_memory(struct memblock *mb, phys_addr_t align)
{
	phys_addr_t start, end, orig_start, orig_end;
	struct memblock_region *r;

	for_each_memblock(mb, memory, r) {
		orig_start = r->base;
		orig_end = r->base + r->size;
		start = round_up(orig_start, align);
		end = round_down(orig_end, align);

		if (start == orig_start && end == orig_end)
			continue;

		if (start < end) {
			r->base = start;
			r->size = end - start;
		} else {
			memblock_remove_region(&mb->memory,
					       r - mb->memory.regions);
			r--;
		}
	}
}

void __init_memblock memblock_set_current_limit(struct memblock *mb, phys_addr_t limit)
{
	mb->current_limit = limit;
}

phys_addr_t __init_memblock memblock_get_current_limit(struct memblock *mb)
{
	return mb->current_limit;
}

static void __init_memblock memblock_dump(struct memblock_type *type)
{
	phys_addr_t base, end, size;
	enum memblock_flags flags;
	int idx;
	struct memblock_region *rgn;

	printf(" %s.cnt  = 0x%llx\n", type->name, type->cnt);

	for_each_memblock_type(idx, type, rgn) {
		base = rgn->base;
		size = rgn->size;
		end = base + size - 1;
		flags = rgn->flags;
		printf(" %s[%#x]\t[0x%016llx-0x%016llx], 0x%llx bytes flags: %#x\n",
			type->name, idx, base, end, size, flags);
	}
}

void __init_memblock __memblock_dump_all(struct memblock *mb)
{
	printf("MEMBLOCK configuration:\n");
	printf(" memory size = 0x%llx reserved size = 0x%llx\n",
		mb->memory.total_size,
		mb->reserved.total_size);

	memblock_dump(&mb->memory);
	memblock_dump(&mb->reserved);
}
