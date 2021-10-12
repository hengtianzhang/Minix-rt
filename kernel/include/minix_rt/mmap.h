#ifndef __MINIX_RT_MMAP_H_
#define __MINIX_RT_MMAP_H_

#include <base/atomic.h>
#include <base/rbtree.h>

#include <uapi/minix_rt/mmap.h>

#include <asm/pgtable-types.h>
#include <asm/mmu.h>
#include <asm/current.h>

extern pgprot_t vm_get_page_prot(unsigned long vm_flags);

struct mm_struct;
extern struct vm_area_struct *mmap_get_vmap_area(unsigned long vstart,
				unsigned long size, unsigned long flags,
				struct mm_struct *mm, phys_addr_t io_space);
extern void mmap_free_vmap_area(unsigned long addr, struct mm_struct *mm);

extern int vmap_page_range(struct vm_area_struct *vma);
extern void vumap_page_range(struct vm_area_struct *vma);

extern struct mm_struct *mmap_alloc_mm_struct(void);
extern void mmap_free_mm_struct(struct mm_struct *mm);

extern void mmap_destroy_mm(struct task_struct *tsk);

struct mmap_ref_pud_talbe {
	struct page *pud_page;
	struct rb_node	node;
};

struct mmap_ref_pmd_talbe {
	struct page *pmd_page;
	struct rb_node	node;
};

struct mmap_ref_pte_talbe {
	struct page *pte_page;
	struct rb_node	node;
};

extern struct vm_area_struct *mmap_first_vma(struct task_struct *tsk);
extern struct vm_area_struct *mmap_next_vma(struct vm_area_struct *vma);

#define for_each_vm_area(vma, tsk)	\
	for (vma = mmap_first_vma(tsk); vma != NULL; vma = mmap_next_vma(vma))

extern int untype_copy_mm(struct task_struct *tsk, struct task_struct *orgi_tsk,
							unsigned long *stack_top, unsigned long *ipcptr);

#endif /* !__MINIX_RT_MMAP_H_ */
