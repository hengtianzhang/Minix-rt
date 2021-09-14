/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_MM_TYPES_H_
#define __SEL4M_MM_TYPES_H_

#ifndef __ASSEMBLY__

#include <base/common.h>
#include <base/list.h>
#include <base/rbtree.h>

#include <sel4m/slub_def.h>
#include <sel4m/object/untype.h>

#include <asm/pgtable-types.h>
#include <asm/mmu.h>

#ifdef CONFIG_HAVE_ALIGNED_STRUCT_PAGE
#define _struct_page_alignment	__aligned(2 * sizeof(unsigned long))
#else
#define _struct_page_alignment
#endif

struct page {
	unsigned long flags;

	union {
		struct {
			struct list_head lru;
			void 		*mapping; /* Reserved */
			unsigned long	index;  /* Reserved */
			unsigned long private;
		};
		struct {
			struct list_head	slab_list;
			struct page		*page;	 /* Reserved */
			struct kmem_cache *slab;
			void *freelist;
			unsigned inuse;
		};
		struct {	/* Tail pages of compound page */
			unsigned long compound_head;	/* Bit zero is set */
			unsigned long compound_dtor;	 /* Reserved */
			unsigned char compound_order;
			unsigned long compound_mapcount; /* Reserved */
		};
	};

	union {
		atomic_t _mapcount;

		unsigned int page_type;
	};

	/* Usage count. *DO NOT USE DIRECTLY*. See page_ref.h */
	atomic_t _refcount;
} _struct_page_alignment;

struct vm_area_struct {
	unsigned long vm_start;
	unsigned long vm_end;

	struct rb_node vm_rb_node;

	struct mm_struct *vm_mm;
	pgprot_t vm_page_prot;
	unsigned long vm_flags;

	unsigned int	nr_pages;

	phys_addr_t		io_space;
};

struct mm_struct {
	struct rb_root vma_rb_root;
	spinlock_t		vma_lock;

	pgd_t * pgd;
	/* Architecture-specific MM context */
	mm_context_t context;

	/**
	 * @mm_count: The number of references to &struct mm_struct
	 * (@mm_users count as 1).
	 *
	 * Use mmgrab()/mmdrop() to modify. When this drops to 0, the
	 * &struct mm_struct is freed.
	 */
	atomic_t mm_count;
	atomic_long_t pgtables_bytes;

	spinlock_t	page_table_lock; /* protect iopgd */

	struct untype_struct untype;
};

extern struct mm_struct init_mm;

/*
 * Used for sizing the vmemmap region on some architectures
 */
#define STRUCT_PAGE_MAX_SHIFT	(order_base_2(sizeof(struct page)))

#endif /* !__ASSEMBLY__ */
#endif /* !__SEL4M_MM_TYPES_H_ */
