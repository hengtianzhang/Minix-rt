/*
 * Based on arch/arm/include/asm/page.h
 *
 * Copyright (C) 1995-2003 Russell King
 * Copyright (C) 2017 ARM Ltd.
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
#ifndef __ASM_MISC_PAGE_H_
#define __ASM_MISC_PAGE_H_

#include <misc/const.h>

/* PAGE_SHIFT determines the page size */
/* CONT_SHIFT determines the number of pages which can be tracked together  */
#ifndef PAGE_SHIFT
#define PAGE_SHIFT		CONFIG_ARM64_PAGE_SHIFT
#endif

#ifndef CONT_SHIFT
#define CONT_SHIFT		CONFIG_ARM64_CONT_SHIFT
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE		(_AC(1, UL) << PAGE_SHIFT)
#endif

#ifndef PAGE_MASK
#define PAGE_MASK		(~(PAGE_SIZE-1))
#endif

#ifndef CONT_SIZE
#define CONT_SIZE		(_AC(1, UL) << (CONT_SHIFT + PAGE_SHIFT))
#endif

#ifndef CONT_MASK
#define CONT_MASK		(~(CONT_SIZE-1))
#endif

#endif /* !__ASM_MISC_PAGE_H_ */
