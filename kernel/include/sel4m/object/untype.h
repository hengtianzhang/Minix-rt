#ifndef __SEL4M_OBJECT_H_
#define __SEL4M_OBJECT_H_

#include <base/atomic.h>

#include <asm/pgtable-types.h>
#include <asm/mmu.h>

/*
 * vm_flags in vm_area_struct, see mm_types.h.
 * When changing, update also include/trace/events/mmflags.h
 */
#define VM_NONE		0x00000000

#define VM_READ		0x00000001	/* currently active flags */
#define VM_WRITE	0x00000002
#define VM_EXEC		0x00000004
#define VM_SHARED	0x00000008

#define VM_IOREMAP	0x00000010

extern pgprot_t vm_get_page_prot(unsigned long vm_flags);

struct untype_struct {
	unsigned long nr_pages;
	unsigned long nr_used_pages;
};

extern void untype_core_init(void);
extern void untype_core_init_late(void);

struct mm_struct;
extern struct vm_area_struct *untype_get_vmap_area(unsigned long vstart,
				unsigned long size, unsigned long flags,
				struct mm_struct *mm, phys_addr_t io_space);
extern void untype_free_vmap_area(unsigned long addr, struct mm_struct *mm);

extern int vmap_page_range(struct vm_area_struct *vma);
extern void vumap_page_range(struct vm_area_struct *vma);

extern struct mm_struct *untype_alloc_mm_struct(void);
extern void untype_free_mm_struct(struct mm_struct *mm);

struct untyp_ref_pud_talbe {
	struct page *pud_page;
	struct rb_node	node;
};

struct untyp_ref_pmd_talbe {
	struct page *pmd_page;
	struct rb_node	node;
};

struct untyp_ref_pte_talbe {
	struct page *pte_page;
	struct rb_node	node;
};

#endif /* !__SEL4M_OBJECT_H_ */
