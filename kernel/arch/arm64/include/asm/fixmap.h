/*
 * fixmap.h: compile-time virtual memory allocation
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1998 Ingo Molnar
 * Copyright (C) 2013 Mark Salter <msalter@redhat.com>
 *
 * Adapted from arch/x86 version.
 *
 */
#ifndef __ASM_ARM64_FIXMAP_H_
#define __ASM_ARM64_FIXMAP_H_

#ifndef __ASSEMBLY__

#include <sel4m/init.h>

#include <asm/memory.h>
#include <asm/kernel-pgtable.h>

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
#endif /* !__ASM_ARM64_FIXMAP_H_ */
