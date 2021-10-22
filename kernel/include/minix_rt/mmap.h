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
				struct mm_struct *mm, phys_addr_t phys_addr);
extern void mmap_free_vmap_area(unsigned long addr, struct mm_struct *mm);

extern struct vm_area_struct *
mmap_find_vma_area(unsigned long addr, struct mm_struct *mm);
extern int vmap_page_range(struct vm_area_struct *vma);
extern void vumap_page_range(struct vm_area_struct *vma);

extern struct mm_struct *mmap_alloc_mm_struct(void);
extern void mmap_free_mm_struct(struct mm_struct *mm);

extern void mmap_destroy_mm(struct mm_struct *mm);

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

extern struct vm_area_struct *mmap_first_vma(struct mm_struct *mm);
extern struct vm_area_struct *mmap_next_vma(struct vm_area_struct *vma);

extern int mmap_memcpy_from_vma(void *dest, unsigned long addr, unsigned long size,
			struct task_struct *tsk);
extern int mmap_memcpy_to_vma(unsigned long addr, unsigned long size, void *src, 
			struct task_struct *tsk);

extern int mmap_strnlen_vma(const char *s, struct task_struct *tsk);

extern int mmap_strcpy_from_vma(void *dest, unsigned long addr, struct task_struct *tsk);
extern int mmap_strcpy_to_vma(unsigned long addr, void *src, struct task_struct *tsk);

#define for_each_vm_area(vma, mm)	\
	for (vma = mmap_first_vma(mm); vma != NULL; vma = mmap_next_vma(vma))

#define for_each_vm_area_safe(vma, n, mm)	\
	for (vma = mmap_first_vma(mm), n = mmap_next_vma(vma); vma != NULL;	\
		vma = n, n = mmap_next_vma(vma))

#define for_each_curr_vm_area(vma, curr_vma)	\
	for (vma = curr_vma; vma != NULL; vma = mmap_next_vma(vma))

#define for_each_curr_vm_area_safe(vma, n, curr_vma)	\
	for (vma = curr_vma, n = mmap_next_vma(vma); vma != NULL; \
		vma = n, n = mmap_next_vma(vma))

#define for_each_next_vm_area(vma, curr_vma)	\
	for (vma = mmap_next_vma(curr_vma); vma != NULL; vma = mmap_next_vma(vma))

#define for_each_next_vm_area_safe(vma, n, curr_vma)	\
	for (vma = mmap_next_vma(curr_vma), n = mmap_next_vma(vma); vma != NULL; \
		vma = n, n = mmap_next_vma(vma))

extern int mmap_copy_mm(struct task_struct *tsk, struct task_struct *orgi_tsk,
							unsigned long *stack_top);

#endif /* !__MINIX_RT_MMAP_H_ */
