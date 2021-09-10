/*
 * Based on arch/arm/include/asm/memory.h
 *
 * Copyright (C) 2000-2002 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Note: this file should not be included by non-asm/.h files
 */
#ifndef __ASM_MEMORY_H_
#define __ASM_MEMORY_H_

#include <base/sizes.h>
#include <base/const.h>

#include <sel4m/mm_types.h>

/*
 * VMEMMAP_SIZE - allows the whole linear region to be covered by
 *                a struct page array
 */
#define VMEMMAP_SIZE (UL(1) << (VA_BITS - PAGE_SHIFT - 1 + STRUCT_PAGE_MAX_SHIFT))

#define VA_BITS			(CONFIG_ARM64_VA_BITS)
#define VA_START		(ULL(0xffffffffffffffff) - \
	(ULL(1) << VA_BITS) + 1)
#define PAGE_OFFSET		(ULL(0xffffffffffffffff) - \
	(ULL(1) << (VA_BITS - 1)) + 1)
#define KIMAGE_VADDR		(VA_START)

#define VMEMMAP_START		(PAGE_OFFSET - VMEMMAP_SIZE)

#define VIOMAP_SIZE (ULL(1) << (VA_BITS - PAGE_SHIFT - 1))
#define VIOMAP_START (VMEMMAP_START - VIOMAP_SIZE)

#define RESERVED_IO_SPACE	SZ_2M
#define	FIXADDR_TOP		(VIOMAP_START - RESERVED_IO_SPACE)

#define MAX_USER_VA_BITS	VA_BITS

/*
 *  4 KB granule:  16 level 3 entries, with contiguous bit
 * 16 KB granule:   4 level 3 entries, without contiguous bit
 * 64 KB granule:   1 level 3 entry
 */
#define SEGMENT_ALIGN			SZ_64K

/*
 * Memory types available.
 */
#define MT_DEVICE_nGnRnE	0
#define MT_DEVICE_nGnRE		1
#define MT_DEVICE_GRE		2
#define MT_NORMAL_NC		3
#define MT_NORMAL		4
#define MT_NORMAL_WT		5

#define MIN_THREAD_SHIFT	(14)

#define THREAD_SHIFT		MIN_THREAD_SHIFT

#define THREAD_SIZE		(ULL(1) << THREAD_SHIFT)

#define THREAD_ALIGN		THREAD_SIZE

#define IRQ_STACK_SIZE		THREAD_SIZE

#define THREAD_STACK_ALIGN	SZ_16

#ifndef __ASSEMBLY__

#include <base/bug.h>
#include <base/types.h>
#include <base/pfn.h>
#include <base/bits.h>

extern s64			memstart_addr;
/* PHYS_OFFSET - the physical address of the start of memory. */
#define PHYS_OFFSET		({ BUG_ON(memstart_addr & 1); memstart_addr; })

/* the virtual base of the kernel image (minus TEXT_OFFSET) */
extern u64			kimage_vaddr;

/* the offset between the kernel virtual and physical mappings */
extern u64			kimage_voffset;

/*
 * Allow all memory at the discovery stage. We will clip it later.
 */
#define MIN_MEMBLOCK_ADDR	0
#define MAX_MEMBLOCK_ADDR	U64_MAX

/*
 * The linear kernel range starts in the middle of the virtual adddress
 * space. Testing the top bit for the start of the region is a
 * sufficient check.
 */
#define __is_lm_address(addr)	(!!((addr) & BIT(VA_BITS - 1)))

#define __lm_to_phys(addr)	(((addr) & ~PAGE_OFFSET) + PHYS_OFFSET)
#define __kimg_to_phys(addr)	((addr) - kimage_voffset)

#define __virt_to_phys(x) ({			\
	phys_addr_t __x = (phys_addr_t)(x);			\
	__is_lm_address(__x) ? __lm_to_phys(__x) :		\
			       __kimg_to_phys(__x);			\
})

#define __phys_addr_symbol(x) __kimg_to_phys((phys_addr_t)(x))

#define __phys_to_virt(x)	((unsigned long)((x) - PHYS_OFFSET) | PAGE_OFFSET)
#define __phys_to_kimg(x)	((u64)((x) + kimage_voffset))

#include <asm-generic/memory_model.h>
#include <asm-generic/getorder.h>
/*
 * Convert a page to/from a physical address
 */
#define page_to_phys(page)	(__pfn_to_phys(page_to_pfn(page)))
#define phys_to_page(phys)	(pfn_to_page(__phys_to_pfn(phys)))
#define virt_to_page(kaddr)	pfn_to_page(__pa(kaddr) >> PAGE_SHIFT)

/*
 * Note: Drivers should NOT use these.  They are the wrong
 * translation for translating DMA addresses.  Use the driver
 * DMA support - see dma-mapping.h.
 */
#define virt_to_phys virt_to_phys
static inline phys_addr_t virt_to_phys(const volatile void *x)
{
	return __virt_to_phys((unsigned long)(x));
}

#define phys_to_virt phys_to_virt
static inline void *phys_to_virt(phys_addr_t x)
{
	return (void *)(__phys_to_virt(x));
}

#define __pa(x)			__virt_to_phys((unsigned long)(x))
#define __pa_symbol(x)		__phys_addr_symbol(RELOC_HIDE((unsigned long)(x), 0))
#define __va(x)			((void *)__phys_to_virt((phys_addr_t)(x)))
#define virt_to_pfn(x)      __phys_to_pfn(__virt_to_phys((unsigned long)(x)))

#define _virt_addr_is_linear(kaddr)	\
	((u64)(kaddr) >= PAGE_OFFSET)
#define virt_addr_valid(kaddr)		\
	(_virt_addr_is_linear(kaddr) && (pfn_valid(__pa(kaddr) >> PAGE_SHIFT)))

extern void arm64_memblock_init(void);

#endif /* !__ASSEMBLY__ */
#endif /* !__ASM_MEMORY_H_ */
