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

#endif /* !__SEL4M_MM_H_ */
