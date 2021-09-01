#ifndef __MEMALLOC_MEMBLOCK_H_
#define __MEMALLOC_MEMBLOCK_H_

#include <base/init.h>
#include <base/types.h>
#include <base/pfn.h>

#define __init_memblock 	__init
#define __initdata_memblock __initdata

extern int memblock_debug;

#define memblock_dbg(fmt, ...) \
	if (memblock_debug) printf(fmt, ##__VA_ARGS__)

static inline void memblock_debug_enable(void)
{
	memblock_debug = 1;
}

static inline void memblock_debug_diable(void)
{
	memblock_debug = 0;
}

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

static inline bool memblock_is_nomap(struct memblock_region *m)
{
	return m->flags & MEMBLOCK_NOMAP;
}

static inline bool memblock_is_dma(struct memblock_region *m)
{
	return m->flags & MEMBLOCK_DMA;
}

static inline bool memblock_is_movable(struct memblock_region *m)
{
	return m->flags & MEMBLOCK_MOVABLE;
}

static inline void memblock_set_region_flags(struct memblock_region *r,
					     enum memblock_flags flags)
{
	r->flags |= flags;
}

static inline void memblock_clear_region_flags(struct memblock_region *r,
					       enum memblock_flags flags)
{
	r->flags &= ~flags;
}

/*
 * Set the allocation direction to bottom-up or top-down.
 */
static inline void __init memblock_set_bottom_up(struct memblock *mb, bool enable)
{
	mb->bottom_up = enable;
}

/*
 * Check if the allocation direction is bottom-up or not.
 * if this is true, that said, memblock will allocate memory
 * in bottom-up direction.
 */
static inline bool memblock_bottom_up(struct memblock *mb)
{
	return mb->bottom_up;
}

/**
 * memblock_region_memory_base_pfn - get the lowest pfn of the memory region
 * @reg: memblock_region structure
 *
 * Return: the lowest pfn intersecting with the memory region
 */
static inline u64 memblock_region_memory_base_pfn(const struct memblock_region *reg)
{
	return PFN_UP(reg->base);
}

/**
 * memblock_region_memory_end_pfn - get the end pfn of the memory region
 * @reg: memblock_region structure
 *
 * Return: the end_pfn of the reserved region
 */
static inline u64 memblock_region_memory_end_pfn(const struct memblock_region *reg)
{
	return PFN_DOWN(reg->base + reg->size);
}

void __next_mem_range(u64 *idx, enum memblock_flags flags,
		      struct memblock_type *type_a,
		      struct memblock_type *type_b, phys_addr_t *out_start,
		      phys_addr_t *out_end);

/**
 * for_each_mem_range - iterate through memblock areas from type_a and not
 * included in type_b. Or just type_a if type_b is NULL.
 * @i: u64 used as loop variable
 * @type_a: ptr to memblock_type to iterate
 * @type_b: ptr to memblock_type which excludes from the iteration
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 */
#define for_each_mem_range(i, type_a, type_b, flags,		\
			   p_start, p_end)			\
	for (i = 0, __next_mem_range(&i, flags, type_a, type_b,	\
				     p_start, p_end);		\
	     i != (u64)ULLONG_MAX;					\
	     __next_mem_range(&i, flags, type_a, type_b,		\
			      p_start, p_end))

/**
 * for_each_free_mem_range - iterate through free memblock areas
 * @i: u64 used as loop variable
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 *
 * Walks over free (memory && !reserved) areas of memblock.  Available as
 * soon as memblock is initialized.
 */
#define for_each_free_mem_range(memblock, i, flags, p_start, p_end)	\
	for_each_mem_range(i, &memblock->memory, &memblock->reserved,	\
				flags, p_start, p_end)

void __next_mem_range_rev(u64 *idx, enum memblock_flags flags,
			  struct memblock_type *type_a,
			  struct memblock_type *type_b, phys_addr_t *out_start,
			  phys_addr_t *out_end);

/**
 * for_each_mem_range_rev - reverse iterate through memblock areas from
 * type_a and not included in type_b. Or just type_a if type_b is NULL.
 * @i: u64 used as loop variable
 * @type_a: ptr to memblock_type to iterate
 * @type_b: ptr to memblock_type which excludes from the iteration
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 */
#define for_each_mem_range_rev(i, type_a, type_b, flags,		\
			       p_start, p_end)			\
	for (i = (u64)ULLONG_MAX,					\
		     __next_mem_range_rev(&i, flags, type_a, type_b,\
					  p_start, p_end);	\
	     i != (u64)ULLONG_MAX;					\
	     __next_mem_range_rev(&i, flags, type_a, type_b,	\
				  p_start, p_end))

/**
 * for_each_free_mem_range_reverse - rev-iterate through free memblock areas
 * @i: u64 used as loop variable
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 *
 * Walks over free (memory && !reserved) areas of memblock in reverse
 * order.  Available as soon as memblock is initialized.
 */
#define for_each_free_mem_range_reverse(memblock, i, flags, p_start, p_end)				\
	for_each_mem_range_rev(i, &memblock->memory, &memblock->reserved,	\
			    flags, p_start, p_end)

#define for_each_memblock_type(i, memblock_type, rgn)			\
	for (i = 0, rgn = &memblock_type->regions[0];			\
	     i < memblock_type->cnt;					\
	     i++, rgn = &memblock_type->regions[i])

void __next_reserved_mem_region(struct memblock *mb,
					   u64 *idx,
					   phys_addr_t *out_start,
					   phys_addr_t *out_end);

/**
 * for_each_reserved_mem_region - iterate over all reserved memblock areas
 * @i: u64 used as loop variable
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 *
 * Walks over reserved areas of memblock. Available as soon as memblock
 * is initialized.
 */
#define for_each_reserved_mem_region(mb, i, p_start, p_end)			\
	for (i = 0UL, __next_reserved_mem_region(mb, &i, p_start, p_end);	\
	     i != (u64)ULLONG_MAX;					\
	     __next_reserved_mem_region(mb, &i, p_start, p_end))

void __next_mem_pfn_range(struct memblock *mb,
			  int *idx, phys_addr_t *out_start_pfn,
			  phys_addr_t *out_end_pfn);

#define for_each_mem_pfn_range(mb, i, p_start, p_end)		\
	for (i = -1, __next_mem_pfn_range(mb, &i, p_start, p_end); \
	     i >= 0; __next_mem_pfn_range(mb, &i, p_start, p_end))

#define for_each_memblock(mb, memblock_type, region)					\
	for (region = mb->memblock_type.regions;					\
	     region < (mb->memblock_type.regions + mb->memblock_type.cnt);	\
	     region++)

/*
 * Memblock core function
 */
void memblock_init(struct memblock *mb);

enum memblock_flags	choose_memblock_flags(void);
bool memblock_overlaps_region(struct memblock_type *type,
					phys_addr_t base, phys_addr_t size);
phys_addr_t __memblock_find_in_range(struct memblock *mb,
					phys_addr_t size,
					phys_addr_t align, phys_addr_t start,
					phys_addr_t end,
					enum memblock_flags flags);
phys_addr_t memblock_find_in_range(struct memblock *mb,
					phys_addr_t start,
					phys_addr_t end, phys_addr_t size,
					phys_addr_t align);

int memblock_add_range(struct memblock_type *type,
				phys_addr_t base, phys_addr_t size,
				enum memblock_flags flags);

int memblock_add(struct memblock *mb, phys_addr_t base, phys_addr_t size);
int memblock_remove(struct memblock *mb, phys_addr_t base, phys_addr_t size);
int memblock_free(struct memblock *mb, phys_addr_t base, phys_addr_t size);
int memblock_reserve(struct memblock *mb, phys_addr_t base, phys_addr_t size);

int memblock_mark_nomap(struct memblock *mb, phys_addr_t base, phys_addr_t size);
int memblock_clear_nomap(struct memblock *mb, phys_addr_t base, phys_addr_t size);

int memblock_mark_dma(struct memblock *mb, phys_addr_t base, phys_addr_t size);
int memblock_clear_dma(struct memblock *mb, phys_addr_t base, phys_addr_t size);

int memblock_mark_movable(struct memblock *mb, phys_addr_t base, phys_addr_t size);
int memblock_clear_movable(struct memblock *mb, phys_addr_t base, phys_addr_t size);

phys_addr_t memblock_alloc_range(struct memblock *mb,
					phys_addr_t size, phys_addr_t align,
					phys_addr_t start, phys_addr_t end,
					enum memblock_flags flags);
phys_addr_t memblock_alloc_base(struct memblock *mb,
					phys_addr_t size,
					phys_addr_t align, phys_addr_t max_addr,
					enum memblock_flags flags);
phys_addr_t memblock_phys_alloc(struct memblock *mb,
						phys_addr_t size, phys_addr_t align);

phys_addr_t memblock_alloc_try_raw(struct memblock *mb,
			phys_addr_t size, phys_addr_t align,
			phys_addr_t min_addr, phys_addr_t max_addr);

/*
 * Remaining API functions
 */
phys_addr_t memblock_phys_mem_size(struct memblock *mb);
phys_addr_t memblock_reserved_size(struct memblock *mb);

phys_addr_t memblock_mem_size(struct memblock *mb, u64 limit_pfn);

phys_addr_t memblock_start_of_DRAM(struct memblock *mb);
phys_addr_t memblock_end_of_DRAM(struct memblock *mb);

void memblock_enforce_memory_limit(struct memblock *mb, phys_addr_t limit);

void memblock_cap_memory_range(struct memblock *mb,
						phys_addr_t base, phys_addr_t size);
void memblock_mem_limit_remove_map(struct memblock *mb, phys_addr_t limit);

bool memblock_is_reserved(struct memblock *mb, phys_addr_t addr);
bool memblock_is_memory(struct memblock *mb, phys_addr_t addr);
bool memblock_is_map_memory(struct memblock *mb, phys_addr_t addr);
bool memblock_is_region_memory(struct memblock *mb, phys_addr_t base, phys_addr_t size);
bool memblock_is_region_reserved(struct memblock *mb, phys_addr_t base, phys_addr_t size);

void memblock_trim_memory(struct memblock *mb, phys_addr_t align);

void memblock_set_current_limit(struct memblock *mb, phys_addr_t limit);
phys_addr_t memblock_get_current_limit(struct memblock *mb);

void __memblock_dump_all(struct memblock *mb);

static inline void memblock_dump_all(struct memblock *mb)
{
	if (memblock_debug)
		__memblock_dump_all(mb);
}

/* We are using top down, so it is safe to use 0 here */
#define MEMBLOCK_LOW_LIMIT 0

#ifndef ARCH_LOW_ADDRESS_LIMIT
#define ARCH_LOW_ADDRESS_LIMIT  0xffffffffUL
#endif

static inline phys_addr_t __init memblock_alloc(struct memblock *mb,
						phys_addr_t size,  phys_addr_t align)
{
	return memblock_alloc_try_raw(mb, size, align, MEMBLOCK_LOW_LIMIT,
				      MEMBLOCK_ALLOC_ACCESSIBLE);
}

static inline phys_addr_t __init memblock_alloc_from(struct memblock *mb,
						phys_addr_t size,
						phys_addr_t align,
						phys_addr_t min_addr)
{
	return memblock_alloc_try_raw(mb, size, align, min_addr,
				      MEMBLOCK_ALLOC_ACCESSIBLE);
}

static inline phys_addr_t __init memblock_alloc_low(struct memblock *mb,
							phys_addr_t size,
					       phys_addr_t align)
{
	return memblock_alloc_try_raw(mb, size, align, MEMBLOCK_LOW_LIMIT,
				      ARCH_LOW_ADDRESS_LIMIT);
}

#endif /* !__MEMALLOC_MEMBLOCK_H_ */
