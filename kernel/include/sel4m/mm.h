#ifndef __SEL4M_MM_H_
#define __SEL4M_MM_H_

#include <base/string.h>

#include <sel4m/mmzone.h>
#include <sel4m/mm_types.h>
#include <sel4m/memory.h>
#include <sel4m/page.h>
#include <sel4m/page-flags.h>
#include <sel4m/page-flags-layout.h>
#include <sel4m/page_ref.h>

#ifndef page_to_virt
#define page_to_virt(x)	__va(PFN_PHYS(page_to_pfn(x)))
#endif

#define page_address(page) page_to_virt(page)

#define offset_in_page(p)	((unsigned long)(p) & ~PAGE_MASK)

/*
 * On some architectures it is expensive to call memset() for small sizes.
 * Those architectures should provide their own implementation of "struct page"
 * zeroing by defining this macro in <asm/pgtable.h>.
 */
#ifndef mm_zero_struct_page
#define mm_zero_struct_page(pp)  ((void)memset((pp), 0, sizeof(struct page)))
#endif

static inline void page_mapcount_reset(struct page *page)
{
	atomic_set(&(page)->_mapcount, -1);
}

static inline unsigned int compound_order(struct page *page)
{
	if (!PageHead(page))
		return 0;
	return page[1].compound_order;
}

static inline void set_compound_order(struct page *page, unsigned int order)
{
	page[1].compound_order = order;
}

#define page_private(page)		((page)->private)
#define set_page_private(page, v)	((page)->private = (v))

static inline void set_page_zone(struct page *page, enum zone_type zone)
{
	page->flags &= ~(ZONEID_MASK << NODES_PGOFF);
	page->flags |= (zone & ZONEID_MASK) << NODES_PGOFF;
}

static inline void set_page_links(struct page *page, enum zone_type zone,
	unsigned long pfn)
{
	set_page_zone(page, zone);
}

static inline int page_zone_id(const struct page *page)
{
	return (page->flags >> NODES_PGOFF) & ZONEID_MASK;
}

static inline struct zone *page_zone(const struct page *page)
{
	return &NODE_DATA()->node_zones[page_zone_id(page)];
}

static inline pg_data_t *page_pgdat(const struct page *page)
{
	return NODE_DATA();
}

extern void reserve_bootmem_region(phys_addr_t start, phys_addr_t end);
extern void memblock_free_pages(struct page *page, unsigned long pfn,
							unsigned int order);
extern void free_area_init_nodes(void);
extern unsigned long memblock_free_all(void);
extern unsigned long free_reserved_area(void *start, void *end,
					int poison, const char *s);

extern void free_compound_page(struct page *page);
extern void free_unref_page(struct page *page);
extern void __free_pages(struct page *page, unsigned int order);
extern void free_pages(unsigned long addr, unsigned int order);

extern struct page *__alloc_pages(gfp_t gfp_mask, unsigned int order);
extern u64 __get_free_pages(gfp_t gfp_mask, unsigned int order);

/*
 * Drop a ref, return true if the refcount fell to zero (the page has no users)
 */
static inline int put_page_testzero(struct page *page)
{
	BUG_ON(page_ref_count(page) == 0);
	return page_ref_dec_and_test(page);
}

static inline void __put_page(struct page *page)
{
	if (unlikely(PageCompound(page)))
		free_compound_page(page);
	else
		free_unref_page(page);
}

static inline void get_page(struct page *page)
{
	page = compound_head(page);
	/*
	 * Getting a normal page or the head of a compound page
	 * requires to already have an elevated page->_refcount.
	 */
	BUG_ON(page_ref_count(page) <= 0);
	page_ref_inc(page);
}

static inline void put_page(struct page *page)
{
	page = compound_head(page);

	if (put_page_testzero(page))
		__put_page(page);
}

static inline struct page *virt_to_head_page(const void *x)
{
	struct page *page = virt_to_page(x);
	return compound_head(page);
}

extern unsigned long total_physpages;

extern unsigned long nr_managed_pages(void);
extern unsigned long nr_used_pages(void);
extern unsigned long nr_percpu_cache_pages(int cpu);

static inline void mm_pgtables_bytes_init(struct mm_struct *mm)
{
	atomic_long_set(&mm->pgtables_bytes, 0);
}

static inline unsigned long mm_pgtables_bytes(const struct mm_struct *mm)
{
	return atomic_long_read(&mm->pgtables_bytes);
}

static inline void mm_inc_nr_ptes(struct mm_struct *mm)
{
	atomic_long_add(PTRS_PER_PTE * sizeof(pte_t), &mm->pgtables_bytes);
}

static inline void mm_dec_nr_ptes(struct mm_struct *mm)
{
	atomic_long_sub(PTRS_PER_PTE * sizeof(pte_t), &mm->pgtables_bytes);
}

static inline void mm_inc_nr_pmds(struct mm_struct *mm)
{
	atomic_long_add(PTRS_PER_PMD * sizeof(pmd_t), &mm->pgtables_bytes);
}

static inline void mm_dec_nr_pmds(struct mm_struct *mm)
{
	atomic_long_sub(PTRS_PER_PMD * sizeof(pmd_t), &mm->pgtables_bytes);
}

static inline void mm_inc_nr_puds(struct mm_struct *mm)
{
	atomic_long_add(PTRS_PER_PUD * sizeof(pud_t), &mm->pgtables_bytes);
}

static inline void mm_dec_nr_puds(struct mm_struct *mm)
{
	atomic_long_sub(PTRS_PER_PUD * sizeof(pud_t), &mm->pgtables_bytes);
}

#endif /* !__SEL4M_MM_H_ */
