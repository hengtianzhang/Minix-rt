/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BASE_MMZONE_H_
#define __BASE_MMZONE_H_

#ifndef __ASSEMBLY__

#include <base/atomic.h>
#include <base/cache.h>
#include <base/list.h>

/* Free memory management - zoned buddy allocator.  */
#ifdef CONFIG_FORCE_MAX_ZONEORDER
#define MAX_ORDER CONFIG_FORCE_MAX_ZONEORDER
#else
#define MAX_ORDER 11
#endif
#define MAX_ORDER_NR_PAGES (1 << (MAX_ORDER - 1))

#define pageblock_order (MAX_ORDER - 1)
#define pageblock_nr_pages (1ULL << pageblock_order)

struct free_area {
	struct list_head    free_list;
	unsigned long       nr_free;
};

struct zone {
	struct pglist_data	*zone_pgdat;

	/* zone_start_pfn == zone_start_paddr >> PAGE_SHIFT */
	unsigned long		zone_start_pfn;
	unsigned long		present_pages;
	unsigned long		spanned_pages;

	atomic_long_t		managed_pages;

	const char			*name;

	int		initialized;

	/* free areas of different sizes */
	struct free_area	free_area[MAX_ORDER];

	unsigned long		flags;
} ____cacheline_internodealigned_in_smp;

enum zone_type {
	ZONE_DMA,
	ZONE_NORMAL,
	ZONE_MOVABLE,
	__MAX_NR_ZONES
};

#define MAX_NR_ZONES    __MAX_NR_ZONES

typedef struct pglist_data {
	struct zone 	node_zones[MAX_NR_ZONES];

	unsigned long 	node_start_pfn;
	unsigned long 	node_present_pages; /* total number of physical pages */
	unsigned long 	node_spanned_pages; /* total size of physical page
						 range, including holes */

	unsigned long   flags;
} pg_data_t;

#endif /* !__ASSEMBLY__ */
#endif /* !__BASE_MMZONE_H_ */
