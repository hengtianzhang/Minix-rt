/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_MISC_BITOPS_ATOMIC_H_
#define __ASM_GENERIC_MISC_BITOPS_ATOMIC_H_

#include <misc/atomic.h>
#include <misc/compiler.h>

#include <asm/misc/barrier.h>

/*
 * Implementation of atomic bitops using atomic-fetch ops.
 * See Documentation/atomic_bitops.txt for details.
 */

static inline void set_bit(unsigned int nr, volatile unsigned long *p)
{
	p += BIT_WORD(nr);
	atomic_long_or(BIT_MASK(nr), (atomic_long_t *)p);
}

static inline void clear_bit(unsigned int nr, volatile unsigned long *p)
{
	p += BIT_WORD(nr);
	atomic_long_andnot(BIT_MASK(nr), (atomic_long_t *)p);
}

static inline void change_bit(unsigned int nr, volatile unsigned long *p)
{
	p += BIT_WORD(nr);
	atomic_long_xor(BIT_MASK(nr), (atomic_long_t *)p);
}

static inline int test_and_set_bit(unsigned int nr, volatile unsigned long *p)
{
	long old;
	unsigned long mask = BIT_MASK(nr);

	p += BIT_WORD(nr);
	if (READ_ONCE(*p) & mask)
		return 1;

	old = atomic_long_fetch_or(mask, (atomic_long_t *)p);
	return !!(old & mask);
}

static inline int test_and_clear_bit(unsigned int nr, volatile unsigned long *p)
{
	long old;
	unsigned long mask = BIT_MASK(nr);

	p += BIT_WORD(nr);
	if (!(READ_ONCE(*p) & mask))
		return 0;

	old = atomic_long_fetch_andnot(mask, (atomic_long_t *)p);
	return !!(old & mask);
}

static inline int test_and_change_bit(unsigned int nr, volatile unsigned long *p)
{
	long old;
	unsigned long mask = BIT_MASK(nr);

	p += BIT_WORD(nr);
	old = atomic_long_fetch_xor(mask, (atomic_long_t *)p);
	return !!(old & mask);
}

#endif /* !__ASM_GENERIC_MISC_BITOPS_ATOMIC_H_ */
