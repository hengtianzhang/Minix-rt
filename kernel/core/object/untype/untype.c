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

static bool pud_untype_insert(struct untype_pud *upud)
{
	struct rb_node **new = &(current->mm->pud_root.rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct untype_pud *this = container_of(*new, struct untype_pud, pud_node);

		parent = *new;
		if (upud->pud_page < this->pud_page)
			new = &((*new)->rb_left);
		else if (upud->pud_page > this->pud_page)
			new = &((*new)->rb_right);
		else
			return false;
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&upud->pud_node, parent, new);
	rb_insert_color(&upud->pud_node, &current->mm->pud_root);

	return true;
}

static struct untype_pud *pud_untype_find(struct page *page)
{
	struct rb_node *node = current->mm->pud_root.rb_node;

	while (node) {
		struct untype_pud *data = container_of(node, struct untype_pud, pud_node);
	
		if (page < data->pud_page)
			node = node->rb_left;
		else if (page > data->pud_page)
			node = node->rb_right;
		else
			return data;
	}

	return NULL;
}

static void mm_pud_ref_inc(struct page *pud_page)
{
	struct untype_pud *upud;

	upud = pud_untype_find(pud_page);
	BUG_ON(!upud);

	spin_lock(&current->mm->page_table_lock);
	atomic_long_add(1, &upud->pud_ref_count);
	spin_unlock(&current->mm->page_table_lock);
}

static void mm_pud_ref_dec(struct page *pud_page)
{
	struct untype_pud *upud;

	upud = pud_untype_find(pud_page);
	BUG_ON(!upud);

	spin_lock(&current->mm->page_table_lock);
	if (atomic_long_dec_and_test(&upud->pud_ref_count)) {
		__free_page(upud->pud_page);
		rb_erase(&upud->pud_node, &current->mm->pud_root);
		kfree(upud);
	}
	spin_unlock(&current->mm->page_table_lock);
}

int untype_create_pud_map(pgd_t *pgd, unsigned long addr, unsigned long end)
{
	unsigned long next = addr;
	pgd_t *pgdp;
	pgd_t pud;
	struct untype_pud *upud; 
	struct page *pud_page;

	do {
		pgdp = pgd_offset(pgd, next);
		pud = READ_ONCE(*pgdp);

		if (pgd_present(pud))
			mm_pud_ref_inc(phys_to_page(__pgd_to_phys(pud)));
		else {
			upud = kmalloc(sizeof (struct untype_pud), GFP_KERNEL);
			if(!upud)
				return -ENOMEM;
			pud_page = alloc_page(GFP_KERNEL | GFP_ZERO);
			if (!pud_page) {
				kfree(upud);
				BUG();
				return -ENOMEM; 
			}
			upud->pud_page = pud_page;
			atomic_long_set(&upud->pud_ref_count, 1);

			spin_lock(&current->mm->page_table_lock);
			BUG_ON(!pud_untype_insert(upud));
			spin_unlock(&current->mm->page_table_lock);

			__pgd_populate(pgdp, page_to_phys(pud_page), PUD_TYPE_TABLE);
		}

		next = pgd_addr_end(addr, end);
	} while (addr = next, addr != end);

	return 0;
}

int untype_remove_pud_map(pgd_t *pgd, unsigned long addr, unsigned long end)
{
	unsigned long next = addr;
	pgd_t *pgdp;
	pgd_t pud;

	do {
		pgdp = pgd_offset(pgd, next);
		pud = READ_ONCE(*pgdp);

		if (pgd_present(pud))
			mm_pud_ref_dec(phys_to_page(__pgd_to_phys(pud)));
		else {
			BUG();
		}

		next = pgd_addr_end(addr, end);
	} while (addr = next, addr != end);

	return 0;
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
