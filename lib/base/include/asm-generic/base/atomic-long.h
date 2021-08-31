/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_BASE_ATOMIC_LONG_H_
#define __ASM_GENERIC_BASE_ATOMIC_LONG_H_
/*
 * Copyright (C) 2005 Silicon Graphics, Inc.
 *	Christoph Lameter
 *
 * Allows to provide arch independent atomic definitions without the need to
 * edit all arch specific atomic.h files.
 */

#include <asm/base/types.h>

/*
 * Suppport for atomic_long_t
 *
 * Casts for parameters are avoided for existing atomic functions in order to
 * avoid issues with cast-as-lval under gcc 4.x and other limitations that the
 * macros of a platform may have.
 */

#if BITS_PER_LONG == 64

typedef atomic64_t atomic_long_t;

#define ATOMIC_LONG_INIT(i)	ATOMIC64_INIT(i)
#define ATOMIC_LONG_PFX(x)	atomic64 ## x
#define ATOMIC_LONG_TYPE	s64

#else

typedef atomic_t atomic_long_t;

#define ATOMIC_LONG_INIT(i)	ATOMIC_INIT(i)
#define ATOMIC_LONG_PFX(x)	atomic ## x
#define ATOMIC_LONG_TYPE	int

#endif

#define ATOMIC_LONG_READ_OP(mo)						\
static inline s64 atomic_long_read##mo(const atomic_long_t *l)		\
{									\
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;		\
									\
	return (s64)ATOMIC_LONG_PFX(_read##mo)(v);			\
}
ATOMIC_LONG_READ_OP()
ATOMIC_LONG_READ_OP(_acquire)

#undef ATOMIC_LONG_READ_OP

#define ATOMIC_LONG_SET_OP(mo)						\
static inline void atomic_long_set##mo(atomic_long_t *l, s64 i)	\
{									\
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;		\
									\
	ATOMIC_LONG_PFX(_set##mo)(v, i);				\
}
ATOMIC_LONG_SET_OP()
ATOMIC_LONG_SET_OP(_release)

#undef ATOMIC_LONG_SET_OP

#define ATOMIC_LONG_ADD_SUB_OP(op, mo)					\
static inline s64							\
atomic_long_##op##_return##mo(s64 i, atomic_long_t *l)			\
{									\
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;		\
									\
	return (s64)ATOMIC_LONG_PFX(_##op##_return##mo)(i, v);		\
}
ATOMIC_LONG_ADD_SUB_OP(add,)
ATOMIC_LONG_ADD_SUB_OP(add, _relaxed)
ATOMIC_LONG_ADD_SUB_OP(add, _acquire)
ATOMIC_LONG_ADD_SUB_OP(add, _release)
ATOMIC_LONG_ADD_SUB_OP(sub,)
ATOMIC_LONG_ADD_SUB_OP(sub, _relaxed)
ATOMIC_LONG_ADD_SUB_OP(sub, _acquire)
ATOMIC_LONG_ADD_SUB_OP(sub, _release)

#undef ATOMIC_LONG_ADD_SUB_OP

#define atomic_long_cmpxchg_relaxed(l, old, new) \
	(ATOMIC_LONG_PFX(_cmpxchg_relaxed)((ATOMIC_LONG_PFX(_t) *)(l), \
					   (old), (new)))
#define atomic_long_cmpxchg_acquire(l, old, new) \
	(ATOMIC_LONG_PFX(_cmpxchg_acquire)((ATOMIC_LONG_PFX(_t) *)(l), \
					   (old), (new)))
#define atomic_long_cmpxchg_release(l, old, new) \
	(ATOMIC_LONG_PFX(_cmpxchg_release)((ATOMIC_LONG_PFX(_t) *)(l), \
					   (old), (new)))
#define atomic_long_cmpxchg(l, old, new) \
	(ATOMIC_LONG_PFX(_cmpxchg)((ATOMIC_LONG_PFX(_t) *)(l), (old), (new)))


#define atomic_long_try_cmpxchg_relaxed(l, old, new) \
	(ATOMIC_LONG_PFX(_try_cmpxchg_relaxed)((ATOMIC_LONG_PFX(_t) *)(l), \
					   (ATOMIC_LONG_TYPE *)(old), (ATOMIC_LONG_TYPE)(new)))
#define atomic_long_try_cmpxchg_acquire(l, old, new) \
	(ATOMIC_LONG_PFX(_try_cmpxchg_acquire)((ATOMIC_LONG_PFX(_t) *)(l), \
					   (ATOMIC_LONG_TYPE *)(old), (ATOMIC_LONG_TYPE)(new)))
#define atomic_long_try_cmpxchg_release(l, old, new) \
	(ATOMIC_LONG_PFX(_try_cmpxchg_release)((ATOMIC_LONG_PFX(_t) *)(l), \
					   (ATOMIC_LONG_TYPE *)(old), (ATOMIC_LONG_TYPE)(new)))
#define atomic_long_try_cmpxchg(l, old, new) \
	(ATOMIC_LONG_PFX(_try_cmpxchg)((ATOMIC_LONG_PFX(_t) *)(l), \
				       (ATOMIC_LONG_TYPE *)(old), (ATOMIC_LONG_TYPE)(new)))


#define atomic_long_xchg_relaxed(v, new) \
	(ATOMIC_LONG_PFX(_xchg_relaxed)((ATOMIC_LONG_PFX(_t) *)(v), (new)))
#define atomic_long_xchg_acquire(v, new) \
	(ATOMIC_LONG_PFX(_xchg_acquire)((ATOMIC_LONG_PFX(_t) *)(v), (new)))
#define atomic_long_xchg_release(v, new) \
	(ATOMIC_LONG_PFX(_xchg_release)((ATOMIC_LONG_PFX(_t) *)(v), (new)))
#define atomic_long_xchg(v, new) \
	(ATOMIC_LONG_PFX(_xchg)((ATOMIC_LONG_PFX(_t) *)(v), (new)))

static __always_inline void atomic_long_inc(atomic_long_t *l)
{
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;

	ATOMIC_LONG_PFX(_inc)(v);
}

static __always_inline void atomic_long_dec(atomic_long_t *l)
{
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;

	ATOMIC_LONG_PFX(_dec)(v);
}

#define ATOMIC_LONG_FETCH_OP(op, mo)					\
static inline s64							\
atomic_long_fetch_##op##mo(s64 i, atomic_long_t *l)			\
{									\
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;		\
									\
	return (s64)ATOMIC_LONG_PFX(_fetch_##op##mo)(i, v);		\
}

ATOMIC_LONG_FETCH_OP(add, )
ATOMIC_LONG_FETCH_OP(add, _relaxed)
ATOMIC_LONG_FETCH_OP(add, _acquire)
ATOMIC_LONG_FETCH_OP(add, _release)
ATOMIC_LONG_FETCH_OP(sub, )
ATOMIC_LONG_FETCH_OP(sub, _relaxed)
ATOMIC_LONG_FETCH_OP(sub, _acquire)
ATOMIC_LONG_FETCH_OP(sub, _release)
ATOMIC_LONG_FETCH_OP(and, )
ATOMIC_LONG_FETCH_OP(and, _relaxed)
ATOMIC_LONG_FETCH_OP(and, _acquire)
ATOMIC_LONG_FETCH_OP(and, _release)
ATOMIC_LONG_FETCH_OP(andnot, )
ATOMIC_LONG_FETCH_OP(andnot, _relaxed)
ATOMIC_LONG_FETCH_OP(andnot, _acquire)
ATOMIC_LONG_FETCH_OP(andnot, _release)
ATOMIC_LONG_FETCH_OP(or, )
ATOMIC_LONG_FETCH_OP(or, _relaxed)
ATOMIC_LONG_FETCH_OP(or, _acquire)
ATOMIC_LONG_FETCH_OP(or, _release)
ATOMIC_LONG_FETCH_OP(xor, )
ATOMIC_LONG_FETCH_OP(xor, _relaxed)
ATOMIC_LONG_FETCH_OP(xor, _acquire)
ATOMIC_LONG_FETCH_OP(xor, _release)

#undef ATOMIC_LONG_FETCH_OP

#define ATOMIC_LONG_FETCH_INC_DEC_OP(op, mo)					\
static inline s64							\
atomic_long_fetch_##op##mo(atomic_long_t *l)				\
{									\
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;		\
									\
	return (s64)ATOMIC_LONG_PFX(_fetch_##op##mo)(v);		\
}

ATOMIC_LONG_FETCH_INC_DEC_OP(inc,)
ATOMIC_LONG_FETCH_INC_DEC_OP(inc, _relaxed)
ATOMIC_LONG_FETCH_INC_DEC_OP(inc, _acquire)
ATOMIC_LONG_FETCH_INC_DEC_OP(inc, _release)
ATOMIC_LONG_FETCH_INC_DEC_OP(dec,)
ATOMIC_LONG_FETCH_INC_DEC_OP(dec, _relaxed)
ATOMIC_LONG_FETCH_INC_DEC_OP(dec, _acquire)
ATOMIC_LONG_FETCH_INC_DEC_OP(dec, _release)

#undef ATOMIC_LONG_FETCH_INC_DEC_OP

#define ATOMIC_LONG_OP(op)						\
static __always_inline void						\
atomic_long_##op(s64 i, atomic_long_t *l)				\
{									\
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;		\
									\
	ATOMIC_LONG_PFX(_##op)(i, v);					\
}

ATOMIC_LONG_OP(add)
ATOMIC_LONG_OP(sub)
ATOMIC_LONG_OP(and)
ATOMIC_LONG_OP(andnot)
ATOMIC_LONG_OP(or)
ATOMIC_LONG_OP(xor)

#undef ATOMIC_LONG_OP

static inline int atomic_long_sub_and_test(s64 i, atomic_long_t *l)
{
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;

	return ATOMIC_LONG_PFX(_sub_and_test)(i, v);
}

static inline int atomic_long_dec_and_test(atomic_long_t *l)
{
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;

	return ATOMIC_LONG_PFX(_dec_and_test)(v);
}

static inline int atomic_long_inc_and_test(atomic_long_t *l)
{
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;

	return ATOMIC_LONG_PFX(_inc_and_test)(v);
}

static inline int atomic_long_add_negative(s64 i, atomic_long_t *l)
{
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;

	return ATOMIC_LONG_PFX(_add_negative)(i, v);
}

#define ATOMIC_LONG_INC_DEC_OP(op, mo)					\
static inline s64							\
atomic_long_##op##_return##mo(atomic_long_t *l)				\
{									\
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;		\
									\
	return (s64)ATOMIC_LONG_PFX(_##op##_return##mo)(v);		\
}
ATOMIC_LONG_INC_DEC_OP(inc,)
ATOMIC_LONG_INC_DEC_OP(inc, _relaxed)
ATOMIC_LONG_INC_DEC_OP(inc, _acquire)
ATOMIC_LONG_INC_DEC_OP(inc, _release)
ATOMIC_LONG_INC_DEC_OP(dec,)
ATOMIC_LONG_INC_DEC_OP(dec, _relaxed)
ATOMIC_LONG_INC_DEC_OP(dec, _acquire)
ATOMIC_LONG_INC_DEC_OP(dec, _release)

#undef ATOMIC_LONG_INC_DEC_OP

static inline s64 atomic_long_add_unless(atomic_long_t *l, s64 a, s64 u)
{
	ATOMIC_LONG_PFX(_t) *v = (ATOMIC_LONG_PFX(_t) *)l;

	return (s64)ATOMIC_LONG_PFX(_add_unless)(v, a, u);
}

#define atomic_long_inc_not_zero(l) \
	ATOMIC_LONG_PFX(_inc_not_zero)((ATOMIC_LONG_PFX(_t) *)(l))

#define atomic_long_cond_read_relaxed(v, c) \
	ATOMIC_LONG_PFX(_cond_read_relaxed)((ATOMIC_LONG_PFX(_t) *)(v), (c))
#define atomic_long_cond_read_acquire(v, c) \
	ATOMIC_LONG_PFX(_cond_read_acquire)((ATOMIC_LONG_PFX(_t) *)(v), (c))

#endif /* !__ASM_GENERIC_BASE_ATOMIC_LONG_H_ */
