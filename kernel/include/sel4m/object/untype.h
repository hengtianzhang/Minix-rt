#ifndef __SEL4M_OBJECT_H_
#define __SEL4M_OBJECT_H_

#include <base/atomic.h>

#include <asm/pgtable-types.h>
#include <asm/mmu.h>

struct untype_struct {
	unsigned long nr_pages;
	unsigned long nr_used_pages;
};

struct untype_pud {
	struct page		*pud_page;
	atomic_long_t	pud_ref_count;
	struct rb_node pud_node;
};

struct untype_pmd {
	struct page		*pmd_page;
	atomic_long_t	pmd_ref_count;
	struct rb_node pmd_node;
};

extern void untype_core_init(void);
extern void untype_core_init_late(void);

extern int untype_create_pud_map(pgd_t *pgd,
			unsigned long addr, unsigned long end);
extern int untype_remove_pud_map(pgd_t *pgd,
			unsigned long addr, unsigned long end);

#endif /* !__SEL4M_OBJECT_H_ */
