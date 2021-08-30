#ifndef __MEMBLOCK_MEMBLOCK_H_
#define __MEMBLOCK_MEMBLOCK_H_

#include <base/init.h>
#include <base/types.h>

#define __init_memblock 	__init
#define __initdata_memblock __initdata

#define INIT_MEMBLOCK_REGIONS CONFIG_MEMBLOCK_REGIONS_NUM
#define MEMORY_REGIONS_LEN 16

enum memblock_flags {
	MEMBLOCK_NONE		= 0x0,
	MEMBLOCK_NOMAP		= 0x1,
	MEMBLOCK_DMA		= 0x2,
	MEMBLOCK_MOVABLE	= 0x3,
};

struct memblock_region {
	phys_addr_t	base;
	phys_addr_t size;
	enum	memblock_flags flags;
};

struct memblock_type {
	u64		cnt;
	u64		max;
	phys_addr_t	total_size;
	struct memblock_region regions[INIT_MEMBLOCK_REGIONS];
	char	name[MEMORY_REGIONS_LEN];
};

struct memblock {
	bool bottom_up;
	phys_addr_t	current_limit;
	struct memblock_type memory;
	struct memblock_type reserved;
};

/* Flags for memblock allocation APIs */
#define MEMBLOCK_ALLOC_ANYWHERE	(~(phys_addr_t)0)
#define MEMBLOCK_ALLOC_ACCESSIBLE	0

void __init_memblock memblock_init(struct memblock *mb);

#endif /* !__MEMBLOCK_MEMBLOCK_H_ */
