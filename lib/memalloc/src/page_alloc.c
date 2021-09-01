/*
 *  linux/mm/page_alloc.c
 *
 *  Manages the free list, the system allocates free pages here.
 *  Note that kmalloc() lives in slab.c
 *
 *  Copyright (C) 1991, 1992, 1993, 1994  Linus Torvalds
 *  Swap reorganised 29.12.95, Stephen Tweedie
 *  Support of BIGMEM added by Gerhard Wichert, Siemens AG, July 1999
 *  Reshaped it to be a zoned allocator, Ingo Molnar, Red Hat, 1999
 *  Discontiguous memory support, Kanoj Sarcar, SGI, Nov 1999
 *  Zone balancing, Kanoj Sarcar, SGI, Jan 2000
 *  Per cpu hot/cold page lists, bulk allocation, Martin J. Bligh, Sept 2002
 *          (lots of bits borrowed from Ingo Molnar & Andrew Morton)
 */
#include <base/common.h>
#include <base/cache.h>

#include <memalloc/gfp.h>
#include <memalloc/memblock.h>
#include <memalloc/mm_types.h>

struct alloc_context {
	enum zone_type zoneidx;
};

struct pglist_data node_data;
gfp_t gfp_allowed_mask __read_mostly = GFP_BOOT_MASK;

static inline bool prepare_alloc_pages(gfp_t gfp_mask, unsigned int order,
				struct alloc_context *ac)
{
	if (gfp_mask & GFP_BOOT_MASK)
		return false;

	ac->zoneidx = gfp_zone(gfp_mask);

	return true;
}

static inline
struct page *rmqueue(gfp_t gfp_mask, unsigned int order, struct zone *zone)
{
	return	NULL;
}

static struct page *
get_page_from_freelist(gfp_t gfp_mask, unsigned int order,
							const struct alloc_context *ac)
{
	struct page *page;
	struct zone *zone;

	zone = NODE_DATA()->node_zones + ac->zoneidx;

	page = rmqueue(gfp_mask, order, zone);

	return page;
}

struct page *
__alloc_pages(gfp_t gfp_mask, unsigned int order)
{
	struct page *page;
	struct alloc_context ac = {};

	if (unlikely(order >= MAX_ORDER)) {
		WARN_ON_ONCE(!(gfp_mask & __GFP_NOWARN));
		return NULL;
	}

	gfp_mask &= gfp_allowed_mask;


	if (!prepare_alloc_pages(gfp_mask, order, &ac))
		return NULL;

	/* First allocation attempt */
	page = get_page_from_freelist(gfp_mask, order, &ac);


	return page;
}

