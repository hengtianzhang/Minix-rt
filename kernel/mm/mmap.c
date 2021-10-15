#include <minix_rt/gfp.h>
#include <minix_rt/slab.h>
#include <minix_rt/cpumask.h>
#include <minix_rt/mmap.h>
#include <minix_rt/spinlock.h>
#include <minix_rt/sched.h>
#include <minix_rt/preempt.h>
#include <minix_rt/syscalls.h>
#include <minix_rt/uaccess.h>

#include <uapi/minix_rt/mmap.h>

#include <asm/mmu_context.h>
#include <asm/mmu.h>
#include <asm/current.h>
#include <asm/sections.h>
#include <asm/processor.h>

/* description of effects of mapping type and prot in current implementation.
 * this is due to the limited x86 page protection hardware.  The expected
 * behavior is in parens:
 *
 * map_type	prot
 *		PROT_NONE	PROT_READ	PROT_WRITE	PROT_EXEC
 * MAP_SHARED	r: (no) no	r: (yes) yes	r: (no) yes	r: (no) yes
 *		w: (no) no	w: (no) no	w: (yes) yes	w: (no) no
 *		x: (no) no	x: (no) yes	x: (no) yes	x: (yes) yes
 *
 * MAP_PRIVATE	r: (no) no	r: (yes) yes	r: (no) yes	r: (no) yes
 *		w: (no) no	w: (no) no	w: (copy) copy	w: (no) no
 *		x: (no) no	x: (no) yes	x: (no) yes	x: (yes) yes
 *
 * On arm64, PROT_EXEC has the following behaviour for both MAP_SHARED and
 * MAP_PRIVATE:
 *								r: (no) no
 *								w: (no) no
 *								x: (yes) yes
 */
static pgprot_t protection_map[16] __ro_after_init = {
	__P000, __P001, __P010, __P011, __P100, __P101, __P110, __P111,
	__S000, __S001, __S010, __S011, __S100, __S101, __S110, __S111
};

pgprot_t vm_get_page_prot(unsigned long vm_flags)
{
	return __pgprot(pgprot_val(protection_map[vm_flags &
				(VM_READ|VM_WRITE|VM_EXEC|VM_SHARED)]));
}

static struct mmap_ref_pte_talbe *
vma_find_pte_table(struct vm_area_struct *vma, struct page *pte_page)
{
	struct rb_node *node = vma->pte_root.rb_node;

	while (node) {
		struct mmap_ref_pte_talbe *data = container_of(node, struct mmap_ref_pte_talbe, node);
	
		if (pte_page < data->pte_page)
			node = node->rb_left;
		else if (pte_page > data->pte_page)
			node = node->rb_right;
		else
			return data;
	}

	return NULL;
}

static void vunmap_pte_range(struct vm_area_struct *vma, pmd_t *pmdp,
						unsigned long addr, unsigned long end)
{
	struct mmap_ref_pte_talbe *ur_pte;
	pte_t *ptep;
	pte_t pte;
	struct page *page;
	unsigned long start;

	start = addr;
	ptep = pte_offset_kernel(pmdp, addr);
	do {
		if (pte_none(*ptep))
			continue;
		pte = ptep_get_and_clear(vma->vm_mm, addr, ptep);
		WARN_ON(!pte_none(pte) && !pte_present(pte));
	} while (ptep++, addr += PAGE_SIZE, addr != end);

	page = pfn_to_page(__phys_to_pfn(__pmd_to_phys(*pmdp)));
	ur_pte = vma_find_pte_table(vma, page);
	BUG_ON(!ur_pte);
	rb_erase(&ur_pte->node, &vma->pte_root);
	kfree(ur_pte);

	if (put_pagetable_testzero(page)) {
		pmd_clear(NULL, NULL, pmdp);
		__put_page(page);
		mm_dec_nr_ptes(vma->vm_mm);
	}
}

static struct mmap_ref_pmd_talbe *
vma_find_pmd_table(struct vm_area_struct *vma, struct page *pmd_page)
{
	struct rb_node *node = vma->pmd_root.rb_node;

	while (node) {
		struct mmap_ref_pmd_talbe *data = container_of(node, struct mmap_ref_pmd_talbe, node);
	
		if (pmd_page < data->pmd_page)
			node = node->rb_left;
		else if (pmd_page > data->pmd_page)
			node = node->rb_right;
		else
			return data;
	}

	return NULL;
}

static void vunmap_pmd_range(struct vm_area_struct *vma, pud_t *pudp,
						unsigned long addr, unsigned long end)
{
	struct mmap_ref_pmd_talbe *ur_pmd;
	struct page *pmd_page;
	pmd_t *pmdp;
	unsigned long next;
	unsigned long start;

	start = addr;
	pmdp = pmd_offset(pudp, addr);
	do {
		next = pmd_addr_end(addr, end);
		if (pmd_clear_huge(pmdp))
			continue;
		if (pmd_none_or_clear_bad(pmdp))
			continue;
		vunmap_pte_range(vma, pmdp, addr, next);
	} while (pmdp++, addr = next, addr != end);

	pmdp = pmd_offset(pudp, start);
	pmd_page = virt_to_page(pmdp);
	ur_pmd = vma_find_pmd_table(vma, pmd_page);
	BUG_ON(!ur_pmd);
	rb_erase(&ur_pmd->node, &vma->pmd_root);
	kfree(ur_pmd);

	if (put_pagetable_testzero(pmd_page)) {
		pud_clear(NULL, NULL, pudp);
		__put_page(pmd_page);
		mm_dec_nr_pmds(vma->vm_mm);
	}
}

static struct mmap_ref_pud_talbe *
vma_find_pud_table(struct vm_area_struct *vma, struct page *pud_page)
{
	struct rb_node *node = vma->pud_root.rb_node;

	while (node) {
		struct mmap_ref_pud_talbe *data = container_of(node, struct mmap_ref_pud_talbe, node);
	
		if (pud_page < data->pud_page)
			node = node->rb_left;
		else if (pud_page > data->pud_page)
			node = node->rb_right;
		else
			return data;
	}

	return NULL;
}

static void vumap_pud_range(struct vm_area_struct *vma, pgd_t *pgdp,
						unsigned long addr, unsigned long end)
{
	struct mmap_ref_pud_talbe *ur_pud;
	struct page *pud_page;
	pud_t *pudp;
	unsigned long next;
	unsigned long start;

	start = addr;
	pudp = pud_offset(pgdp, addr);
	do {
		next = pud_addr_end(addr, end);
		if (pud_clear_huge(pudp))
			continue;
		if (pud_none_or_clear_bad(pudp))
			continue;
		vunmap_pmd_range(vma, pudp, addr, next);
	} while (pudp++, addr = next, addr != end);

	pudp = pud_offset(pgdp, start);
	pud_page = virt_to_page(pudp);
	ur_pud = vma_find_pud_table(vma, pud_page);
	BUG_ON(!ur_pud);
	rb_erase(&ur_pud->node, &vma->pud_root);
	kfree(ur_pud);

	if (put_pagetable_testzero(pud_page)) {
		pgd_clear(NULL, NULL, pgdp);
		__put_page(pud_page);
		mm_dec_nr_puds(vma->vm_mm);
	}
}

void vumap_page_range(struct vm_area_struct *vma)
{
	pgd_t *pgdp;
	unsigned long next;
	unsigned long addr;
	unsigned long end;

	if (!vma) {
		WARN_ON(1);
		return;
	}

	addr = vma->vm_start;
	end = vma->vm_end;
	if (addr >= end) {
		WARN_ON(1);
		return;
	}

	pgdp = pgd_offset(vma->vm_mm->pgd, addr);
	do {
		next = pgd_addr_end(addr, vma->vm_end);
		if (pgd_none_or_clear_bad(pgdp))
			continue;
		vumap_pud_range(vma, pgdp, addr, next);
	} while (pgdp++, addr = next, addr != end);
}

static void vunmap_bad_pte_range(struct vm_area_struct *vma, pmd_t *pmdp,
						unsigned long addr, unsigned long end)
{
	pte_t *ptep;
	pte_t pte;

	ptep = pte_offset_kernel(pmdp, addr);
	do {
		if (pte_none(*ptep))
			continue;
		pte = ptep_get_and_clear(vma->vm_mm, addr, ptep);
		WARN_ON(!pte_none(pte) && !pte_present(pte));
	} while (ptep++, addr += PAGE_SIZE, addr != end);
}

static void vunmap_bad_pmd_range(struct vm_area_struct *vma, pud_t *pudp,
						unsigned long addr, unsigned long end)
{
	struct mmap_ref_pte_talbe *ur_pte;
	struct page *pte_page;
	pmd_t *pmdp;
	unsigned long next;
	unsigned long start;

	start = addr;
	pmdp = pmd_offset(pudp, addr);
	do {
		next = pmd_addr_end(addr, end);
		if (pmd_clear_huge(pmdp))
			continue;
		if (pmd_none_or_clear_bad(pmdp))
			continue;
		vunmap_bad_pte_range(vma, pmdp, addr, next);
		pte_page = pfn_to_page(__phys_to_pfn(__pmd_to_phys(*pmdp)));
		ur_pte = vma_find_pte_table(vma, pte_page);
		if (ur_pte) {
			if (put_pagetable_testzero(pte_page)) {
				pmd_clear(NULL, NULL, pmdp);
				__put_page(pte_page);
				mm_dec_nr_ptes(vma->vm_mm);
			}
			rb_erase(&ur_pte->node, &vma->pte_root);
			kfree(ur_pte);
		}
	} while (pmdp++, addr = next, addr != end);
}

static void vumap_bad_pud_range(struct vm_area_struct *vma, pgd_t *pgdp,
						unsigned long addr, unsigned long end)
{
	struct mmap_ref_pmd_talbe *ur_pmd;
	struct page *pmd_page;
	pud_t *pudp;
	unsigned long next;
	unsigned long start;

	start = addr;
	pudp = pud_offset(pgdp, addr);
	do {
		next = pud_addr_end(addr, end);
		if (pud_clear_huge(pudp))
			continue;
		if (pud_none_or_clear_bad(pudp))
			continue;
		vunmap_bad_pmd_range(vma, pudp, addr, next);
		pmd_page = pfn_to_page(__phys_to_pfn(__pud_to_phys(*pudp)));
		ur_pmd = vma_find_pmd_table(vma, pmd_page);
		if (ur_pmd) {
			if (put_pagetable_testzero(pmd_page)) {
				pud_clear(NULL, NULL, pudp);
				__put_page(pmd_page);
				mm_dec_nr_pmds(vma->vm_mm);
			}
			rb_erase(&ur_pmd->node, &vma->pmd_root);
			kfree(ur_pmd);
		}
	} while (pudp++, addr = next, addr != end);
}

static void vmap_free_bad_area(struct vm_area_struct *vma)
{
	struct mmap_ref_pud_talbe *ur_pud;
	struct page *page;
	pgd_t *pgdp;
	unsigned long next;
	unsigned long addr;
	unsigned long end;

	if (!vma) {
		WARN_ON(1);
		return;
	}

	addr = vma->vm_start;
	end = vma->vm_end;
	if (addr >= end) {
		WARN_ON(1);
		return;
	}

	pgdp = pgd_offset(vma->vm_mm->pgd, addr);
	do {
		next = pgd_addr_end(addr, vma->vm_end);
		if (pgd_none_or_clear_bad(pgdp))
			continue;
		vumap_bad_pud_range(vma, pgdp, addr, next);
		page = pfn_to_page(__phys_to_pfn(__pgd_to_phys(*pgdp)));
		ur_pud = vma_find_pud_table(vma, page);
		if (ur_pud) {
			if (put_pagetable_testzero(page)) {
				pgd_clear(NULL, NULL, pgdp);
				__put_page(page);
				mm_dec_nr_puds(vma->vm_mm);
			}
			rb_erase(&ur_pud->node, &vma->pud_root);
			kfree(ur_pud);
		}
	} while (pgdp++, addr = next, addr != end);
}

static bool vma_insert_pud_table(struct vm_area_struct *vma,
			struct mmap_ref_pud_talbe *ur_pud)
{
	struct rb_node **new = &(vma->pud_root.rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct mmap_ref_pud_talbe *this = container_of(*new, struct mmap_ref_pud_talbe, node);

		parent = *new;
		if (ur_pud->pud_page < this->pud_page)
			new = &((*new)->rb_left);
		else if (ur_pud->pud_page > this->pud_page)
			new = &((*new)->rb_right);
		else
			return false;
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&ur_pud->node, parent, new);
	rb_insert_color(&ur_pud->node, &vma->pud_root);

	return true;
}

static inline pud_t *pud_alloc(struct vm_area_struct *vma, pgd_t *pgdp, unsigned long address)
{
	struct mmap_ref_pud_talbe *ur_pud;
	pud_t *new ;
	struct page *page;

	ur_pud = kmalloc(sizeof (*ur_pud), GFP_KERNEL);
	if (!ur_pud)
		goto fail_table;

	if (!unlikely(pgd_none(*pgdp))) {
		page = pfn_to_page(__phys_to_pfn(__pgd_to_phys(*pgdp)));
		get_page(page);
		ur_pud->pud_page = page;
		if (!vma_insert_pud_table(vma, ur_pud)) {
			put_page(page);
			goto fail_get_page;
		}
	} else {
		new = (pud_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
		if (!new)
			goto fail_get_page;

		ur_pud->pud_page = virt_to_page(new);
		if (!vma_insert_pud_table(vma, ur_pud)) {
			free_page((u64)new);
			goto fail_get_page;
		}
		spin_lock(&vma->vm_mm->page_table_lock);
		__pgd_populate(pgdp, virt_to_phys((void *)new), PUD_TYPE_TABLE);
		mm_inc_nr_puds(vma->vm_mm);
		spin_unlock(&vma->vm_mm->page_table_lock);
	}

	smp_wmb();

	return pud_offset(pgdp, address);

fail_get_page:
	kfree(ur_pud);
fail_table:
	return NULL;
}

static bool vma_insert_pmd_table(struct vm_area_struct *vma,
			struct mmap_ref_pmd_talbe *ur_pmd)
{
	struct rb_node **new = &(vma->pmd_root.rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct mmap_ref_pmd_talbe *this = container_of(*new, struct mmap_ref_pmd_talbe, node);

		parent = *new;
		if (ur_pmd->pmd_page < this->pmd_page)
			new = &((*new)->rb_left);
		else if (ur_pmd->pmd_page > this->pmd_page)
			new = &((*new)->rb_right);
		else
			return false;
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&ur_pmd->node, parent, new);
	rb_insert_color(&ur_pmd->node, &vma->pmd_root);

	return true;
}

static inline pmd_t *pmd_alloc(struct vm_area_struct *vma, pud_t *pudp, unsigned long address)
{
	struct mmap_ref_pmd_talbe *ur_pmd;
	pmd_t *new;
	struct page *page;

	ur_pmd = kmalloc(sizeof (*ur_pmd), GFP_KERNEL);
	if (!ur_pmd)
		goto fail_table;

	if (!unlikely(pud_none(*pudp))) {
		page = pfn_to_page(__phys_to_pfn(__pud_to_phys(*pudp)));
		get_page(page);
		ur_pmd->pmd_page = page;
		if (!vma_insert_pmd_table(vma, ur_pmd)) {
			put_page(page);
			goto fail_get_page;
		}
	} else {
		new = (pmd_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
		if (!new)
			goto fail_get_page;

		ur_pmd->pmd_page = virt_to_page(new);
		if (!vma_insert_pmd_table(vma, ur_pmd)) {
			free_page((u64)new);
			goto fail_get_page;
		}
		spin_lock(&vma->vm_mm->page_table_lock);
		__pud_populate(pudp, virt_to_phys((void *)new), PUD_TYPE_TABLE);
		mm_inc_nr_pmds(vma->vm_mm);
		spin_unlock(&vma->vm_mm->page_table_lock);
	}

	smp_wmb();

	return pmd_offset(pudp, address);

fail_get_page:
	kfree(ur_pmd);
fail_table:
	return NULL;
}

static bool vma_insert_pte_table(struct vm_area_struct *vma,
			struct mmap_ref_pte_talbe *ur_pte)
{
	struct rb_node **new = &(vma->pte_root.rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct mmap_ref_pte_talbe *this = container_of(*new, struct mmap_ref_pte_talbe, node);

		parent = *new;
		if (ur_pte->pte_page < this->pte_page)
			new = &((*new)->rb_left);
		else if (ur_pte->pte_page > this->pte_page)
			new = &((*new)->rb_right);
		else
			return false;
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&ur_pte->node, parent, new);
	rb_insert_color(&ur_pte->node, &vma->pte_root);

	return true;
}

static inline pte_t *pte_alloc(struct vm_area_struct *vma, pmd_t *pmdp, unsigned long address)
{
	struct mmap_ref_pte_talbe *ur_pte;
	pte_t *new;
	struct page *page;

	ur_pte = kmalloc(sizeof (*ur_pte), GFP_KERNEL);
	if (!ur_pte)
		goto fail_table;

	if (!unlikely(pmd_none(*pmdp))) {
		page = pfn_to_page(__phys_to_pfn(__pmd_to_phys(*pmdp)));
		get_page(page);
		ur_pte->pte_page = page;
		if (!vma_insert_pte_table(vma, ur_pte)) {
			put_page(page);
			goto fail_get_page;
		}
	} else {
		new = (pte_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
		if (!new)
			goto fail_get_page;

		ur_pte->pte_page = virt_to_page(new);
		if (!vma_insert_pte_table(vma, ur_pte)) {
			free_page((u64)new);
			goto fail_get_page;
		}
		spin_lock(&vma->vm_mm->page_table_lock);
		__pmd_populate(pmdp, virt_to_phys((void *)new), PMD_TYPE_TABLE);
		mm_inc_nr_ptes(vma->vm_mm);
		spin_unlock(&vma->vm_mm->page_table_lock);
	}

	smp_wmb();

	return pte_offset_kernel(pmdp, address);

fail_get_page:
	kfree(ur_pte);
fail_table:
	return NULL;
}

static int vmap_pte_range(struct vm_area_struct *vma, pmd_t *pmdp,
					unsigned long addr, unsigned long end, int *nr)
{
	pte_t *pte;

	/*
	 * nr is a running index into the array which helps higher level
	 * callers keep track of where we're up to.
	 */
	pte = pte_alloc(vma, pmdp, addr);
	if (!pte)
		return -ENOMEM;
	do {
		struct page *page = vma->pages[*nr];

		if (WARN_ON(!pte_none(*pte)))
			return -EBUSY;
		if (WARN_ON(!page))
			return -ENOMEM;

		set_pte(pte, mk_pte(page, vma->vm_page_prot));
		(*nr)++;
	} while (pte++, addr += PAGE_SIZE, addr != end);

	return 0;
}

static int vmap_pmd_range(struct vm_area_struct *vma, pud_t *pudp,
					unsigned long addr, unsigned long end, int *nr)
{
	pmd_t *pmd;
	unsigned long next;

	pmd = pmd_alloc(vma, pudp, addr);
	if (!pmd)
		return -ENOMEM;
	do {
		next = pmd_addr_end(addr, end);
		if (vmap_pte_range(vma, pmd, addr, next, nr))
			return -ENOMEM;
	} while (pmd++, addr = next, addr != end);

	return 0;
}

static int vmap_pud_range(struct vm_area_struct *vma, pgd_t *pgdp,
					unsigned long addr, unsigned long end, int *nr)
{
	pud_t *pud;
	unsigned long next;

	pud = pud_alloc(vma, pgdp, addr);
	if (!pud)
		return -ENOMEM;
	do {
		next = pud_addr_end(addr, end);
		if (vmap_pmd_range(vma, pud, addr, next, nr))
			return -ENOMEM;
	} while (pud++, addr = next, addr != end);

	return 0;
}

int vmap_page_range(struct vm_area_struct *vma)
{
	pgd_t *pgdp;
	unsigned long next;
	unsigned long addr;
	int err = 0;
	int nr = 0;

	if (!vma)
		return -EINVAL;

	addr = vma->vm_start;
	if (addr >= vma->vm_end) {
		WARN_ON(1);
		return -EINVAL;
	}

	pgdp = pgd_offset(vma->vm_mm->pgd, addr);
	do {
		next = pgd_addr_end(addr, vma->vm_end);
		err = vmap_pud_range(vma, pgdp, addr, next, &nr);
		if (err)
			goto err;
	} while (pgdp++, addr = next, addr != vma->vm_end);

	if (nr != vma->nr_pages)
		goto err;

	return nr;
err:
	vmap_free_bad_area(vma);
	return err;
}

static bool __insert_vma_area(struct vm_area_struct *vma)
{
	struct rb_node **new = &(vma->vm_mm->vma_rb_root.rb_node), *parent = NULL;

	while (*new) {
		struct vm_area_struct *this = container_of(*new, struct vm_area_struct, vm_rb_node);

		parent = *new;
		if (vma->vm_end <= this->vm_start)
			new = &((*new)->rb_left);
		else if (vma->vm_start >= this->vm_end)
			new = &((*new)->rb_right);
		else
			return false;
	}

	/* Add new vm_rb_node and rebalance tree. */
	rb_link_node(&vma->vm_rb_node, parent, new);
	rb_insert_color(&vma->vm_rb_node, &vma->vm_mm->vma_rb_root);

	return true;
}

static struct vm_area_struct *__find_vma_area(unsigned long addr,
					struct mm_struct *mm)
{
	struct rb_node *node;

	if (!mm)
		return NULL;

	node = mm->vma_rb_root.rb_node;

	while (node) {
		struct vm_area_struct *vma = container_of(node, struct vm_area_struct, vm_rb_node);

		if (addr < vma->vm_start)
			node = node->rb_left;
		else if (addr >= vma->vm_end)
			node = node->rb_right;
		else
			return vma;
	}

	return NULL;
}

struct vm_area_struct *mmap_get_vmap_area(unsigned long vstart,
				unsigned long size, unsigned long flags,
				struct mm_struct *mm, phys_addr_t io_space)
{
	struct vm_area_struct *vma;
	unsigned long array_size, i;

	BUG_ON(in_interrupt());

	if (!PAGE_ALIGNED(vstart) || !mm ||
			!PAGE_ALIGNED(size) || !size || !vstart)
		return NULL;

	vma = kmalloc(sizeof (*vma), GFP_KERNEL | GFP_ZERO);
	if (unlikely(!vma))
		return NULL;

	vma->vm_start = vstart;
	vma->vm_end	= vstart + size;

	vma->vm_page_prot = vm_get_page_prot(flags | VM_SHARED);
	vma->vm_flags = flags;

	vma->nr_pages = size >> PAGE_SHIFT;
	array_size = (vma->nr_pages * sizeof (struct page *));
	vma->pages = kmalloc(array_size, GFP_KERNEL | GFP_ZERO);
	if (!vma->pages)
		goto fail_pages;

	if (!(flags & VM_PRIVATE_SHARE)) {
		for (i = 0; i < vma->nr_pages; i++) {
			struct page *page;

			page = alloc_page(GFP_USER | GFP_ZERO);
			if (unlikely(!page)) {
				vma->nr_pages = i;
				goto fail_page;
			}
			vma->pages[i] = page;
		}
	}

	vma->pud_root = RB_ROOT;
	vma->pmd_root = RB_ROOT;
	vma->pte_root = RB_ROOT;

	vma->vm_mm = mm;
	spin_lock(&mm->vma_lock);
	if (!__insert_vma_area(vma)) {
		kfree(vma);
		spin_unlock(&mm->vma_lock);
		return NULL;
	}
	spin_unlock(&mm->vma_lock);

	return vma;

fail_page:
	for (i = 0; i < vma->nr_pages; i++) {
		struct page *page = vma->pages[i];

		BUG_ON(!page);
		__free_page(page);
	}

fail_pages:
	kfree(vma);
	return NULL;
}

struct vm_area_struct *
mmap_find_vma_area(unsigned long addr, struct mm_struct *mm)
{
	struct vm_area_struct *vma;

	if (!mm) {
		return NULL;
	}

	spin_lock(&mm->vma_lock);
	vma = __find_vma_area(addr, mm);
	spin_unlock(&mm->vma_lock);

	return vma;
}

void mmap_free_vmap_area(unsigned long addr, struct mm_struct *mm)
{
	unsigned long i;
	struct vm_area_struct *vma;

	if (!mm) {
		WARN_ON(1);
		return;
	}

	spin_lock(&mm->vma_lock);
	vma = __find_vma_area(addr, mm);
	if (!vma) {
		WARN_ON(1);
		spin_unlock(&mm->vma_lock);
		return;
	}
	BUG_ON(rb_first(&vma->pud_root));
	BUG_ON(rb_first(&vma->pmd_root));
	BUG_ON(rb_first(&vma->pte_root));
	rb_erase(&vma->vm_rb_node, &mm->vma_rb_root);
	spin_unlock(&mm->vma_lock);

	for (i = 0; i < vma->nr_pages; i++) {
		struct page *page = vma->pages[i];

		BUG_ON(unlikely(!page));
		put_page(page);
	}

	kfree(vma->pages);
	kfree(vma);
}

struct mm_struct *mmap_alloc_mm_struct(void)
{
	struct mm_struct *mm;

	mm = kmalloc(sizeof (*mm), GFP_KERNEL | GFP_ZERO);
	if (!mm)
		return NULL;

	mm->pgd = (pgd_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
	if (!mm->pgd)
		goto fail_pgd;

	mm->mmap_base = USER_DS;
	mm->mmap_end = 0;
	mm->task_size = TASK_SIZE;
	mm->vma_rb_root = RB_ROOT;
	spin_lock_init(&mm->vma_lock);
	mm_pgtables_bytes_init(mm);
	spin_lock_init(&mm->page_table_lock);
	atomic_set(&mm->mm_count, 1);
	init_new_context(NULL, mm);

	return mm;

fail_pgd:
	kfree(mm);

	return NULL;
}

struct vm_area_struct *mmap_first_vma(struct mm_struct *mm)
{
	struct rb_node *node;
	struct vm_area_struct *vma;

	node = rb_first(&mm->vma_rb_root);
	if (!node)
		return NULL;

	vma = rb_entry(node, struct vm_area_struct, vm_rb_node);

	return vma;
}

struct vm_area_struct *mmap_next_vma(struct vm_area_struct *vma)
{
	struct rb_node *node;
	struct vm_area_struct *next;

	if (!vma)
		return NULL;

	node = rb_next(&vma->vm_rb_node);
	if (!node)
		return NULL;

	next = rb_entry(node, struct vm_area_struct, vm_rb_node);

	return next;
}

void mmap_free_mm_struct(struct mm_struct *mm)
{
	if (!mm) {
		WARN_ON(1);
		return;
	}

	BUG_ON(mm_pgtables_bytes(mm));
	BUG_ON(!mm->pgd);
	BUG_ON(rb_first(&mm->vma_rb_root));

	BUG_ON(!atomic_dec_and_test(&mm->mm_count));

	free_page((u64)mm->pgd);
	kfree(mm);
}

int mmap_copy_mm(struct task_struct *tsk, struct task_struct *orgi_tsk,
			unsigned long *stack_top)
{
	int i = 0, j, ret;
	unsigned long vstart, size, flags;
	struct vm_area_struct *vma, *new_vma, *tmp_vma;

	for_each_vm_area(vma, orgi_tsk->mm) {
		vstart = vma->vm_start;
		size = vma->vm_end - vma->vm_start;
		flags = vma->vm_flags;

		if (!(vma->vm_flags & (VM_USER_STACK)))
			flags |= VM_PRIVATE_SHARE;

		new_vma = mmap_get_vmap_area(vstart, size, flags, tsk->mm, 0);
		if (!new_vma)
			goto fail_out;

		BUG_ON(new_vma->nr_pages != vma->nr_pages);
		ret = vmap_page_range(new_vma);
		if (ret <= 0)
			goto fail_vma;

		if (new_vma->vm_flags & VM_USER_STACK)
			if (stack_top)
				*stack_top = new_vma->vm_end;
		i++;
	}

	return 0;

fail_vma:
	mmap_free_vmap_area(vstart, tsk->mm);
fail_out:
	j = 0;
	for_each_vm_area_safe(vma, tmp_vma, tsk->mm) {
		vumap_page_range(vma);
		mmap_free_vmap_area(vma->vm_start, tsk->mm);
		j++;
	}
	BUG_ON(j != i);

	return -ENOMEM;
}

void mmap_destroy_mm(struct mm_struct *mm)
{
	struct vm_area_struct *vma, *tmp_vma;

	if (!mm)
		return;

	for_each_vm_area_safe(vma, tmp_vma, mm) {
		vumap_page_range(vma);
		mmap_free_vmap_area(vma->vm_start, mm);
	}
}
