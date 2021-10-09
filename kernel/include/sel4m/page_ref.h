/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MINIX_RT_PAGE_REF_H_
#define __MINIX_RT_PAGE_REF_H_

#include <base/atomic.h>

#include <minix_rt/mm_types.h>

static inline int page_ref_count(struct page *page)
{
	return atomic_read(&page->_refcount);
}

static inline int page_count(struct page *page)
{
	return atomic_read(&compound_head(page)->_refcount);
}

static inline void set_page_count(struct page *page, int v)
{
	atomic_set(&page->_refcount, v);
}

/*
 * Setup the page count before being freed into the page allocator for
 * the first time (boot or memory hotplug)
 */
static inline void init_page_count(struct page *page)
{
	set_page_count(page, 1);
}

static inline void page_ref_add(struct page *page, int nr)
{
	atomic_add(nr, &page->_refcount);
}

static inline void page_ref_sub(struct page *page, int nr)
{
	atomic_sub(nr, &page->_refcount);
}

static inline void page_ref_inc(struct page *page)
{
	atomic_inc(&page->_refcount);
}

static inline void page_ref_dec(struct page *page)
{
	atomic_dec(&page->_refcount);
}

static inline int page_ref_sub_and_test(struct page *page, int nr)
{
	return atomic_sub_and_test(nr, &page->_refcount);
}

static inline int page_ref_inc_return(struct page *page)
{
	return atomic_inc_return(&page->_refcount);
}

static inline int page_ref_dec_and_test(struct page *page)
{
	return atomic_dec_and_test(&page->_refcount);
}

static inline int page_ref_dec_return(struct page *page)
{
	return atomic_dec_return(&page->_refcount);
}

static inline int page_ref_add_unless(struct page *page, int nr, int u)
{
	return atomic_add_unless(&page->_refcount, nr, u);
}

static inline int page_ref_freeze(struct page *page, int count)
{
	return likely(atomic_cmpxchg(&page->_refcount, count, 0) == count);
}

static inline void page_ref_unfreeze(struct page *page, int count)
{
	BUG_ON(page_count(page) != 0);
	BUG_ON(count == 0);

	atomic_set_release(&page->_refcount, count);
}

#endif /* !__MINIX_RT_PAGE_REF_H_ */
