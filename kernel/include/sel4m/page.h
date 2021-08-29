#ifndef __SEL4M_PAGE_H_
#define __SEL4M_PAGE_H_

#include <sel4m/compiler.h>

#include <asm/pgtable.h>

#define __page_aligned_data	__section(.data..page_aligned) __aligned(PAGE_SIZE)
#define __page_aligned_bss	__section(.bss..page_aligned) __aligned(PAGE_SIZE)

/*
 * For assembly routines.
 *
 * Note when using these that you must specify the appropriate
 * alignment directives yourself
 */
#define __PAGE_ALIGNED_DATA	.section ".data..page_aligned", "aw"
#define __PAGE_ALIGNED_BSS	.section ".bss..page_aligned", "aw"

#endif /* !__SEL4M_PAGE_H_ */
