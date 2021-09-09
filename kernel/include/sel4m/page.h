#ifndef __SEL4M_PAGE_H_
#define __SEL4M_PAGE_H_

#include <base/compiler.h>

#include <asm/base/page-def.h>

#include <asm/pgtable.h>

/*
 * For assembly routines.
 *
 * Note when using these that you must specify the appropriate
 * alignment directives yourself
 */
#define __PAGE_ALIGNED_DATA	.section ".data..page_aligned", "aw"
#define __PAGE_ALIGNED_BSS	.section ".bss..page_aligned", "aw"

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)

/* test whether an address (unsigned long or pointer) is aligned to PAGE_SIZE */
#define PAGE_ALIGNED(addr)	IS_ALIGNED((unsigned long)(addr), PAGE_SIZE)

#ifndef __ASSEMBLY__

#define __page_aligned_data	__section(.data..page_aligned) __aligned(PAGE_SIZE)
#define __page_aligned_bss	__section(.bss..page_aligned) __aligned(PAGE_SIZE)

extern int pfn_valid(unsigned long);

extern void clear_page(void *to);

#endif /* !__ASSEMBLY__ */
#endif /* !__SEL4M_PAGE_H_ */
