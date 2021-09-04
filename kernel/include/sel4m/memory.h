#ifndef __SEL4M_MEMORY_H_
#define __SEL4M_MEMORY_H_

#include <memalloc/memblock.h>

#include <asm/memory.h>

extern struct memblock memblock_kernel;

#ifdef CONFIG_MEMTEST
extern void early_memtest(phys_addr_t start, phys_addr_t end);
#else
static inline void early_memtest(phys_addr_t start, phys_addr_t end)
{
}
#endif

#endif /* !__SEL4M_MEMORY_H_ */
