/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_MISC_BITOPS_FFZ_H_
#define __ASM_GENERIC_MISC_BITOPS_FFZ_H_

/*
 * ffz - find first zero in word.
 * @word: The word to search
 *
 * Undefined if no zero exists, so code should check against ~0UL first.
 */
#define ffz(x)  __ffs(~(x))

#endif /* !__ASM_GENERIC_MISC_BITOPS_FFZ_H_ */
