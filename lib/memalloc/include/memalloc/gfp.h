#ifndef __MEMALLOC_GFP_H_
#define __MEMALLOC_GFP_H_

#include <memalloc/mmzone.h>

#include <asm/base/types.h>

typedef unsigned int __bitwise gfp_t;

#define ___GFP_NORMAL	0x01u
#define ___GFP_DMA		0x02u
#define ___GFP_MOVABLE	0x04u
#define ___GFP_NOWARN	0x08u

#define ___GFP_BITS_SHIFT	4

#define __GFP_NORMAL	((gfp_t)___GFP_NORMAL)
#define __GFP_DMA		((gfp_t)___GFP_DMA)
#define __GFP_MOVABLE	((gfp_t)___GFP_MOVABLE)
#define __GFP_NOWARN	((gfp_t)___GFP_NOWARN)

#define __GFP_BITS_SHIFT ___GFP_BITS_SHIFT
#define __GFP_BITS_MASK ((gfp_t)((1 << __GFP_BITS_SHIFT) - 1))

#define GFP_BOOT_MASK	((gfp_t)(~__GFP_BITS_MASK))

static inline enum zone_type gfp_zone(gfp_t flags)
{
	if (unlikely(__GFP_DMA & flags))
		return ZONE_DMA;
	if (unlikely(__GFP_MOVABLE & flags))
		return ZONE_MOVABLE;

	return ZONE_NORMAL;
}

struct page *
__alloc_pages(gfp_t gfp_mask, unsigned int order);
#endif /* !__MEMALLOC_GFP_H_ */
