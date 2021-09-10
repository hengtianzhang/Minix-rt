#include <sel4m/gfp.h>
#include <sel4m/slab.h>
#include <sel4m/cpumask.h>
#include <sel4m/object/cap_types.h>
#include <sel4m/object/untype.h>

#include <asm/mmu.h>
#include <asm/sections.h>

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
