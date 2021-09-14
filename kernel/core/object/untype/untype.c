#include <sel4m/gfp.h>
#include <sel4m/slab.h>
#include <sel4m/cpumask.h>
#include <sel4m/object/cap_types.h>
#include <sel4m/object/untype.h>
#include <sel4m/spinlock.h>
#include <sel4m/sched.h>
#include <sel4m/preempt.h>

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

static void vunmap_pte_range(struct vm_area_struct *vma, pmd_t *pmdp,
						unsigned long addr, unsigned long end)
{
	pte_t *ptep;
	pte_t pte;
	struct page *page;
	unsigned long start;
	int nr = 0;

	start = addr;
	ptep = pte_offset_kernel(pmdp, addr);
	do {
		if (pte_none(*ptep))
			continue;
		pte = ptep_get_and_clear(vma->vm_mm, addr, ptep);
		page = phys_to_page(__pte_to_phys(pte));
		pte_clear(NULL, NULL, ptep);
		__free_page(page);
		nr++;
	} while (ptep++, addr += PAGE_SIZE, addr != end);

	page = pfn_to_page(__phys_to_pfn(__pmd_to_phys(*pmdp)));
	pmd_clear(NULL, NULL, pmdp);
	put_page(page);
	mm_dec_nr_ptes(vma->vm_mm);
}

static void vunmap_pmd_range(struct vm_area_struct *vma, pud_t *pudp,
						unsigned long addr, unsigned long end)
{
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
	pud_clear(NULL, NULL, pudp);
	put_page(virt_to_page(pmdp));
	mm_dec_nr_pmds(vma->vm_mm);
}

static void vumap_pud_range(struct vm_area_struct *vma, pgd_t *pgdp,
						unsigned long addr, unsigned long end)
{
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
	pgd_clear(NULL, NULL, pgdp);
	put_page(virt_to_page(pudp));
	mm_dec_nr_puds(vma->vm_mm);
}

void vumap_page_range(struct vm_area_struct *vma)
{
	pgd_t *pgdp;
	unsigned long next;
	unsigned long addr;
	unsigned long end;

	if (!vma)
		return;

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

static int __pud_alloc(struct vm_area_struct *vma, pgd_t *pgdp, unsigned long address)
{
	pud_t *new = (pud_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
	if (!new)
		return -ENOMEM;

	smp_wmb();

	spin_lock(&vma->vm_mm->page_table_lock);
	if (!pgd_present(*pgdp)) {
		__pgd_populate(pgdp, virt_to_phys((void *)new), PUD_TYPE_TABLE);
		mm_inc_nr_puds(vma->vm_mm);
	} else
		free_page((u64)new);

	spin_unlock(&vma->vm_mm->page_table_lock);

	return 0;
}

static inline pud_t *pud_alloc(struct vm_area_struct *vma, pgd_t *pgdp, unsigned long address)
{
	return (unlikely(pgd_none(*pgdp)) && __pud_alloc(vma, pgdp, address)) ?
		NULL : pud_offset(pgdp, address);
}

static int __pmd_alloc(struct vm_area_struct *vma, pud_t *pudp, unsigned long address)
{
	pmd_t *new = (pmd_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
	if (!new)
		return -ENOMEM;

	smp_wmb();

	spin_lock(&vma->vm_mm->page_table_lock);
	if (!pud_present(*pudp)) {
		__pud_populate(pudp, virt_to_phys((void *)new), PUD_TYPE_TABLE);
		mm_inc_nr_pmds(vma->vm_mm);
	} else
		free_page((u64)new);
	spin_unlock(&vma->vm_mm->page_table_lock);
	return 0;
}

static inline pmd_t *pmd_alloc(struct vm_area_struct *vma, pud_t *pudp, unsigned long address)
{
	return (unlikely(pud_none(*pudp)) && __pmd_alloc(vma, pudp, address)) ?
		NULL : pmd_offset(pudp, address);
}

static int __pte_alloc(struct vm_area_struct *vma, pmd_t *pmdp, unsigned long address)
{
	pte_t *new = (pte_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
	if (!new)
		return -ENOMEM;

	smp_wmb();

	spin_lock(&vma->vm_mm->page_table_lock);
	if (!pmd_present(*pmdp)) {
		__pmd_populate(pmdp, virt_to_phys((void *)new), PMD_TYPE_TABLE);
		mm_inc_nr_ptes(vma->vm_mm);
	} else
		free_page((u64)new);
	spin_unlock(&vma->vm_mm->page_table_lock);
	return 0;
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
	vumap_page_range(vma);
	return err;
}

static bool __insert_vma_area(struct vm_area_struct *vma)
{
	struct rb_node **new = &(vma->vm_mm->vma_rb_root.rb_node), *parent = NULL;

	while (*new) {
		struct vm_area_struct *this = container_of(*new, struct vm_area_struct, vm_rb_node);
	
		if (vma->vm_start < this->vm_end)
			new = &((*new)->rb_left);
		else if (vma->vm_end > this->vm_start)
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

	if (!mm)
		return;

	spin_lock(&mm->vma_lock);
	vma = __find_vma_area(addr, mm);
	if (!vma) {
		spin_unlock(&mm->vma_lock);
		return;
	}
	rb_erase(&vma->vm_rb_node, &mm->vma_rb_root);
	spin_unlock(&mm->vma_lock);

	kfree(vma);
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
		nr_used_pages() << (PAGE_SHIFT - 10),
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
