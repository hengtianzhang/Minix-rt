#include <stdlib.h>

#include <base/compiler.h>
#include <base/common.h>

#include <asm/base/page-def.h>

#include <memalloc/memblock.h>

#include <libminix_rt/brk.h>

static unsigned long initialized_base = 0;
static struct memblock memblock;

unsigned long get_free_page(void)
{
	void *ptr;
	unsigned long addr;

	if (!initialized_base) {
		memblock_init(&memblock);
		initialized_base = (unsigned long)sbrk(0);
	}

retry:
	addr = memblock_alloc(&memblock, PAGE_SIZE, PAGE_SIZE);
	if (!addr) {
		ptr = sbrk(PAGE_SIZE);
		if (!ptr)
			return 0;
		memblock_add(&memblock, (unsigned long)ptr, PAGE_SIZE);
		goto retry;
	}

	return addr;
}
