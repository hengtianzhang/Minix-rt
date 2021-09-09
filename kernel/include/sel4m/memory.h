#ifndef __SEL4M_MEMORY_H_
#define __SEL4M_MEMORY_H_

#include <base/string.h>

#include <memalloc/memblock.h>

#include <asm/pgtable.h>
#include <asm/memory.h>

extern struct memblock memblock_kernel;

static inline void * __init memblock_alloc_virt(struct memblock *mb, phys_addr_t size,  phys_addr_t align)
{
	void *virt;
	phys_addr_t addr;

	addr = memblock_alloc(mb, size, align);
	if (!addr)
		return NULL;

	virt = phys_to_virt(addr);
	memset(virt, 0, size);
	return virt;
}

#ifdef CONFIG_MEMTEST
extern void early_memtest(phys_addr_t start, phys_addr_t end);
#else
static inline void early_memtest(phys_addr_t start, phys_addr_t end)
{
}
#endif

#endif /* !__SEL4M_MEMORY_H_ */
