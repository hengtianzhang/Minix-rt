/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_MMZONE_H_
#define __SEL4M_MMZONE_H_

#include <base/list.h>
#include <base/cache.h>

#include <sel4m/spinlock.h>

/* Free memory management - zoned buddy allocator.  */
#ifndef CONFIG_FORCE_MAX_ZONEORDER
#define MAX_ORDER 11
#else
#define MAX_ORDER CONFIG_FORCE_MAX_ZONEORDER
#endif
#define MAX_ORDER_NR_PAGES (1 << (MAX_ORDER - 1))

struct free_area {
	struct list_head	free_list;
	unsigned long		nr_free;
};

struct per_cpu_pages {
	int count;		/* number of pages in the list */
	int high;		/* high watermark, emptying needed */
	int batch;		/* chunk size for buddy add/remove */

	struct list_head lists;
};


struct per_cpu_pageset {
	struct per_cpu_pages pcp;
};

enum zone_type {
	ZONE_DMA,
	ZONE_NORMAL,
	ZONE_MOVABLE,
	__MAX_NR_ZONES,
};

#define MAX_NR_ZONES	__MAX_NR_ZONES

struct pglist_data;

struct zone {
	struct pglist_data	*zone_pgdat;
	struct per_cpu_pageset pageset[CONFIG_NR_CPUS];

	/* zone_start_pfn == zone_start_paddr >> PAGE_SHIFT */
	unsigned long		zone_start_pfn;

	atomic_long_t		managed_pages;

	const char		*name;

	int initialized;

	/* free areas of different sizes */
	struct free_area	free_area[MAX_ORDER];

	/* Primarily protects free_area */
	spinlock_t		lock;
} ____cacheline_internodealigned_in_smp;

typedef struct pglist_data {
	struct zone node_zones[MAX_NR_ZONES];

	unsigned long		node_start_pfn;

	unsigned long		totalreserve_pages;
} pg_data_t;

#endif /* !__SEL4M_MMZONE_H_ */
