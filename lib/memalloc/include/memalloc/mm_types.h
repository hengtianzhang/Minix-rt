/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MEMALLOC_MM_TYPES_H_
#define __MEMALLOC_MM_TYPES_H_

#include <base/compiler.h>
#include <base/types.h>
#include <base/list.h>
#include <base/atomic.h>

#ifdef CONFIG_HAVE_ALIGNED_STRUCT_PAGE
#define _struct_page_alignment	__aligned(2 * sizeof(unsigned long))
#else
#define _struct_page_alignment
#endif

struct page {
	unsigned long flags;
	/*
	 * Five words (20/40 bytes) are available in this union.
	 * WARNING: bit 0 of the first word is used for PageTail(). That
	 * means the other users of this union MUST NOT use the bit to
	 * avoid collision and false-positive PageTail().
	 */
	union {
		struct {
			struct list_head 	lru;
			unsigned long		private;
		};
	};

	atomic_t _refcount;
} _struct_page_alignment;

#endif /* !__MEMALLOC_MM_TYPES_H_ */
