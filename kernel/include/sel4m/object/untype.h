#ifndef __SEL4M_OBJECT_H_
#define __SEL4M_OBJECT_H_

#include <base/atomic.h>
#include <base/rbtree.h>

#include <uapi/sel4m/object/untype.h>

#include <asm/pgtable-types.h>
#include <asm/mmu.h>
#include <asm/current.h>

extern pgprot_t vm_get_page_prot(unsigned long vm_flags);

struct untype_struct {
	unsigned long nr_pages;
	unsigned long nr_used_pages;
};

extern void untype_core_init(void);
extern void untype_core_init_late(struct task_struct *tsk);

struct mm_struct;
extern struct vm_area_struct *untype_get_vmap_area(unsigned long vstart,
				unsigned long size, unsigned long flags,
				struct mm_struct *mm, phys_addr_t io_space);
extern void untype_free_vmap_area(unsigned long addr, struct mm_struct *mm);

extern int vmap_page_range(struct vm_area_struct *vma);
extern void vumap_page_range(struct vm_area_struct *vma);

extern struct mm_struct *untype_alloc_mm_struct(void);
extern void untype_free_mm_struct(struct mm_struct *mm);

extern void untype_destroy_mm(struct task_struct *tsk);

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

extern struct vm_area_struct *untype_first_vma(struct task_struct *tsk);
extern struct vm_area_struct *untype_next_vma(struct vm_area_struct *vma);

#define for_each_vm_area(vma, tsk)	\
	for (vma = untype_first_vma(tsk); vma != NULL; vma = untype_next_vma(vma))

extern int untype_copy_mm(struct task_struct *tsk, struct task_struct *orgi_tsk,
							unsigned long *stack_top, unsigned long *ipcptr);

#endif /* !__SEL4M_OBJECT_H_ */
