/*
 * Based on arch/arm/include/asm/mmu_context.h
 *
 * Copyright (C) 1996 Russell King.
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
 */
#ifndef __ASM_MMU_CONTEXT_H_
#define __ASM_MMU_CONTEXT_H_

#ifndef __ASSEMBLY__

#include <base/types.h>

/*
 * TCR.T0SZ value to use when the ID map is active. Usually equals
 * TCR_T0SZ(VA_BITS), unless system RAM is positioned very high in
 * physical memory, in which case it will be smaller.
 */
extern u64 idmap_t0sz;
extern u64 idmap_ptrs_per_pgd;

#endif /* !__ASSEMBLY__ */
#endif /* !__ASM_MMU_CONTEXT_H_ */
