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
#include <base/pfn.h>

#include <sel4m/gfp.h>
#include <sel4m/mm.h>
#include <sel4m/jiffies.h>
#include <sel4m/ktime.h>
#include <sel4m/sched.h>

#include <asm/current.h>

#include <asm/base/processor.h>

#include "internal.h"

struct pglist_data node_data;

static char * const zone_names[MAX_NR_ZONES] = {
	"DMA",
	"Normal",
	"Movable",
};

static void __free_pages_ok(struct page *page, unsigned int order);

static void bad_page(struct page *page, const char *reason,
		unsigned long bad_flags)
{
	static u64 resume;
	static unsigned long nr_shown;
	static unsigned long nr_unshown;

	/*
	 * Allow a burst of 60 reports, then keep quiet for that minute;
	 * or allow a steady drip of one report per second.
	 */
	if (nr_shown == 60) {
		if (time_before(jiffies, resume)) {
			nr_unshown++;
			goto out;
		}
		if (nr_unshown) {
			printf(
			      "BUG: Bad page state: %lu messages suppressed\n",
				nr_unshown);
			nr_unshown = 0;
		}
		nr_shown = 0;
	}
	if (nr_shown++ == 0)
		resume = jiffies + 60 * HZ;

	printf("BUG: Bad page state in process %s  pfn:%05lx\n",
		current->comm, page_to_pfn(page));

	bad_flags &= page->flags;
	if (bad_flags)
		printf("bad because of flags: %#lx(%pGp)\n",
						bad_flags, &bad_flags);

out:
	page_mapcount_reset(page); /* remove PageBuddy */
}

void free_compound_page(struct page *page)
{
	__free_pages_ok(page, compound_order(page));
}

static void prep_compound_page(struct page *page, unsigned int order)
{
	int i;
	int nr_pages = 1 << order;

	set_compound_order(page, order);
	__SetPageHead(page);
	for (i = 1; i < nr_pages; i++) {
		struct page *p = page + i;
		set_page_count(p, 0);
		set_compound_head(p, page);
	}
}

static inline void set_page_order(struct page *page, unsigned int order)
{
	set_page_private(page, order);
	__SetPageBuddy(page);
}

static inline void rmv_page_order(struct page *page)
{
	__ClearPageBuddy(page);
	set_page_private(page, 0);
}

/*
 * This function checks whether a page is free && is the buddy
 * we can coalesce a page and its buddy if
 * (a) the buddy is not in a hole (check before calling!) &&
 * (b) the buddy is in the buddy system &&
 * (c) a page and its buddy have the same order &&
 * (d) a page and its buddy are in the same zone.
 *
 * For recording whether a page is in the buddy system, we set PageBuddy.
 * Setting, clearing, and testing PageBuddy is serialized by zone->lock.
 *
 * For recording page's order, we use page_private(page).
 */
static inline int page_is_buddy(struct page *page, struct page *buddy,
							unsigned int order)
{
	if (PageBuddy(buddy) && page_order(buddy) == order) {
		/*
		 * zone check is done late to avoid uselessly
		 * calculating zone/node ids for pages that could
		 * never merge.
		 */
		if (page_zone_id(page) != page_zone_id(buddy))
			return 0;

		BUG_ON(page_count(buddy) != 0);

		return 1;
	}
	return 0;
}

static inline void __free_one_page(struct page *page,
		unsigned long pfn,
		struct zone *zone, unsigned int order)
{
	unsigned long combined_pfn;
	unsigned long uninitialized_var(buddy_pfn);
	struct page *buddy;
	unsigned int max_order = MAX_ORDER;

	BUG_ON(!zone_is_initialized(zone));
	BUG_ON(page->flags & PAGE_FLAGS_CHECK_AT_PREP);

	BUG_ON(pfn & ((1 << order) - 1));

	while (order < max_order - 1) {
		buddy_pfn = __find_buddy_pfn(pfn, order);
		buddy = page + (buddy_pfn - pfn);

		if (!pfn_valid_within(buddy_pfn))
			goto done_merging;
		if (!page_is_buddy(page, buddy, order))
			goto done_merging;

		list_del(&buddy->lru);
		zone->free_area[order].nr_free--;
		rmv_page_order(buddy);

		combined_pfn = buddy_pfn & pfn;
		page = page + (combined_pfn - pfn);
		pfn = combined_pfn;
		order++;
	}

done_merging:
	set_page_order(page, order);

	/*
	 * If this is not the largest possible page, check if the buddy
	 * of the next-highest order is free. If it is, it's possible
	 * that pages are being freed that will coalesce soon. In case,
	 * that is happening, add the free page to the tail of the list
	 * so it's less likely to be used soon and more likely to be merged
	 * as a higher order page
	 */
	if ((order < MAX_ORDER - 2) && pfn_valid_within(buddy_pfn)) {
		struct page *higher_page, *higher_buddy;
		combined_pfn = buddy_pfn & pfn;
		higher_page = page + (combined_pfn - pfn);
		buddy_pfn = __find_buddy_pfn(combined_pfn, order + 1);
		higher_buddy = higher_page + (buddy_pfn - combined_pfn);
		if (pfn_valid_within(buddy_pfn) &&
		    page_is_buddy(higher_page, higher_buddy, order + 1)) {
			list_add_tail(&page->lru,
				&zone->free_area[order].free_list);
			goto out;
		}
	}

	list_add(&page->lru, &zone->free_area[order].free_list);
out:
	zone->free_area[order].nr_free++;
}

/*
 * A bad page could be due to a number of fields. Instead of multiple branches,
 * try and check multiple fields with one check. The caller must do a detailed
 * check if necessary.
 */
static inline bool page_expected_state(struct page *page,
					unsigned long check_flags)
{
	if (unlikely(atomic_read(&page->_mapcount) != -1))
		return false;

	if (unlikely(page_ref_count(page) | (page->flags & check_flags)))
		return false;

	return true;
}

static void free_pages_check_bad(struct page *page)
{
	const char *bad_reason;
	unsigned long bad_flags;

	bad_reason = NULL;
	bad_flags = 0;

	if (unlikely(atomic_read(&page->_mapcount) != -1))
		bad_reason = "nonzero mapcount";
	if (unlikely(page_ref_count(page) != 0))
		bad_reason = "nonzero _refcount";
	if (unlikely(page->flags & PAGE_FLAGS_CHECK_AT_FREE)) {
		bad_reason = "PAGE_FLAGS_CHECK_AT_FREE flag(s) set";
		bad_flags = PAGE_FLAGS_CHECK_AT_FREE;
	}

	bad_page(page, bad_reason, bad_flags);
}

static inline int free_pages_check(struct page *page)
{
	if (likely(page_expected_state(page, PAGE_FLAGS_CHECK_AT_FREE)))
		return 0;

	/* Something has gone sideways, find it */
	free_pages_check_bad(page);
	return 1;
}

static int free_tail_pages_check(struct page *head_page, struct page *page)
{
	int ret = 1;

	/*
	 * We rely page->lru.next never has bit 0 set, unless the page
	 * is PageTail(). Let's make sure that's true even for poisoned ->lru.
	 */
	BUILD_BUG_ON((unsigned long)LIST_POISON1 & 1);

	if (unlikely(!PageTail(page))) {
		bad_page(page, "PageTail not set", 0);
		goto out;
	}
	if (unlikely(compound_head(page) != head_page)) {
		bad_page(page, "compound_head not consistent", 0);
		goto out;
	}
	ret = 0;
out:
	clear_compound_head(page);
	return ret;
}

void __weak arch_free_page(struct page *page, int order) {}

static __always_inline bool free_pages_prepare(struct page *page,
					unsigned int order, bool check_free)
{
	int bad = 0;

	BUG_ON(PageTail(page));

	/*
	 * Check tail pages before head page information is cleared to
	 * avoid checking PageCompound for order-0 pages.
	 */
	if (unlikely(order)) {
		bool compound = PageCompound(page);
		int i;

		BUG_ON(compound && compound_order(page) != order);

		for (i = 1; i < (1 << order); i++) {
			if (compound)
				bad += free_tail_pages_check(page, page + i);
			if (unlikely(free_pages_check(page + i))) {
				bad++;
				continue;
			}
			(page + i)->flags &= ~PAGE_FLAGS_CHECK_AT_PREP;
		}
	}

	if (check_free)
		bad += free_pages_check(page);
	if (bad)
		return false;

	page->flags &= ~PAGE_FLAGS_CHECK_AT_PREP;

	arch_free_page(page, order);

	return true;
}

static inline bool free_pcp_prepare(struct page *page)
{
	return free_pages_prepare(page, 0, true);
}

static inline void prefetch_buddy(struct page *page)
{
	unsigned long pfn = page_to_pfn(page);
	unsigned long buddy_pfn = __find_buddy_pfn(pfn, 0);
	struct page *buddy = page + (buddy_pfn - pfn);

	prefetch(buddy);
}

/*
 * Frees a number of pages from the PCP lists
 * Assumes all pages on list are in same zone, and of same order.
 * count is the number of pages to free.
 *
 * If the zone was previously in an "all pages pinned" state then look to
 * see if this freeing clears that state.
 *
 * And clear the zone's pages_scanned counter, to hold off the "all pages are
 * pinned" detection logic.
 */
static void free_pcppages_bulk(struct zone *zone, int count,
					struct per_cpu_pages *pcp)
{
	int batch_free = 0;
	int prefetch_nr = 0;
	struct page *page, *tmp;
	LIST_HEAD(head);

	while (count) {
		struct list_head *list;

		/*
		 * Remove pages from lists in a round-robin fashion. A
		 * batch_free count is maintained that is incremented when an
		 * empty list is encountered.  This is so more pages are freed
		 * off fuller lists instead of spinning excessively around empty
		 * lists
		 */
		batch_free++;
		list = &pcp->lists;

		do {
			page = list_last_entry(list, struct page, lru);
			/* must delete to avoid corrupting pcp list */
			list_del(&page->lru);
			pcp->count--;

			list_add_tail(&page->lru, &head);

			/*
			 * We are going to put the page back to the global
			 * pool, prefetch its buddy to speed up later access
			 * under zone->lock. It is believed the overhead of
			 * an additional test and calculating buddy_pfn here
			 * can be offset by reduced memory latency later. To
			 * avoid excessive prefetching due to large count, only
			 * prefetch buddy for the first pcp->batch nr of pages.
			 */
			if (prefetch_nr++ < pcp->batch)
				prefetch_buddy(page);
		} while (--count && --batch_free && !list_empty(list));
	}

	spin_lock(&zone->lock);
	/*
	 * Use safe version since after __free_one_page(),
	 * page->lru.next will not point to original list.
	 */
	list_for_each_entry_safe(page, tmp, &head, lru)
		__free_one_page(page, page_to_pfn(page), zone, 0);
	spin_unlock(&zone->lock);
}

static void free_one_page(struct zone *zone,
				struct page *page, unsigned long pfn,
				unsigned int order)
{
	spin_lock(&zone->lock);
	__free_one_page(page, pfn, zone, order);
	spin_unlock(&zone->lock);
}

static void __init_single_page(struct page *page, unsigned long pfn,
				unsigned long zone)
{
	mm_zero_struct_page(page);
	set_page_links(page, zone, pfn);
	init_page_count(page);
	page_mapcount_reset(page);

	INIT_LIST_HEAD(&page->lru);
}

/*
 * Initialised pages do not have PageReserved set. This function is
 * called for each range allocated by the bootmem allocator and
 * marks the pages PageReserved. The remaining valid pages are later
 * sent to the buddy page allocator.
 */
void reserve_bootmem_region(phys_addr_t start, phys_addr_t end)
{
	unsigned long start_pfn = PFN_DOWN(start);
	unsigned long end_pfn = PFN_UP(end);

	for (; start_pfn < end_pfn; start_pfn++) {
		if (pfn_valid(start_pfn)) {
			struct page *page = pfn_to_page(start_pfn);

			/* Avoid false-positive PageTail() */
			INIT_LIST_HEAD(&page->lru);

			/*
			 * no need for atomic set_bit because the struct
			 * page is not visible yet so nobody should
			 * access it yet.
			 */
			__SetPageReserved(page);
		}
	}
}

static void __free_pages_ok(struct page *page, unsigned int order)
{
	u64 flags;
	unsigned long pfn = page_to_pfn(page);

	if (!free_pages_prepare(page, order, true))
		return;

	local_irq_save(flags);
	free_one_page(page_zone(page), page, pfn, order);
	local_irq_restore(flags);
}

static void __init __free_pages_boot_core(struct page *page, unsigned int order)
{
	unsigned int nr_pages = 1 << order;
	struct page *p = page;
	unsigned int loop;

	prefetchw(p);
	for (loop = 0; loop < (nr_pages - 1); loop++, p++) {
		prefetchw(p + 1);
		__ClearPageReserved(p);
		set_page_count(p, 0);
	}
	__ClearPageReserved(p);
	set_page_count(p, 0);

	atomic_long_add(nr_pages, &page_zone(page)->managed_pages);
	set_page_refcounted(page);
	__free_pages(page, order);
}

void __init memblock_free_pages(struct page *page, unsigned long pfn,
							unsigned int order)
{
	return __free_pages_boot_core(page, order);
}

static void __init __free_pages_memory(unsigned long start, unsigned long end)
{
	int order;

	while (start < end) {
		order = min(MAX_ORDER - 1UL, __ffs(start));

		while (start + (1UL << order) > end)
			order--;

		memblock_free_pages(pfn_to_page(start), start, order);

		start += (1UL << order);
	}
}

unsigned long __init memblock_free_all(void)
{
	u64 i;
	phys_addr_t start, end;
	phys_addr_t start_pfn, end_pfn;
	unsigned long pages = 0;

	for_each_reserved_mem_region(&memblock_kernel, i, &start, &end)
		reserve_bootmem_region(start, end);

	for_each_free_mem_range(&memblock_kernel, i,
					MEMBLOCK_NONE, &start, &end) {
		start_pfn = PFN_UP(start);
		end_pfn = PFN_DOWN(end);

		if (start_pfn >= end_pfn)
			continue;

		pages += end_pfn - start_pfn;
		__free_pages_memory(start_pfn, end_pfn);
	}

	for_each_free_mem_range(&memblock_kernel, i,
					MEMBLOCK_DMA, &start, &end) {
		start_pfn = PFN_UP(start);
		end_pfn = PFN_DOWN(end);

		if (start_pfn >= end_pfn)
			continue;
		
		pages += end_pfn - start_pfn;
		__free_pages_memory(start_pfn, end_pfn);
	}

	for_each_free_mem_range(&memblock_kernel, i,
					MEMBLOCK_MOVABLE, &start, &end) {
		start_pfn = PFN_UP(start);
		end_pfn = PFN_DOWN(end);

		if (start_pfn >= end_pfn)
			continue;

		pages += end_pfn - start_pfn;
		__free_pages_memory(start_pfn, end_pfn);
	}

	return pages;
}

/*
 * The order of subdivision here is critical for the IO subsystem.
 * Please do not alter this order without good reasons and regression
 * testing. Specifically, as large blocks of memory are subdivided,
 * the order in which smaller blocks are delivered depends on the order
 * they're subdivided in this function. This is the primary factor
 * influencing the order in which pages are delivered to the IO
 * subsystem according to empirical testing, and this is also justified
 * by considering the behavior of a buddy system containing a single
 * large block of memory acted on by a series of small allocations.
 * This behavior is a critical factor in sglist merging's success.
 *
 * -- nyc
 */
static inline void expand(struct zone *zone, struct page *page,
	int low, int high, struct free_area *area)
{
	unsigned long size = 1 << high;

	while (high > low) {
		area--;
		high--;
		size >>= 1;

		list_add(&page[size].lru, &area->free_list);
		area->nr_free++;
		set_page_order(&page[size], high);
	}
}

static void check_new_page_bad(struct page *page)
{
	const char *bad_reason = NULL;
	unsigned long bad_flags = 0;

	if (unlikely(atomic_read(&page->_mapcount) != -1))
		bad_reason = "nonzero mapcount";
	if (unlikely(page_ref_count(page) != 0))
		bad_reason = "nonzero _count";
	if (unlikely(page->flags & __PG_HWPOISON)) {
		bad_reason = "HWPoisoned (hardware-corrupted)";
		bad_flags = __PG_HWPOISON;
		/* Don't complain about hwpoisoned pages */
		page_mapcount_reset(page); /* remove PageBuddy */
		return;
	}
	if (unlikely(page->flags & PAGE_FLAGS_CHECK_AT_PREP)) {
		bad_reason = "PAGE_FLAGS_CHECK_AT_PREP flag set";
		bad_flags = PAGE_FLAGS_CHECK_AT_PREP;
	}

	bad_page(page, bad_reason, bad_flags);
}

/*
 * This page is about to be returned from the page allocator
 */
static inline int check_new_page(struct page *page)
{
	if (likely(page_expected_state(page,
				PAGE_FLAGS_CHECK_AT_PREP|__PG_HWPOISON)))
		return 0;

	check_new_page_bad(page);
	return 1;
}

static bool check_new_pcp(struct page *page)
{
	return check_new_page(page);
}

static bool check_new_pages(struct page *page, unsigned int order)
{
	int i;
	for (i = 0; i < (1 << order); i++) {
		struct page *p = page + i;

		if (unlikely(check_new_page(p)))
			return true;
	}

	return false;
}

static inline void post_alloc_hook(struct page *page, unsigned int order,
				gfp_t gfp_mask)
{
	set_page_private(page, 0);
	set_page_refcounted(page);
}

static void prep_new_page(struct page *page, unsigned int order, gfp_t gfp_mask)
{
	int i;

	post_alloc_hook(page, order, gfp_mask);

	if ((gfp_mask & __GFP_ZERO))
		for (i = 0; i < (1 << order); i++)
			clear_page(page_address(page + i));

	if (order)
		prep_compound_page(page, order);
}

/*
 * Go through the free lists for the given migratetype and remove
 * the smallest available page from the freelists
 */
static __always_inline
struct page *__rmqueue_smallest(struct zone *zone, unsigned int order)
{
	unsigned int current_order;
	struct free_area *area;
	struct page *page;

	/* Find a page of the appropriate size in the preferred list */
	for (current_order = order; current_order < MAX_ORDER; ++current_order) {
		area = &(zone->free_area[current_order]);
		page = list_first_entry_or_null(&area->free_list,
							struct page, lru);
		if (!page)
			continue;
		list_del(&page->lru);
		rmv_page_order(page);
		area->nr_free--;
		expand(zone, page, order, current_order, area);

		return page;
	}

	return NULL;
}

/*
 * Do the hard work of removing an element from the buddy allocator.
 * Call me with the zone->lock already held.
 */
static __always_inline struct page *
__rmqueue(struct zone *zone, unsigned int order, gfp_t gfp_mask)
{
	return __rmqueue_smallest(zone, order);
}

/*
 * Obtain a specified number of elements from the buddy allocator, all under
 * a single hold of the lock, for efficiency.  Add them to the supplied list.
 * Returns the number of new pages which were placed at *list.
 */
static int rmqueue_bulk(struct zone *zone, unsigned int order,
			unsigned long count, struct list_head *list,
			gfp_t gfp_mask)
{
	int i, alloced = 0;

	spin_lock(&zone->lock);
	for (i = 0; i < count; ++i) {
		struct page *page = __rmqueue(zone, order, gfp_mask);
		if (unlikely(page == NULL))
			break;

		/*
		 * Split buddy pages returned by expand() are received here in
		 * physical page order. The page is added to the tail of
		 * caller's list. From the callers perspective, the linked list
		 * is ordered by page number under some conditions. This is
		 * useful for IO devices that can forward direction from the
		 * head, thus also in the physical page order. This is useful
		 * for IO devices that can merge IO requests if the physical
		 * pages are ordered properly.
		 */
		list_add_tail(&page->lru, list);
		alloced++;
	}

	spin_unlock(&zone->lock);
	return alloced;
}

static bool free_unref_page_prepare(struct page *page, unsigned long pfn)
{
	if (!free_pcp_prepare(page))
		return false;

	return true;
}

static void free_unref_page_commit(struct page *page, unsigned long pfn)
{
	struct zone *zone = page_zone(page);
	struct per_cpu_pages *pcp;

	pcp = &(&zone->pageset[smp_processor_id()])->pcp;
	list_add(&page->lru, &pcp->lists);
	pcp->count++;
	if (pcp->count >= pcp->high) {
		unsigned long batch = READ_ONCE(pcp->batch);
		free_pcppages_bulk(zone, batch, pcp);
	}
}

/*
 * Free a 0-order page
 */
void free_unref_page(struct page *page)
{
	u64 flags;
	unsigned long pfn = page_to_pfn(page);

	if (!free_unref_page_prepare(page, pfn))
		return;

	local_irq_save(flags);
	free_unref_page_commit(page, pfn);
	local_irq_restore(flags);
}

/* Remove page from the per-cpu list, caller must protect the list */
static struct page *__rmqueue_pcplist(struct zone *zone,
			gfp_t gfp_mask,
			struct per_cpu_pages *pcp,
			struct list_head *list)
{
	struct page *page;

	do {
		if (list_empty(list)) {
			pcp->count += rmqueue_bulk(zone, 0,
					pcp->batch, list, gfp_mask);
			if (unlikely(list_empty(list)))
				return NULL;
		}

		page = list_first_entry(list, struct page, lru);
		list_del(&page->lru);
		pcp->count--;
	} while (check_new_pcp(page));

	return page;
}

/* Lock and remove page from the per-cpu list */
static struct page *rmqueue_pcplist(struct zone *zone,
			unsigned int order, gfp_t gfp_mask)
{
	struct per_cpu_pages *pcp;
	struct list_head *list;
	struct page *page;
	u64 flags;

	local_irq_save(flags);
	pcp = &(&zone->pageset[smp_processor_id()])->pcp;
	list = &pcp->lists;
	page = __rmqueue_pcplist(zone, gfp_mask, pcp, list);
	local_irq_restore(flags);
	return page;
}

/*
 * Allocate a page from the given zone. Use pcplists for order-0 allocations.
 */
static inline
struct page *rmqueue(struct zone *zone, unsigned int order, gfp_t gfp_mask)
{
	u64 flags;
	struct page *page;

	if (likely(order == 0)) {
		page = rmqueue_pcplist(zone, order, gfp_mask);
		goto out;
	}

	spin_lock_irqsave(&zone->lock, flags);

	do {
		page = __rmqueue_smallest(zone, order);
	} while (page && check_new_pages(page, order));
	spin_unlock(&zone->lock);
	if (!page)
		goto failed;

	local_irq_restore(flags);

out:
	return page;

failed:
	local_irq_restore(flags);
	return NULL;
}

/*
 * get_page_from_freelist goes through the zonelist trying to allocate
 * a page.
 */
static struct page *
get_page_from_freelist(gfp_t gfp_mask, unsigned int order, const struct alloc_context *ac)
{
	struct page *page;
	struct zone *zone = NODE_DATA()->node_zones + ac->zoneidx;

	page = rmqueue(zone, order, gfp_mask);
	if (page) {
		prep_new_page(page, order, gfp_mask);

		return page;
	}

	return NULL;
}

static inline bool prepare_alloc_pages(gfp_t gfp_mask, unsigned int order,
		struct alloc_context *ac)
{
	ac->zoneidx = gfp_zone(gfp_mask);

	return true;
}

/*
 * This is the 'heart' of the zoned buddy allocator.
 */
struct page *__alloc_pages(gfp_t gfp_mask, unsigned int order)
{
	struct page *page;
	struct alloc_context ac = { };

	if (unlikely(order >= MAX_ORDER)) {
		WARN_ON_ONCE(!(gfp_mask & __GFP_NOWARN));
		return NULL;
	}

	if (!prepare_alloc_pages(gfp_mask, order, &ac))
		return NULL;

	/* First allocation attempt */
	page = get_page_from_freelist(gfp_mask, order, &ac);

	return page;
}

u64 __get_free_pages(gfp_t gfp_mask, unsigned int order)
{
	struct page *page;

	page = __alloc_pages(gfp_mask, order);
	if (!page)
		return 0;
	return (u64)page_address(page);
}

static inline void free_the_page(struct page *page, unsigned int order)
{
	if (order == 0)		/* Via pcp? */
		free_unref_page(page);
	else
		__free_pages_ok(page, order);
}

void __free_pages(struct page *page, unsigned int order)
{
	if (put_page_testzero(page))
		free_the_page(page, order);
}

void free_pages(unsigned long addr, unsigned int order)
{
	if (addr != 0) {
		BUG_ON(!virt_addr_valid((void *)addr));
		__free_pages(virt_to_page((void *)addr), order);
	}
}

static void __init zone_init_free_lists(struct zone *zone)
{
	unsigned long order;

	for_each_order(order) {
		INIT_LIST_HEAD(&zone->free_area[order].free_list);
		zone->free_area[order].nr_free = 0;
	}
}

static void __init pageset_init(struct per_cpu_pageset *p)
{
	struct per_cpu_pages *pcp;

	memset(p, 0, sizeof(*p));

	pcp = &p->pcp;
	INIT_LIST_HEAD(&pcp->lists);
}

/*
 * pcp->high and pcp->batch values are related and dependent on one another:
 * ->batch must never be higher then ->high.
 * The following function updates them in a safe manner without read side
 * locking.
 *
 * Any new users of pcp->batch and pcp->high should ensure they can cope with
 * those fields changing asynchronously (acording the the above rule).
 *
 * mutex_is_locked(&pcp_batch_high_lock) required when calling this function
 * outside of boot time (or some other assurance that no concurrent updaters
 * exist).
 */
static void __init pageset_update(struct per_cpu_pages *pcp, unsigned long high,
		unsigned long batch)
{
	/* start with a fail safe value for batch */
	pcp->batch = 1;
	smp_wmb();

       /* Update high, then batch, in order */
	pcp->high = high;
	smp_wmb();

	pcp->batch = batch;
}

/* a companion to pageset_set_high() */
static void __init pageset_set_batch(struct per_cpu_pageset *p, unsigned long batch)
{
	pageset_update(&p->pcp, 6 * batch, max(1UL, 1 * batch));
}

static int __init zone_batchsize(struct zone *zone)
{
	int batch;

	/*
	 * The per-cpu-pages pools are set to around 1000th of the
	 * size of the zone.
	 */
	batch = zone_managed_pages(zone) / 1024;
	/* But no more than a meg. */
	if (batch * PAGE_SIZE > 1024 * 1024)
		batch = (1024 * 1024) / PAGE_SIZE;
	batch /= 4;		/* We effectively *= 4 below */
	if (batch < 1)
		batch = 1;

	/*
	 * Clamp the batch to a 2^n - 1 value. Having a power
	 * of 2 value was found to be more likely to have
	 * suboptimal cache aliasing properties in some cases.
	 *
	 * For example if 2 tasks are alternately allocating
	 * batches of pages, one task can end up with a lot
	 * of pages of one half of the possible page colors
	 * and the other with pages of the other colors.
	 */
	batch = rounddown_pow_of_two(batch + batch/2) - 1;

	return batch;
}

static void __init pageset_set_high_and_batch(struct zone *zone,
				       struct per_cpu_pageset *pcp)
{
	pageset_set_batch(pcp, zone_batchsize(zone));
}

static void __init zone_pageset_init(struct zone *zone, int cpu)
{
	struct per_cpu_pageset *pcp = &zone->pageset[cpu];

	pageset_init(pcp);
	pageset_set_high_and_batch(zone, pcp);
}

static void __init free_area_init_node(phys_addr_t node_start_pfn)
{
	u64 i;
	size_t totalreserved_pages = 0;
	phys_addr_t start_pfn, end_pfn;
	enum zone_type j;
	struct zone *zone;
	phys_addr_t min_pfn = ULONG_MAX;
	pg_data_t *pgdat = NODE_DATA();
	struct page *page;

	pgdat->node_start_pfn = node_start_pfn;

	for_each_reserved_mem_region(&memblock_kernel, i, &start_pfn, &end_pfn)
		totalreserved_pages += PFN_UP(end_pfn) - PFN_DOWN(start_pfn);

	pgdat->totalreserve_pages = totalreserved_pages;

	for (j = 0; j < MAX_NR_ZONES; j++) {
		int cpu;

		zone = pgdat->node_zones + j;

		for_each_possible_cpu(cpu)
			zone_pageset_init(zone, cpu);

		zone->zone_start_pfn = 0;
		zone->zone_pgdat = pgdat;
		zone->name = zone_names[j];
		atomic_long_set(&zone->managed_pages, 0);
		spin_lock_init(&zone->lock);
		zone_init_free_lists(zone);
	}

	zone = pgdat->node_zones + ZONE_DMA;
	zone->zone_start_pfn = 0;
	for_each_free_mem_range(&memblock_kernel, i, MEMBLOCK_DMA,
					&start_pfn, &end_pfn) {
		phys_addr_t pfn;

		start_pfn = PFN_UP(start_pfn);
		end_pfn = PFN_DOWN(end_pfn);

		min_pfn = min(min_pfn, start_pfn);
		for (pfn = start_pfn; pfn < end_pfn; pfn++) {
			page = pfn_to_page(pfn);
			__init_single_page(page, pfn, ZONE_DMA);
		}
	}
	if (min_pfn != ULONG_MAX)
		zone->zone_start_pfn = min_pfn;
	zone->initialized = 1;

	zone = pgdat->node_zones + ZONE_MOVABLE;
	min_pfn = ULONG_MAX;
	for_each_free_mem_range(&memblock_kernel, i, MEMBLOCK_MOVABLE,
					&start_pfn, &end_pfn) {
		phys_addr_t pfn;

		start_pfn = PFN_UP(start_pfn);
		end_pfn = PFN_DOWN(end_pfn);

		min_pfn = min(min_pfn, start_pfn);
		for (pfn = start_pfn; pfn < end_pfn; pfn++) {
			page = pfn_to_page(pfn);
			__init_single_page(page, pfn, ZONE_MOVABLE);
		}
	}
	if (min_pfn != ULONG_MAX)
		zone->zone_start_pfn = min_pfn;
	zone->initialized = 1;

	zone = pgdat->node_zones + ZONE_NORMAL;
	min_pfn = ULONG_MAX;
	for_each_free_mem_range(&memblock_kernel, i, MEMBLOCK_NONE,
					&start_pfn, &end_pfn) {
		phys_addr_t pfn;

		start_pfn = PFN_UP(start_pfn);
		end_pfn = PFN_DOWN(end_pfn);

		min_pfn = min(min_pfn, start_pfn);

		for (pfn = start_pfn; pfn < end_pfn; pfn++) {
			page = pfn_to_page(pfn);
			__init_single_page(page, pfn, ZONE_NORMAL);
		}
	}
	if (min_pfn != ULONG_MAX)
		zone->zone_start_pfn = min_pfn;
	zone->initialized = 1;

	for_each_reserved_mem_region(&memblock_kernel, i, &start_pfn, &end_pfn) {
		phys_addr_t pfn;

		start_pfn = PFN_UP(start_pfn);
		end_pfn = PFN_DOWN(end_pfn);

		for (pfn = start_pfn; pfn < end_pfn; pfn++) {
			page = pfn_to_page(pfn);
			__init_single_page(page, pfn, ZONE_NORMAL);
		}
	}
}

static phys_addr_t __init find_min_pfn_for_mem(void)
{
	phys_addr_t min_pfn = ULONG_MAX;
	phys_addr_t start_pfn;
	int i;

	for_each_mem_pfn_range(&memblock_kernel, i, &start_pfn, NULL)
		min_pfn = min(min_pfn, start_pfn);

	if (min_pfn == ULONG_MAX) {
		printf("Could not find start_pfn for mem\n");
		return 0;
	}

	return min_pfn;
}

unsigned long total_physpages;

void __init free_area_init_nodes(void)
{
	phys_addr_t start_pfn, end_pfn;
	int i, has_zone;
	struct zone *zone;
	u64 j;

	total_physpages = 0;
	/* Print out the early node map */
	printf("Early memory node ranges\n");
	for_each_mem_pfn_range(&memblock_kernel, i, &start_pfn, &end_pfn) {
		printf("  node: [mem %#018Lx-%#018Lx]\n",
			(u64)start_pfn << PAGE_SHIFT,
			((u64)end_pfn << PAGE_SHIFT) - 1);
		total_physpages += (end_pfn - start_pfn);
	}

	printf("Zone ranges:\n");
	has_zone = 0;
	zone = NODE_DATA()->node_zones + ZONE_NORMAL;
	atomic_long_set(&zone->managed_pages, 0);
	for_each_free_mem_range(&memblock_kernel, j, MEMBLOCK_NONE,
						&start_pfn, &end_pfn) {
		has_zone = 1;
		printf("  %-8s [mem %#018Lx-%#018Lx]\n",
				zone_names[ZONE_NORMAL], (u64)start_pfn, ((u64)end_pfn) -1);
		
		start_pfn = PFN_UP(start_pfn);
		end_pfn = PFN_DOWN(end_pfn);
		atomic_long_add(end_pfn - start_pfn, &zone->managed_pages);
	}
	if (!has_zone)
		printf("  %-8s empty\n", zone_names[ZONE_NORMAL]);

	has_zone = 0;
	zone = NODE_DATA()->node_zones + ZONE_DMA;
	atomic_long_set(&zone->managed_pages, 0);
	for_each_free_mem_range(&memblock_kernel, j, MEMBLOCK_DMA,
						&start_pfn, &end_pfn) {
		has_zone = 1;
		printf("  %-8s [mem %#018Lx-%#018Lx]\n",
				zone_names[ZONE_DMA], (u64)start_pfn, ((u64)end_pfn) -1);

		start_pfn = PFN_UP(start_pfn);
		end_pfn = PFN_DOWN(end_pfn);
		atomic_long_add(end_pfn - start_pfn, &zone->managed_pages);
	}
	if (!has_zone)
		printf("  %-8s empty\n", zone_names[ZONE_DMA]);

	has_zone = 0;
	zone = NODE_DATA()->node_zones + ZONE_MOVABLE;
	atomic_long_set(&zone->managed_pages, 0);
	for_each_free_mem_range(&memblock_kernel, j, MEMBLOCK_MOVABLE,
						&start_pfn, &end_pfn) {
		has_zone = 1;
		printf("  %-8s [mem %#018Lx-%#018Lx]\n",
				zone_names[ZONE_DMA], (u64)start_pfn, ((u64)end_pfn) -1);

		start_pfn = PFN_UP(start_pfn);
		end_pfn = PFN_DOWN(end_pfn);
		atomic_long_add(end_pfn - start_pfn, &zone->managed_pages);
	}
	if (!has_zone)
		printf("  %-8s empty\n", zone_names[ZONE_MOVABLE]);

	free_area_init_node(find_min_pfn_for_mem());

	memblock_free_all();
}

static inline void free_reserved_page(struct page *page)
{
	ClearPageReserved(page);
	init_page_count(page);
	atomic_long_add(1, &page_zone(page)->managed_pages);
	__free_page(page);
}

unsigned long free_reserved_area(void *start, void *end, int poison, const char *s)
{
	void *pos;
	unsigned long pages = 0;

	start = (void *)PAGE_ALIGN((unsigned long)start);
	end = (void *)((unsigned long)end & PAGE_MASK);
	for (pos = start; pos < end; pos += PAGE_SIZE, pages++) {
		struct page *page = virt_to_page(pos);
		void *direct_map_addr;

		/*
		 * 'direct_map_addr' might be different from 'pos'
		 * because some architectures' virt_to_page()
		 * work with aliases.  Getting the direct map
		 * address ensures that we get a _writeable_
		 * alias for the memset().
		 */
		direct_map_addr = page_address(page);
		if ((unsigned int)poison <= 0xFF)
			memset(direct_map_addr, poison, PAGE_SIZE);

		free_reserved_page(page);
	}

	if (pages && s)
		printf("Freeing %s memory: %ldK\n",
			s, pages << (PAGE_SHIFT - 10));

	return pages;
}

unsigned long nr_managed_pages(void)
{
	enum zone_type i;
	unsigned long total_pages = 0;
	struct zone *zone;

	for (i = 0; i < MAX_NR_ZONES; i++) {
		zone = NODE_DATA()->node_zones + i;
		total_pages += zone_managed_pages(zone);
	}

	return total_pages;
}

unsigned long nr_free_pages(void)
{
	enum zone_type i;
	unsigned long nr_free = 0;
	struct zone *zone;

	for (i = 0; i < MAX_NR_ZONES; i++) {
		int order;
		zone = NODE_DATA()->node_zones + i;

		for_each_order(order)
			nr_free += zone->free_area[order].nr_free * (1 << order);
	}

	return nr_free;
}

unsigned long nr_percpu_cache_pages(int cpu)
{
	enum zone_type i;
	unsigned long total_pages = 0;
	struct zone *zone;

	for (i = 0; i < MAX_NR_ZONES; i++) {
		zone = NODE_DATA()->node_zones + i;
		total_pages += zone->pageset[cpu].pcp.count;
	}

	return total_pages;
}
