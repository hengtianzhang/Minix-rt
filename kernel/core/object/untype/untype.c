#include <sel4m/gfp.h>
#include <sel4m/slab.h>
#include <sel4m/cpumask.h>
#include <sel4m/object/cap_types.h>
#include <sel4m/object/untype.h>
#include <sel4m/spinlock.h>
#include <sel4m/sched.h>

#include <asm/mmu.h>
#include <asm/current.h>
#include <asm/sections.h>

static int __pud_alloc(struct mm_struct *mm, pgd_t *pgdp, unsigned long address)
{
	pud_t *new = (pud_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
	if (!new)
		return -ENOMEM;

	smp_wmb();

	spin_lock(&mm->page_table_lock);
	if (!pgd_present(*pgdp))
		__pgd_populate(pgdp, virt_to_phys((void *)new), PUD_TYPE_TABLE);
	else
		free_page((u64)new);
	spin_unlock(&mm->page_table_lock);
	return 0;
}

static inline pud_t *pud_alloc(struct mm_struct *mm, pgd_t *pgdp, unsigned long address)
{
	return (unlikely(pgd_none(*pgdp)) && __pud_alloc(mm, pgdp, address)) ?
		NULL : pud_offset(pgdp, address);
}

static int __pmd_alloc(struct mm_struct *mm, pud_t *pudp, unsigned long address)
{
	pmd_t *new = (pmd_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
	if (!new)
		return -ENOMEM;

	smp_wmb();

	spin_lock(&mm->page_table_lock);
	if (!pud_present(*pudp))
		__pud_populate(pudp, virt_to_phys((void *)new), PUD_TYPE_TABLE);
	else
		free_page((u64)new);
	spin_unlock(&mm->page_table_lock);
	return 0;
}

static inline pmd_t *pmd_alloc(struct mm_struct *mm, pud_t *pudp, unsigned long address)
{
	return (unlikely(pud_none(*pudp)) && __pmd_alloc(mm, pudp, address)) ?
		NULL : pmd_offset(pudp, address);
}

static int __pte_alloc(struct mm_struct *mm, pmd_t *pmdp, unsigned long address)
{
	pte_t *new = (pte_t *)get_free_page(GFP_KERNEL | GFP_ZERO);
	if (!new)
		return -ENOMEM;

	smp_wmb();

	spin_lock(&mm->page_table_lock);
	if (!pmd_present(*pmdp))
		__pmd_populate(pmdp, virt_to_phys((void *)new), PMD_TYPE_TABLE);
	else
		free_page((u64)new);
	spin_unlock(&mm->page_table_lock);
	return 0;
}

static inline pte_t *pte_alloc(struct mm_struct *mm, pmd_t *pmdp, unsigned long address)
{
	return (unlikely(pmd_none(*pmdp)) && __pte_alloc(mm, pmdp, address)) ?
		NULL : pte_offset_kernel(pmdp, address);
}

static int vmap_pte_range(struct mm_struct *mm, pmd_t *pmdp,
					unsigned long addr, unsigned long end, pgprot_t prot, int *nr)
{
	pte_t *pte;

	/*
	 * nr is a running index into the array which helps higher level
	 * callers keep track of where we're up to.
	 */

	pte = pte_alloc(mm, pmdp, addr);
	if (!pte)
		return -ENOMEM;
	do {
		struct page *page = alloc_page(GFP_USER | GFP_ZERO);

		if (WARN_ON(!pte_none(*pte)))
			return -EBUSY;
		if (WARN_ON(!page))
			return -ENOMEM;

		set_pte(pte, mk_pte(page, prot));
		(*nr)++;
	} while (pte++, addr += PAGE_SIZE, addr != end);
	return 0;
}

static int vmap_pmd_range(struct mm_struct *mm, pud_t *pudp,
					unsigned long addr, unsigned long end, pgprot_t prot, int *nr)
{
	pmd_t *pmd;
	unsigned long next;

	pmd = pmd_alloc(mm, pudp, addr);
	if (!pmd)
		return -ENOMEM;
	do {
		next = pmd_addr_end(addr, end);
		if (vmap_pte_range(mm, pmd, addr, next, prot, nr))
			return -ENOMEM;
	} while (pmd++, addr = next, addr != end);

	return 0;
}

static int vmap_pud_range(struct mm_struct *mm, pgd_t *pgdp,
					unsigned long addr, unsigned long end, pgprot_t prot, int *nr)
{
	pud_t *pud;
	unsigned long next;

	pud = pud_alloc(mm, pgdp, addr);
	if (!pud)
		return -ENOMEM;
	do {
		next = pud_addr_end(addr, end);
		if (vmap_pmd_range(mm, pud, addr, next, prot, nr))
			return -ENOMEM;
	} while (pud++, addr = next, addr != end);

	return 0;
}

int vmap_page_range(struct mm_struct *mm, pgd_t *pgd, unsigned long start, unsigned long end, pgprot_t prot)
{
	pgd_t *pgdp;
	unsigned long next;
	unsigned long addr = start;
	int err = 0;
	int nr = 0;

	WARN_ON(addr >= end);
	pgdp = pgd_offset(pgd, addr);
	do {
		next = pgd_addr_end(addr, end);
		err = vmap_pud_range(mm, pgdp, addr, next, prot, &nr);
		if (err)
			return err;
	} while (pgdp++, addr = next, addr != end);

	return nr;
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
