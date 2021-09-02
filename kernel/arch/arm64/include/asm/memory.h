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

#define VA_BITS			(CONFIG_ARM64_VA_BITS)
#define VA_START		(ULL(0xffffffffffffffff) - \
	(ULL(1) << VA_BITS) + 1)
#define PAGE_OFFSET		(ULL(0xffffffffffffffff) - \
	(ULL(1) << (VA_BITS - 1)) + 1)
#define KIMAGE_VADDR		(VA_START)

#define RESERVED_IO_SPACE	SZ_2M
#define	FIXADDR_TOP		(PAGE_OFFSET - RESERVED_IO_SPACE)

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

/* the virtual base of the kernel image (minus TEXT_OFFSET) */
extern u64			kimage_vaddr;

/* the offset between the kernel virtual and physical mappings */
extern u64			kimage_voffset;

/*
 * TCR.T0SZ value to use when the ID map is active. Usually equals
 * TCR_T0SZ(VA_BITS), unless system RAM is positioned very high in
 * physical memory, in which case it will be smaller.
 */
extern u64 idmap_t0sz;
extern u64 idmap_ptrs_per_pgd;

#define __kimg_to_phys(addr)	((addr) - kimage_voffset)
#define __phys_to_kimg(x)	((u64)((x) + kimage_voffset))

#define __phys_addr_symbol(x)	__kimg_to_phys((phys_addr_t)(x))

#define __pa_symbol(x)		__phys_addr_symbol(RELOC_HIDE((unsigned long)(x), 0))

#endif /* !__ASSEMBLY__ */

#endif /* !__ASM_MEMORY_H_ */
