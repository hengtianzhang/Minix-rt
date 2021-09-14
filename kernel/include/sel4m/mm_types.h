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

	struct vm_area_struct *vm_next, *vm_prev;

	struct rb_node vm_rb;

	struct mm_struct *vm_mm;
	pgprot_t vm_page_prot;
	unsigned long vm_flags;

	struct page **pud_pages;
	unsigned int	nr_pud_pages;

	struct page **pmd_pages;
	unsigned int	nr_pmd_pages;

	struct page **pte_pages;
	unsigned int	nr_pte_pages;

	struct page **pages;
	unsigned int	nr_pages;

	phys_addr_t		io_space;
};

struct mm_struct {
	struct vm_area_struct *mmap;		/* list of VMAs */
	struct rb_root mm_rb;

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
