/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef __MISC_TYPES_H_
#define __MISC_TYPES_H_

#include <misc/stddef.h>

#include <asm/misc/types.h>

#ifndef __ASSEMBLY__

#ifndef __bitwise__
#define __bitwise__
#endif
#define __bitwise __bitwise__

typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;

typedef __u16 __bitwise __sum16;
typedef __u32 __bitwise __wsum;

/*
 * aligned_u64 should be used in defining kernel<->userspace ABIs to avoid
 * common 32/64-bit compat problems.
 * 64-bit values align to 4-byte boundaries on x86_32 (and possibly other
 * architectures) and to 8-byte boundaries on 64-bit architectures.  The new
 * aligned_64 type enforces 8-byte alignment so that structs containing
 * aligned_64 values have the same alignment on 32-bit and 64-bit architectures.
 * No conversions are necessary between 32-bit user-space and a 64-bit kernel.
 */
#define __aligned_u64 __u64 __attribute__((aligned(8)))
#define __aligned_be64 __be64 __attribute__((aligned(8)))
#define __aligned_le64 __le64 __attribute__((aligned(8)))

typedef unsigned __bitwise __poll_t;

typedef long	off_t;
typedef int 	pid_t;
typedef int 	key_t;
typedef long 	suseconds_t;
typedef int 	timer_t;
typedef int		clockid_t;

typedef _Bool			bool;

typedef unsigned long 	uintptr_t;

#if defined(__GNUC__)
typedef long long		loff_t;
#endif

#if __BITS_PER_LONG != 64
typedef unsigned int 		__size_t;
typedef int 			__ssize_t;
typedef int 			__ptrdiff_t;
#else
typedef unsigned long		__size_t;
typedef long			__ssize_t;
typedef long			__ptrdiff_t;
#endif

/*
 * The following typedefs are also protected by individual ifdefs for
 * historical reasons:
 */
#ifndef _SIZE_T
#define _SIZE_T
typedef __size_t		size_t;
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef __ssize_t		ssize_t;
#endif

#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef __ptrdiff_t		ptrdiff_t;
#endif

#ifndef _TIME_T
#define _TIME_T
typedef long		time_t;
#endif

#ifndef _CLOCK_T
#define _CLOCK_T
typedef long		clock_t;
#endif

#ifndef _CADDR_T
#define _CADDR_T
typedef char *				caddr_t;
#endif

/* bsd */
typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned int		u_int;
typedef unsigned long		u_long;

/* sysv */
typedef unsigned char		unchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;

#ifndef __BIT_TYPES_DEFINED__
#define __BIT_TYPES_DEFINED__

typedef u8			u_int8_t;
typedef s8			int8_t;
typedef u16			u_int16_t;
typedef s16			int16_t;
typedef u32			u_int32_t;
typedef s32			int32_t;

#endif /* !(__BIT_TYPES_DEFINED__) */

typedef u8			uint8_t;
typedef u16			uint16_t;
typedef u32			uint32_t;

#if defined(__GNUC__)
typedef u64			uint64_t;
typedef u64			u_int64_t;
typedef s64			int64_t;
#endif

/* this is a special 64bit data type that is 8-byte aligned */
#define aligned_u64		__aligned_u64
#define aligned_be64		__aligned_be64
#define aligned_le64		__aligned_le64

typedef unsigned long sector_t;
typedef unsigned long blkcnt_t;

/*
 * The type of an index into the pagecache.
 */
#define pgoff_t unsigned long

/*
 * A dma_addr_t can hold any valid DMA address, i.e., any address returned
 * by the DMA API.
 *
 * If the DMA API only uses 32-bit addresses, dma_addr_t need only be 32
 * bits wide.  Bus addresses, e.g., PCI BARs, may be wider than 32 bits,
 * but drivers do memory-mapped I/O to ioremapped kernel virtual addresses,
 * so they don't care about the size of the actual bus addresses.
 */
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
typedef u64 dma_addr_t;
#else
typedef u32 dma_addr_t;
#endif

#ifdef CONFIG_ARCH_PHYS_ADDR_T_64BIT
typedef u64 phys_addr_t;
#else
typedef u32 phys_addr_t;
#endif

# define __user
# define __kernel
# define __safe
# define __force
# define __nocast
# define __iomem
# define __chk_user_ptr(x) (void)0
# define __chk_io_ptr(x) (void)0
# define __builtin_warning(x, y...) (1)
# define __must_hold(x)
# define __acquires(x)
# define __releases(x)
# define __acquire(x) (void)0
# define __release(x) (void)0
# define __cond_lock(x,c) (c)
# define __percpu
# define __rcu
# define __private
# define ACCESS_PRIVATE(p, member) ((p)->member)

#endif /* !__ASSEMBLY__ */
#endif /* !__MISC_TYPES_H_ */
