#ifndef __ASM_FIXMAP_H_
#define __ASM_FIXMAP_H_

#include <base/init.h>

#ifndef __ASSEMBLY__

#include <asm/kernel-pgtable.h>
#include <asm/memory.h>

enum fixed_addresses {
	FIX_HOLE,

	FIX_EARLYCON_MEM_BASE,

	__end_of_permanent_fixed_addresses,

	/*
	 * Used for kernel page table creation, so unmapped memory may be used
	 * for tables.
	 */
	FIX_PTE,
	FIX_PMD,
	FIX_PUD,
	FIX_PGD,

	__end_of_fixed_addresses
};

#define FIXADDR_SIZE	(__end_of_permanent_fixed_addresses << PAGE_SHIFT)
#define FIXADDR_START	(FIXADDR_TOP - FIXADDR_SIZE)

#define FIXMAP_PAGE_IO     __pgprot(PROT_DEVICE_nGnRE)

void __init early_fixmap_init(void);

#endif /* !__ASSEMBLY__ */
#endif /* !__ASM_FIXMAP_H_ */
