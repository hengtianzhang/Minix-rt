#include <sel4m/gfp.h>
#include <sel4m/slab.h>
#include <sel4m/cpumask.h>
#include <sel4m/object/cap_types.h>
#include <sel4m/object/untype.h>
#include <sel4m/spinlock.h>
#include <sel4m/sched.h>
#include <sel4m/preempt.h>

#include <asm/mmu_context.h>
#include <asm/mmu.h>
#include <asm/current.h>
#include <asm/sections.h>

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

static struct untyp_ref_pte_talbe *
vma_find_pte_table(struct vm_area_struct *vma, struct page *pte_page)
{
	struct rb_node *node = vma->pte_root.rb_node;

	while (node) {
		struct untyp_ref_pte_talbe *data = container_of(node, struct untyp_ref_pte_talbe, node);
	
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
	struct untyp_ref_pte_talbe *ur_pte;
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
		page = phys_to_page(__pte_to_phys(pte));
		pte_clear(NULL, NULL, ptep);
		__free_page(page);
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

static struct untyp_ref_pmd_talbe *
vma_find_pmd_table(struct vm_area_struct *vma, struct page *pmd_page)
{
	struct rb_node *node = vma->pmd_root.rb_node;

	while (node) {
		struct untyp_ref_pmd_talbe *data = container_of(node, struct untyp_ref_pmd_talbe, node);
	
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
	struct untyp_ref_pmd_talbe *ur_pmd;
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

static struct untyp_ref_pud_talbe *
vma_find_pud_table(struct vm_area_struct *vma, struct page *pud_page)
{
	struct rb_node *node = vma->pud_root.rb_node;

	while (node) {
		struct untyp_ref_pud_talbe *data = container_of(node, struct untyp_ref_pud_talbe, node);
	
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
	struct untyp_ref_pud_talbe *ur_pud;
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
	struct page *page;
	unsigned long start;

	start = addr;
	ptep = pte_offset_kernel(pmdp, addr);
	do {
		if (pte_none(*ptep))
			continue;
		pte = ptep_get_and_clear(vma->vm_mm, addr, ptep);
		page = phys_to_page(__pte_to_phys(pte));
		pte_clear(NULL, NULL, ptep);
		__free_page(page);
	} while (ptep++, addr += PAGE_SIZE, addr != end);
}

static void vunmap_bad_pmd_range(struct vm_area_struct *vma, pud_t *pudp,
						unsigned long addr, unsigned long end)
{
	struct untyp_ref_pte_talbe *ur_pte;
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
	struct untyp_ref_pmd_talbe *ur_pmd;
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
	struct untyp_ref_pud_talbe *ur_pud;
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
			struct untyp_ref_pud_talbe *ur_pud)
{
	struct rb_node **new = &(vma->pud_root.rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct untyp_ref_pud_talbe *this = container_of(*new, struct untyp_ref_pud_talbe, node);

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

static int __pud_alloc(struct vm_area_struct *vma, pgd_t *pgdp, unsigned long address)
{
	int ret = 0;
	struct untyp_ref_pud_talbe *ur_pud;

	pud_t *new = (pud_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
	if (!new)
		return -ENOMEM;

	ur_pud = kmalloc(sizeof (*ur_pud), GFP_KERNEL);
	if (!ur_pud) {
		ret = -ENOMEM;
		goto fail_table;
	}

	ur_pud->pud_page = virt_to_page(new);
	if (!vma_insert_pud_table(vma, ur_pud)) {
		ret = -EINVAL;
		goto fail_insert;
	}

	smp_wmb();

	spin_lock(&vma->vm_mm->page_table_lock);
	if (!pgd_present(*pgdp)) {
		__pgd_populate(pgdp, virt_to_phys((void *)new), PUD_TYPE_TABLE);
		mm_inc_nr_puds(vma->vm_mm);
	} else {
		get_page(pfn_to_page(__phys_to_pfn(__pgd_to_phys(*pgdp))));
		free_page((u64)new);
	}

	spin_unlock(&vma->vm_mm->page_table_lock);

	return 0;

fail_insert:
	kfree(ur_pud);
fail_table:
	free_page((u64)new);

	return ret;
}

static inline pud_t *pud_alloc(struct vm_area_struct *vma, pgd_t *pgdp, unsigned long address)
{
	return (unlikely(pgd_none(*pgdp)) && __pud_alloc(vma, pgdp, address)) ?
		NULL : pud_offset(pgdp, address);
}

static bool vma_insert_pmd_table(struct vm_area_struct *vma,
			struct untyp_ref_pmd_talbe *ur_pmd)
{
	struct rb_node **new = &(vma->pmd_root.rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct untyp_ref_pmd_talbe *this = container_of(*new, struct untyp_ref_pmd_talbe, node);

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

static int __pmd_alloc(struct vm_area_struct *vma, pud_t *pudp, unsigned long address)
{
	int ret = 0;
	struct untyp_ref_pmd_talbe *ur_pmd;

	pmd_t *new = (pmd_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
	if (!new)
		return -ENOMEM;

	ur_pmd = kmalloc(sizeof (*ur_pmd), GFP_KERNEL);
	if (!ur_pmd) {
		ret = -ENOMEM;
		goto fail_table;
	}

	ur_pmd->pmd_page = virt_to_page(new);
	if (!vma_insert_pmd_table(vma, ur_pmd)) {
		ret = -EINVAL;
		goto fail_insert;
	}

	smp_wmb();

	spin_lock(&vma->vm_mm->page_table_lock);
	if (!pud_present(*pudp)) {
		__pud_populate(pudp, virt_to_phys((void *)new), PUD_TYPE_TABLE);
		mm_inc_nr_pmds(vma->vm_mm);
	} else {
		get_page(pfn_to_page(__phys_to_pfn(__pud_to_phys(*pudp))));
		free_page((u64)new);
	}

	spin_unlock(&vma->vm_mm->page_table_lock);

	return 0;

fail_insert:
	kfree(ur_pmd);
fail_table:
	free_page((u64)new);

	return 0;
}

static inline pmd_t *pmd_alloc(struct vm_area_struct *vma, pud_t *pudp, unsigned long address)
{
	return (unlikely(pud_none(*pudp)) && __pmd_alloc(vma, pudp, address)) ?
		NULL : pmd_offset(pudp, address);
}

static bool vma_insert_pte_table(struct vm_area_struct *vma,
			struct untyp_ref_pte_talbe *ur_pte)
{
	struct rb_node **new = &(vma->pte_root.rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct untyp_ref_pte_talbe *this = container_of(*new, struct untyp_ref_pte_talbe, node);

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

static int __pte_alloc(struct vm_area_struct *vma, pmd_t *pmdp, unsigned long address)
{
	int ret = 0;
	struct untyp_ref_pte_talbe *ur_pte;

	pte_t *new = (pte_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
	if (!new)
		return -ENOMEM;

	ur_pte = kmalloc(sizeof (*ur_pte), GFP_KERNEL);
	if (!ur_pte) {
		ret = -ENOMEM;
		goto fail_table;
	}

	ur_pte->pte_page = virt_to_page(new);
	if (!vma_insert_pte_table(vma, ur_pte)) {
		ret = -EINVAL;
		goto fail_insert;
	}

	smp_wmb();

	spin_lock(&vma->vm_mm->page_table_lock);
	if (!pmd_present(*pmdp)) {
		__pmd_populate(pmdp, virt_to_phys((void *)new), PMD_TYPE_TABLE);
		mm_inc_nr_ptes(vma->vm_mm);
	} else {
		get_page(pfn_to_page(__phys_to_pfn(__pmd_to_phys(*pmdp))));
		free_page((u64)new);
	}

	spin_unlock(&vma->vm_mm->page_table_lock);

	return 0;

fail_insert:
	kfree(ur_pte);
fail_table:
	free_page((u64)new);

	return ret;
}

static inline pte_t *pte_alloc(struct vm_area_struct *vma, pmd_t *pmdp, unsigned long address)
{
	return (unlikely(pmd_none(*pmdp)) && __pte_alloc(vma, pmdp, address)) ?
		NULL : pte_offset_kernel(pmdp, address);
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
		struct page *page = alloc_page(GFP_USER | GFP_ZERO);

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

struct vm_area_struct *untype_get_vmap_area(unsigned long vstart,
				unsigned long size, unsigned long flags,
				struct mm_struct *mm, phys_addr_t io_space)
{
	struct vm_area_struct *vma;

	BUG_ON(in_interrupt());

	if (!PAGE_ALIGNED(vstart) || !mm ||
			!PAGE_ALIGNED(size) || !size)
		return NULL;

	vma = kmalloc(sizeof (*vma), GFP_KERNEL | GFP_ZERO);
	if (unlikely(!vma))
		return NULL;

	vma->vm_start = vstart;
	vma->vm_end	= vstart + size;

	vma->vm_page_prot = vm_get_page_prot(flags);
	vma->vm_flags = flags;

	vma->nr_pages = size >> PAGE_SHIFT;

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
}

void untype_free_vmap_area(unsigned long addr, struct mm_struct *mm)
{
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

	kfree(vma);
}

struct mm_struct *untype_alloc_mm_struct(void)
{
	struct mm_struct *mm;

	mm = kmalloc(sizeof (*mm), GFP_KERNEL | GFP_ZERO);
	if (!mm)
		return NULL;

	mm->pgd = (pgd_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
	if (!mm->pgd)
		goto err;

	mm->vma_rb_root = RB_ROOT;
	spin_lock_init(&mm->vma_lock);
	mm_pgtables_bytes_init(mm);
	spin_lock_init(&mm->page_table_lock);
	atomic_set(&mm->mm_count, 1);
	init_new_context(NULL, mm);

	return mm;
err:
	kfree(mm);
	return NULL;
}

void untype_free_mm_struct(struct mm_struct *mm)
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

void untype_core_init(void)
{
	free_area_init_nodes();
	kmem_cache_init();
}

static void untype_print_memory_info(void)
{
	int cpu;
	unsigned long percpu_cache_pages = 0;
	unsigned long physpages, codesize, datasize, rosize, bss_size;
	unsigned long init_code_size, init_data_size;

	physpages = total_physpages;
	codesize = _etext - _stext;
	datasize = _edata - _sdata;
	rosize = __end_rodata - __start_rodata;
	bss_size = __bss_stop - __bss_start;
	init_data_size = __init_end - __init_begin;
	init_code_size = _einittext - _sinittext;

#define adj_init_size(start, end, size, pos, adj) \
	do { \
		if (start <= pos && pos < end && size > adj) \
			size -= adj; \
	} while (0)

	adj_init_size(__init_begin, __init_end, init_data_size,
		     _sinittext, init_code_size);
	adj_init_size(_stext, _etext, codesize, _sinittext, init_code_size);
	adj_init_size(_sdata, _edata, datasize, __init_begin, init_data_size);
	adj_init_size(_stext, _etext, codesize, __start_rodata, rosize);
	adj_init_size(_sdata, _edata, datasize, __start_rodata, rosize);

#undef	adj_init_size
	printf("Kernel memory info:\n");
	printf("  Memory: %luK/%luK available (%luK kernel code, %luK rwdata, %luK rodata, %luK init, %luK bss, %luK"
		" reserved)\n",
		nr_managed_pages() << (PAGE_SHIFT - 10),
		physpages << (PAGE_SHIFT - 10),
		codesize >> 10, datasize >> 10, rosize >> 10,
		(init_data_size + init_code_size) >> 10, bss_size >> 10,
		(physpages - nr_managed_pages()) << (PAGE_SHIFT - 10));

	printf("Services memory info:\n");
	printf("  Memory: %luK/%luK used\n",
		(nr_managed_pages() - nr_free_pages()) << (PAGE_SHIFT - 10),
		nr_managed_pages() << (PAGE_SHIFT - 10));

	for_each_possible_cpu(cpu)
		percpu_cache_pages += nr_percpu_cache_pages(cpu);

	printf("  Cpu cache memory: %luK available\n",
		percpu_cache_pages << (PAGE_SHIFT - 10));
}

void untype_core_init_late(void)
{
	free_initmem();

	untype_print_memory_info();
}
