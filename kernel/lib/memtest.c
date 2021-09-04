// SPDX-License-Identifier: GPL-2.0
#include <base/common.h>
#include <base/types.h>
#include <base/init.h>

#include <sel4m/memory.h>

static u64 patterns[] __initdata = {
	/* The first entry has to be 0 to leave memtest with zeroed memory */
	0,
	0xffffffffffffffffULL,
	0x5555555555555555ULL,
	0xaaaaaaaaaaaaaaaaULL,
	0x1111111111111111ULL,
	0x2222222222222222ULL,
	0x4444444444444444ULL,
	0x8888888888888888ULL,
	0x3333333333333333ULL,
	0x6666666666666666ULL,
	0x9999999999999999ULL,
	0xccccccccccccccccULL,
	0x7777777777777777ULL,
	0xbbbbbbbbbbbbbbbbULL,
	0xddddddddddddddddULL,
	0xeeeeeeeeeeeeeeeeULL,
	0x7a6c7258554e494cULL, /* yeah ;-) */
};

static void __init reserve_bad_mem(u64 pattern, phys_addr_t start_bad, phys_addr_t end_bad)
{
	printf("  %016llx bad mem addr 0x%016llx - 0x%016llx reserved\n",
		cpu_to_be64(pattern), start_bad, end_bad);
	memblock_reserve(&memblock_kernel, start_bad, end_bad - start_bad);
}

static void __init memtest(u64 pattern, phys_addr_t start_phys, phys_addr_t size)
{
	u64 *p, *start, *end;
	phys_addr_t start_bad, last_bad;
	phys_addr_t start_phys_aligned;
	const size_t incr = sizeof(pattern);

	start_phys_aligned = ALIGN(start_phys, incr);
	start = __va(start_phys_aligned);
	end = start + (size - (start_phys_aligned - start_phys)) / incr;
	start_bad = 0;
	last_bad = 0;

	for (p = start; p < end; p++)
		*p = pattern;

	for (p = start; p < end; p++, start_phys_aligned += incr) {
		if (*p == pattern)
			continue;
		if (start_phys_aligned == last_bad + incr) {
			last_bad += incr;
			continue;
		}
		if (start_bad)
			reserve_bad_mem(pattern, start_bad, last_bad + incr);
		start_bad = last_bad = start_phys_aligned;
	}
	if (start_bad)
		reserve_bad_mem(pattern, start_bad, last_bad + incr);
}

static void __init do_one_pass(u64 pattern, phys_addr_t start, phys_addr_t end)
{
	u64 i;
	phys_addr_t this_start, this_end;

	for_each_free_mem_range(&memblock_kernel, i, MEMBLOCK_NONE, &this_start,
				&this_end) {
		this_start = clamp(this_start, start, end);
		this_end = clamp(this_end, start, end);
		if (this_start < this_end) {
			printf("  0x%016llx - 0x%016llx pattern %016llx\n",
				this_start, this_end, cpu_to_be64(pattern));
			memtest(pattern, this_start, this_end - this_start);
		}
	}
}

void __init early_memtest(phys_addr_t start, phys_addr_t end)
{
	unsigned int i;
	unsigned int idx = 0;
    unsigned int memtest_pattern = ARRAY_SIZE(patterns);

	printf("early_memtest: # of tests: %u\n", memtest_pattern);
	for (i = memtest_pattern-1; i < UINT_MAX; --i) {
		idx = i % ARRAY_SIZE(patterns);
		do_one_pass(patterns[idx], start, end);
	}
}
