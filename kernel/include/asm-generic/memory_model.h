/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_MEMORY_MODEL_H_
#define __ASM_MEMORY_MODEL_H_

#include <base/pfn.h>

#ifndef __ASSEMBLY__

/* memmap is virtually contiguous.  */
#define __pfn_to_page(pfn)	(vmemmap + (pfn))
#define __page_to_pfn(page)	(unsigned long)((page) - vmemmap)

#define page_to_pfn __page_to_pfn
#define pfn_to_page __pfn_to_page

#endif /* !__ASSEMBLY__ */
#endif /* !__ASM_MEMORY_MODEL_H_ */
