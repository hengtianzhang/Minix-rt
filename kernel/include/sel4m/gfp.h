/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_GFP_H_
#define __SEL4M_GFP_H_

#include <base/types.h>

#include <sel4m/mm.h>

#define ___GFP_DMA			BIT(0)
#define ___GFP_MOVABLE		BIT(1)
#define ___GFP_NORMAL		BIT(2)

#define ___GFP_ZERO			BIT(3)
#define ___GFP_NOWARN		BIT(4)

#define ___GFP_KERNEL		BIT(5)
#define ___GFP_USER			BIT(6)

#define ___GFP_BITS_SHIFT	7

#define __GFP_DMA			((gfp_t)___GFP_DMA)
#define __GFP_MOVABLE		((gfp_t)___GFP_MOVABLE)
#define __GFP_NORMAL		((gfp_t)___GFP_NORMAL)

#define __GFP_ZERO			((gfp_t)___GFP_ZERO)
#define __GFP_NOWARN		((gfp_t)___GFP_NOWARN)

#define __GFP_KERNEL		((gfp_t)___GFP_KERNEL)
#define __GFP_USER			((gfp_t)___GFP_USER)

#define GFP_DMA				(__GFP_DMA)
#define GFP_MOVABLE			(__GFP_MOVABLE)
#define GFP_NORMAL			(__GFP_NORMAL)

#define GFP_ZERO			(__GFP_ZERO)
#define GFP_NOWARN			(__GFP_NOWARN)

#define GFP_KERNEL			(__GFP_KERNEL)
#define GFP_USER			(__GFP_USER)

static inline enum zone_type gfp_zone(gfp_t flags)
{
	if (unlikely(__GFP_DMA & flags))
		return ZONE_DMA;
	if (unlikely(__GFP_MOVABLE & flags))
		return ZONE_MOVABLE;
	
	return ZONE_NORMAL;
}

#define alloc_pages(gfp_mask, order)	__alloc_pages(gfp_mask, order)
#define alloc_page(gfp_mask)			__alloc_pages(gfp_mask, 0)

#define get_free_pages(gfp_mask, order)	__get_free_pages(gfp_mask, order)
#define get_free_page(gfp_mask)	__get_free_pages(gfp_mask, 0)

#define __free_page(page) __free_pages((page), 0)
#define free_page(addr) free_pages((addr), 0)

#endif /* !__SEL4M_GFP_H_ */
