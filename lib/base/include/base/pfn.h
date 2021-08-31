/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BASE_PFN_H_
#define __BASE_PFN_H_

#ifndef __ASSEMBLY__

#include <base/types.h>

#include <asm/base/page-def.h>

/*
 * pfn_t: encapsulates a page-frame number that is optionally backed
 * by memmap (struct page).  Whether a pfn_t has a 'struct page'
 * backing is indicated by flags in the high bits of the value.
 */
typedef struct {
	u64 val;
} pfn_t;

/*
 * Convert a physical address to a Page Frame Number and back
 */
#define	__phys_to_pfn(paddr)	PHYS_PFN(paddr)
#define	__pfn_to_phys(pfn)	PFN_PHYS(pfn)

#endif /* !__ASSEMBLY__ */

#define PFN_ALIGN(x)	(((u64)(x) + (PAGE_SIZE - 1)) & PAGE_MASK)
#define PFN_UP(x)	(((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)
#define PFN_DOWN(x)	((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)	((phys_addr_t)(x) << PAGE_SHIFT)
#define PHYS_PFN(x)	((u64)((x) >> PAGE_SHIFT))

#endif /* !__BASE_PFN_H_ */
