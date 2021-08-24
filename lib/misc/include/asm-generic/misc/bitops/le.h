/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_MISC_BITOPS_LE_H_
#define __ASM_GENERIC_MISC_BITOPS_LE_H_

#include <asm/misc/types.h>
#include <asm/misc/byteorder.h>

#if defined(__LITTLE_ENDIAN)

#define BITOP_LE_SWIZZLE	0

static inline u64 find_next_zero_bit_le(const void *addr,
		u64 size, u64 offset)
{
	return find_next_zero_bit(addr, size, offset);
}

static inline u64 find_next_bit_le(const void *addr,
		u64 size, u64 offset)
{
	return find_next_bit(addr, size, offset);
}

static inline u64 find_first_zero_bit_le(const void *addr,
		u64 size)
{
	return find_first_zero_bit(addr, size);
}

#elif defined(__BIG_ENDIAN)

#define BITOP_LE_SWIZZLE	((BITS_PER_LONG-1) & ~0x7)

#ifndef find_next_zero_bit_le
extern u64 find_next_zero_bit_le(const void *addr,
		u64 size, u64 offset);
#endif

#ifndef find_next_bit_le
extern u64 find_next_bit_le(const void *addr,
		u64 size, u64 offset);
#endif

#ifndef find_first_zero_bit_le
#define find_first_zero_bit_le(addr, size) \
	find_next_zero_bit_le((addr), (size), 0)
#endif

#else
#error "Please fix <asm/misc/byteorder.h>"
#endif

static inline int test_bit_le(int nr, const void *addr)
{
	return test_bit(nr ^ BITOP_LE_SWIZZLE, addr);
}

static inline void set_bit_le(int nr, void *addr)
{
	set_bit(nr ^ BITOP_LE_SWIZZLE, addr);
}

static inline void clear_bit_le(int nr, void *addr)
{
	clear_bit(nr ^ BITOP_LE_SWIZZLE, addr);
}

static inline void __set_bit_le(int nr, void *addr)
{
	__set_bit(nr ^ BITOP_LE_SWIZZLE, addr);
}

static inline void __clear_bit_le(int nr, void *addr)
{
	__clear_bit(nr ^ BITOP_LE_SWIZZLE, addr);
}

static inline int test_and_set_bit_le(int nr, void *addr)
{
	return test_and_set_bit(nr ^ BITOP_LE_SWIZZLE, addr);
}

static inline int test_and_clear_bit_le(int nr, void *addr)
{
	return test_and_clear_bit(nr ^ BITOP_LE_SWIZZLE, addr);
}

static inline int __test_and_set_bit_le(int nr, void *addr)
{
	return __test_and_set_bit(nr ^ BITOP_LE_SWIZZLE, addr);
}

static inline int __test_and_clear_bit_le(int nr, void *addr)
{
	return __test_and_clear_bit(nr ^ BITOP_LE_SWIZZLE, addr);
}

#endif /* !__ASM_GENERIC_MISC_BITOPS_LE_H_ */
