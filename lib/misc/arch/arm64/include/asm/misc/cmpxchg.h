/*
 * Based on arch/arm/include/asm/cmpxchg.h
 *
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
#ifndef __ASM_CMPXCHG_H_
#define __ASM_CMPXCHG_H_

#include <misc/types.h>
#include <misc/compiler.h>

/*
 * We need separate acquire parameters for ll/sc and lse, since the full
 * barrier case is generated as release+dmb for the former and
 * acquire+release for the latter.
 */
#define __XCHG_CASE(w, sfx, name, sz, mb, nop_lse, acq, acq_lse, rel, cl)	\
static inline u##sz __xchg_case_##name##sz(u##sz x, volatile void *ptr)		\
{										\
	u##sz ret;								\
	u64 tmp;							\
										\
	asm volatile(							\
	"	prfm	pstl1strm, %2\n"					\
	"1:	ld" #acq "xr" #sfx "\t%" #w "0, %2\n"				\
	"	st" #rel "xr" #sfx "\t%w1, %" #w "3, %2\n"			\
	"	cbnz	%w1, 1b\n"						\
	"	" #mb							\
	: "=&r" (ret), "=&r" (tmp), "+Q" (*(u##sz *)ptr)			\
	: "r" (x)								\
	: cl);									\
										\
	return ret;								\
}

__XCHG_CASE(w, b,     ,  8,        ,    ,  ,  ,  ,         )
__XCHG_CASE(w, h,     , 16,        ,    ,  ,  ,  ,         )
__XCHG_CASE(w,  ,     , 32,        ,    ,  ,  ,  ,         )
__XCHG_CASE( ,  ,     , 64,        ,    ,  ,  ,  ,         )
__XCHG_CASE(w, b, acq_,  8,        ,    , a, a,  , "memory")
__XCHG_CASE(w, h, acq_, 16,        ,    , a, a,  , "memory")
__XCHG_CASE(w,  , acq_, 32,        ,    , a, a,  , "memory")
__XCHG_CASE( ,  , acq_, 64,        ,    , a, a,  , "memory")
__XCHG_CASE(w, b, rel_,  8,        ,    ,  ,  , l, "memory")
__XCHG_CASE(w, h, rel_, 16,        ,    ,  ,  , l, "memory")
__XCHG_CASE(w,  , rel_, 32,        ,    ,  ,  , l, "memory")
__XCHG_CASE( ,  , rel_, 64,        ,    ,  ,  , l, "memory")
__XCHG_CASE(w, b,  mb_,  8, dmb ish, nop,  , a, l, "memory")
__XCHG_CASE(w, h,  mb_, 16, dmb ish, nop,  , a, l, "memory")
__XCHG_CASE(w,  ,  mb_, 32, dmb ish, nop,  , a, l, "memory")
__XCHG_CASE( ,  ,  mb_, 64, dmb ish, nop,  , a, l, "memory")

#undef __XCHG_CASE

#define __XCHG_GEN(sfx)							\
static inline u64 __xchg##sfx(u64 x,		\
					volatile void *ptr,		\
					int size)			\
{									\
	switch (size) {							\
	case 1:								\
		return __xchg_case##sfx##_8(x, ptr);			\
	case 2:								\
		return __xchg_case##sfx##_16(x, ptr);			\
	case 4:								\
		return __xchg_case##sfx##_32(x, ptr);			\
	case 8:								\
		return __xchg_case##sfx##_64(x, ptr);			\
	default:							\
		BUILD_BUG();						\
	}								\
									\
	unreachable();							\
}

__XCHG_GEN()
__XCHG_GEN(_acq)
__XCHG_GEN(_rel)
__XCHG_GEN(_mb)

#undef __XCHG_GEN

#define __xchg_wrapper(sfx, ptr, x)					\
({									\
	__typeof__(*(ptr)) __ret;					\
	__ret = (__typeof__(*(ptr)))					\
		__xchg##sfx((u64)(x), (ptr), sizeof(*(ptr))); \
	__ret;								\
})

/* xchg */
#define xchg_relaxed(...)	__xchg_wrapper(    , __VA_ARGS__)
#define xchg_acquire(...)	__xchg_wrapper(_acq, __VA_ARGS__)
#define xchg_release(...)	__xchg_wrapper(_rel, __VA_ARGS__)
#define xchg(...)		__xchg_wrapper( _mb, __VA_ARGS__)

#define __CMPXCHG_CASE(w, sfx, name, sz, mb, acq, rel, cl)		\
static inline u##sz __cmpxchg_case_##name##sz(volatile void *ptr,		\
					 u64 old,		\
					 u##sz new)			\
{									\
	u64 tmp;						\
	u##sz oldval;							\
									\
	/*								\
	 * Sub-word sizes require explicit casting so that the compare  \
	 * part of the cmpxchg doesn't end up interpreting non-zero	\
	 * upper bits of the register containing "old".			\
	 */								\
	if (sz < 32)							\
		old = (u##sz)old;					\
									\
	asm volatile(							\
	"	prfm	pstl1strm, %[v]\n"				\
	"1:	ld" #acq "xr" #sfx "\t%" #w "[oldval], %[v]\n"		\
	"	eor	%" #w "[tmp], %" #w "[oldval], %" #w "[old]\n"	\
	"	cbnz	%" #w "[tmp], 2f\n"				\
	"	st" #rel "xr" #sfx "\t%w[tmp], %" #w "[new], %[v]\n"	\
	"	cbnz	%w[tmp], 1b\n"					\
	"	" #mb "\n"						\
	"2:"								\
	: [tmp] "=&r" (tmp), [oldval] "=&r" (oldval),			\
	  [v] "+Q" (*(u##sz *)ptr)					\
	: [old] "Kr" (old), [new] "r" (new)				\
	: cl);								\
									\
	return oldval;							\
}

__CMPXCHG_CASE(w, b,     ,  8,        ,  ,  ,         )
__CMPXCHG_CASE(w, h,     , 16,        ,  ,  ,         )
__CMPXCHG_CASE(w,  ,     , 32,        ,  ,  ,         )
__CMPXCHG_CASE( ,  ,     , 64,        ,  ,  ,         )
__CMPXCHG_CASE(w, b, acq_,  8,        , a,  , "memory")
__CMPXCHG_CASE(w, h, acq_, 16,        , a,  , "memory")
__CMPXCHG_CASE(w,  , acq_, 32,        , a,  , "memory")
__CMPXCHG_CASE( ,  , acq_, 64,        , a,  , "memory")
__CMPXCHG_CASE(w, b, rel_,  8,        ,  , l, "memory")
__CMPXCHG_CASE(w, h, rel_, 16,        ,  , l, "memory")
__CMPXCHG_CASE(w,  , rel_, 32,        ,  , l, "memory")
__CMPXCHG_CASE( ,  , rel_, 64,        ,  , l, "memory")
__CMPXCHG_CASE(w, b,  mb_,  8, dmb ish,  , l, "memory")
__CMPXCHG_CASE(w, h,  mb_, 16, dmb ish,  , l, "memory")
__CMPXCHG_CASE(w,  ,  mb_, 32, dmb ish,  , l, "memory")
__CMPXCHG_CASE( ,  ,  mb_, 64, dmb ish,  , l, "memory")

#undef __CMPXCHG_CASE

#define __CMPXCHG_GEN(sfx)						\
static inline u64 __cmpxchg##sfx(volatile void *ptr,		\
					   u64 old,		\
					   u64 new,		\
					   int size)			\
{									\
	switch (size) {							\
	case 1:								\
		return __cmpxchg_case##sfx##_8(ptr, old, new);		\
	case 2:								\
		return __cmpxchg_case##sfx##_16(ptr, old, new);		\
	case 4:								\
		return __cmpxchg_case##sfx##_32(ptr, old, new);		\
	case 8:								\
		return __cmpxchg_case##sfx##_64(ptr, old, new);		\
	default:							\
		BUILD_BUG();						\
	}								\
									\
	unreachable();							\
}

__CMPXCHG_GEN()
__CMPXCHG_GEN(_acq)
__CMPXCHG_GEN(_rel)
__CMPXCHG_GEN(_mb)

#undef __CMPXCHG_GEN

#define __cmpxchg_wrapper(sfx, ptr, o, n)				\
({									\
	__typeof__(*(ptr)) __ret;					\
	__ret = (__typeof__(*(ptr)))					\
		__cmpxchg##sfx((ptr), (u64)(o),		\
				(u64)(n), sizeof(*(ptr)));	\
	__ret;								\
})

/* cmpxchg */
#define cmpxchg_relaxed(...)	__cmpxchg_wrapper(    , __VA_ARGS__)
#define cmpxchg_acquire(...)	__cmpxchg_wrapper(_acq, __VA_ARGS__)
#define cmpxchg_release(...)	__cmpxchg_wrapper(_rel, __VA_ARGS__)
#define cmpxchg(...)		__cmpxchg_wrapper( _mb, __VA_ARGS__)
#define cmpxchg_local		cmpxchg_relaxed

/* cmpxchg64 */
#define cmpxchg64_relaxed	cmpxchg_relaxed
#define cmpxchg64_acquire	cmpxchg_acquire
#define cmpxchg64_release	cmpxchg_release
#define cmpxchg64		cmpxchg
#define cmpxchg64_local		cmpxchg_local

/* cmpxchg_double */
#define system_has_cmpxchg_double()     1

#define __cmpxchg_double_check(ptr1, ptr2)					\
({										\
	if (sizeof(*(ptr1)) != 8)						\
		BUILD_BUG();							\
	BUG_ON((u64 *)(ptr2) - (u64 *)(ptr1) != 1);	\
})

#define __CMPXCHG_DBL(name, mb, rel, cl)				\
static inline s64 __cmpxchg_double##name(u64 old1,		\
				      u64 old2,		\
				      u64 new1,		\
				      u64 new2,		\
				      volatile void *ptr)		\
{									\
	u64 tmp, ret;						\
									\
	asm volatile("// __cmpxchg_double" #name "\n"			\
	"	prfm	pstl1strm, %2\n"				\
	"1:	ldxp	%0, %1, %2\n"					\
	"	eor	%0, %0, %3\n"					\
	"	eor	%1, %1, %4\n"					\
	"	orr	%1, %0, %1\n"					\
	"	cbnz	%1, 2f\n"					\
	"	st" #rel "xp	%w0, %5, %6, %2\n"			\
	"	cbnz	%w0, 1b\n"					\
	"	" #mb "\n"						\
	"2:"								\
	: "=&r" (tmp), "=&r" (ret), "+Q" (*(u64 *)ptr)	\
	: "r" (old1), "r" (old2), "r" (new1), "r" (new2)		\
	: cl);								\
									\
	return ret;							\
}

__CMPXCHG_DBL(   ,        ,  ,         )
__CMPXCHG_DBL(_mb, dmb ish, l, "memory")

#undef __CMPXCHG_DBL

#define cmpxchg_double(ptr1, ptr2, o1, o2, n1, n2) \
({\
	int __ret;\
	__cmpxchg_double_check(ptr1, ptr2); \
	__ret = !__cmpxchg_double_mb((u64)(o1), (u64)(o2), \
				     (u64)(n1), (u64)(n2), \
				     ptr1); \
	__ret; \
})

#define cmpxchg_double_local(ptr1, ptr2, o1, o2, n1, n2) \
({\
	int __ret;\
	__cmpxchg_double_check(ptr1, ptr2); \
	__ret = !__cmpxchg_double((u64)(o1), (u64)(o2), \
				  (u64)(n1), (u64)(n2), \
				  ptr1); \
	__ret; \
})

#define __CMPWAIT_CASE(w, sfx, sz)					\
static inline void __cmpwait_case_##sz(volatile void *ptr,		\
				       u64 val)		\
{									\
	u64 tmp;						\
									\
	asm volatile(							\
	"	sevl\n"							\
	"	wfe\n"							\
	"	ldxr" #sfx "\t%" #w "[tmp], %[v]\n"			\
	"	eor	%" #w "[tmp], %" #w "[tmp], %" #w "[val]\n"	\
	"	cbnz	%" #w "[tmp], 1f\n"				\
	"	wfe\n"							\
	"1:"								\
	: [tmp] "=&r" (tmp), [v] "+Q" (*(u64 *)ptr)		\
	: [val] "r" (val));						\
}

__CMPWAIT_CASE(w, b, 8);
__CMPWAIT_CASE(w, h, 16);
__CMPWAIT_CASE(w,  , 32);
__CMPWAIT_CASE( ,  , 64);

#undef __CMPWAIT_CASE

#define __CMPWAIT_GEN(sfx)						\
static inline void __cmpwait##sfx(volatile void *ptr,			\
				  u64 val,			\
				  int size)				\
{									\
	switch (size) {							\
	case 1:								\
		return __cmpwait_case##sfx##_8(ptr, (u8)val);		\
	case 2:								\
		return __cmpwait_case##sfx##_16(ptr, (u16)val);		\
	case 4:								\
		return __cmpwait_case##sfx##_32(ptr, val);		\
	case 8:								\
		return __cmpwait_case##sfx##_64(ptr, val);		\
	default:							\
		BUILD_BUG();						\
	}								\
									\
	unreachable();							\
}

__CMPWAIT_GEN()

#undef __CMPWAIT_GEN

#define __cmpwait_relaxed(ptr, val) \
	__cmpwait((ptr), (u64)(val), sizeof(*(ptr)))

#endif /* !__ASM_CMPXCHG_H_ */
