/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_BASE_BITOPS_BUILTIN_FFS_H_
#define __ASM_GENERIC_BASE_BITOPS_BUILTIN_FFS_H_

/**
 * ffs - find first bit set
 * @x: the word to search
 *
 * This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */
static __always_inline int ffs(int x)
{
	return __builtin_ffs(x);
}

#endif /* !__ASM_GENERIC_BASE_BITOPS_BUILTIN_FFS_H_ */
