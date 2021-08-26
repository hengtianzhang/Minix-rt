#ifndef __SEL4M_TYPES_H_
#define __SEL4M_TYPES_H_

#include <sel4m/stddef.h>

#include <asm/types.h>

#ifndef __ASSEMBLY__

typedef __s8  	s8;
typedef __u8  	u8;
typedef __s16 	s16;
typedef __u16	u16;
typedef __s32 	s32;
typedef __u32 	u32;
typedef __s64 	s64;
typedef __u64 	u64;

#if __BITS_PER_LONG != 64
typedef u32		size_t;
typedef s32		ssize_t;
#else
typedef unsigned long		size_t;
typedef __signed__ long		ssize_t;
#endif

typedef _Bool	bool;

/* this is a special 64bit data type that is 8-byte aligned */
#define aligned_u64		__aligned_u64
#define aligned_be64		__aligned_be64
#define aligned_le64		__aligned_le64

#ifdef CONFIG_ARCH_PHYS_ADDR_T_64BIT
typedef u64 	phys_addr_t;
#else
typedef u32 	phys_addr_t;
#endif

#endif /* !__ASSEMBLY__ */
#endif /* !__SEL4M_TYPES_H_ */