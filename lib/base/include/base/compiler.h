#ifndef __BASE_COMPILER_H_
#define __BASE_COMPILER_H_

#include <base/compiler_types.h>

#ifndef __ASSEMBLY__

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

/* Optimization barrier */
#ifndef barrier
#define barrier() __memory_barrier()
#endif

#ifndef barrier_data
#define barrier_data(ptr) barrier()
#endif

/* workaround for GCC PR82365 if needed */
#ifndef barrier_before_unreachable
#define barrier_before_unreachable() do { } while (0)
#endif

#ifndef __always_inline
#define __always_inline inline
#endif

#define annotate_reachable()
#define annotate_unreachable()

#ifndef ASM_UNREACHABLE
#define ASM_UNREACHABLE
#endif

#ifndef unreachable
#define unreachable() do {		\
	annotate_unreachable();		\
	__builtin_unreachable();	\
} while (0)
#endif

#ifndef RELOC_HIDE
#define RELOC_HIDE(ptr, off)					\
  ({ unsigned long long __ptr;					\
     __ptr = (unsigned long long) (ptr);				\
    (typeof(ptr)) (__ptr + (off)); })
#endif

#ifndef OPTIMIZER_HIDE_VAR
/* Make the optimizer believe the variable can be manipulated arbitrarily. */
#define OPTIMIZER_HIDE_VAR(var)						\
	__asm__ ("" : "=r" (var) : "0" (var))
#endif

/* Not-quite-unique ID. */
#ifndef __UNIQUE_ID
#define __UNIQUE_ID(prefix) __PASTE(__PASTE(__UNIQUE_ID_, prefix), __LINE__)
#endif

#include <asm/base/types.h>
#include <asm/base/barrier.h>

static __always_inline
void __read_once_size(const volatile void *p, void *res, __s32 size)
{
	switch (size) {							\
	case 1: *(__u8 *)res = *(volatile __u8 *)p; break;		\
	case 2: *(__u16 *)res = *(volatile __u16 *)p; break;		\
	case 4: *(__u32 *)res = *(volatile __u32 *)p; break;		\
	case 8: *(__u64 *)res = *(volatile __u64 *)p; break;		\
	default:							\
		barrier();						\
		__builtin_memcpy((void *)res, (const void *)p, size);	\
		barrier();						\
	}			
}

#define __READ_ONCE(x)						\
({									\
	union { typeof(x) __val; char __c[1]; } __u;			\
	__read_once_size(&(x), __u.__c, sizeof(x));		\
	smp_read_barrier_depends(); /* Enforce dependency ordering from x */ \
	__u.__val;							\
})
#define READ_ONCE(x) __READ_ONCE(x)

static __always_inline
unsigned long long read_word_at_a_time(const void *addr)
{
	return *(unsigned long long *)addr;
}

static __always_inline
void __write_once_size(volatile void *p, void *res, __s32 size)
{
	switch (size) {
	case 1: *(volatile __u8 *)p = *(__u8 *)res; break;
	case 2: *(volatile __u16 *)p = *(__u16 *)res; break;
	case 4: *(volatile __u32 *)p = *(__u32 *)res; break;
	case 8: *(volatile __u64 *)p = *(__u64 *)res; break;
	default:
		barrier();
		__builtin_memcpy((void *)p, (const void *)res, size);
		barrier();
	}
}

#define __WRITE_ONCE(x, val) \
({							\
	union { typeof(x) __val; char __c[1]; } __u =	\
		{ .__val = (__force typeof(x)) (val) }; \
	__write_once_size(&(x), __u.__c, sizeof(x));	\
	__u.__val;					\
})
#define WRITE_ONCE(x, val) __WRITE_ONCE(x, val)

/*
 * Force the compiler to emit 'sym' as a symbol, so that we can reference
 * it from inline assembler. Necessary in case 'sym' could be inlined
 * otherwise, or eliminated entirely due to lack of references that are
 * visible to the compiler.
 */
#define __ADDRESSABLE(sym) \
	static void * __section(".discard.addressable") __used \
		__PASTE(__addressable_##sym, __LINE__) = (void *)&sym;

/**
 * offset_to_ptr - convert a relative memory offset to an absolute pointer
 * @off:	the address of the 32-bit offset value
 */
static inline void *offset_to_ptr(const __s32 *off)
{
	return (void *)((unsigned long long)off + *off);
}

#endif /* !__ASSEMBLY__ */

/* Compile time object size, -1 for unknown */
#ifndef __compiletime_object_size
#define __compiletime_object_size(obj) -1
#endif
#ifndef __compiletime_warning
#define __compiletime_warning(message)
#endif
#ifndef __compiletime_error
#define __compiletime_error(message)
#endif

#ifdef __OPTIMIZE__
#define __compiletime_assert(condition, msg, prefix, suffix)		\
	do {								\
		extern void prefix ## suffix(void) __compiletime_error(msg); \
		if (!(condition))					\
			prefix ## suffix();				\
	} while (0)
#else
#define __compiletime_assert(condition, msg, prefix, suffix) do { } while (0)
#endif

#define _compiletime_assert(condition, msg, prefix, suffix) \
	__compiletime_assert(condition, msg, prefix, suffix)

/**
 * compiletime_assert - break build and emit msg if condition is false
 * @condition: a compile-time constant condition to check
 * @msg:       a message to emit if condition is false
 *
 * In tradition of POSIX assert, this macro will break the build if the
 * supplied condition is *false*, emitting the supplied error message if the
 * compiler has support to do so.
 */
#define compiletime_assert(condition, msg) \
	_compiletime_assert(condition, msg, __compiletime_assert_, __LINE__)

#define compiletime_assert_atomic_type(t)				\
	compiletime_assert(__native_word(t),				\
		"Need native word sized stores/loads for atomicity.")

/* &a[0] degrades to a pointer: a different type from an array */
#define __must_be_array(a)	BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))

#endif /* !__BASE_COMPILER_H_ */
