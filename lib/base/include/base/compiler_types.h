/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BASE_COMPILER_TYPES_H_
#define __BASE_COMPILER_TYPES_H_

#ifndef __ASSEMBLY__

#define __must_check

#define __user
#define __kernel
#define __safe
#define __force
#define __nocast
#define __iomem
#define __chk_user_ptr(x) (void)0
#define __chk_io_ptr(x) (void)0
#define __builtin_warning(x, y...) (1)
#define __must_hold(x)
#define __acquires(x)
#define __releases(x)
#define __acquire(x) (void)0
#define __release(x) (void)0
#define __cond_lock(x,c) (c)
#define __percpu
#define __rcu
#define __private
#define ACCESS_PRIVATE(p, member) ((p)->member)

/* Indirect macros required for expanded argument pasting, eg. __LINE__. */
#define ___PASTE(a,b) a##b
#define __PASTE(a,b) ___PASTE(a,b)

/* Attributes */
#include <base/compiler_attributes.h>

#ifdef __KERNEL__

/* Compiler specific macros. */
#ifdef __clang__
#include <base/compiler-clang.h>
#elif defined(__GNUC__)
/* The above compilers also define __GNUC__, so order is important here. */
#include <base/compiler-gcc.h>
#else
#error "Unknown compiler"
#endif

/*
 * Some architectures need to provide custom definitions of macros provided
 * by linux/compiler-*.h, and can do so using asm/compiler.h. We include that
 * conditionally rather than using an asm-generic wrapper in order to avoid
 * build failures if any C compilation, which will include this file via an
 * -include argument in c_flags, occurs prior to the asm-generic wrappers being
 * generated.
 */
#ifdef CONFIG_HAVE_ARCH_COMPILER_H
#include <asm/base/compiler.h>
#endif

#if defined(CC_USING_HOTPATCH)
#define notrace			__attribute__((hotpatch(0, 0)))
#else
#define notrace			__attribute__((__no_instrument_function__))
#endif

/*
 * it doesn't make sense on ARM (currently the only user of __naked)
 * to trace naked functions because then mcount is called without
 * stack and frame pointer being set up and there is no chance to
 * restore the lr register to the value before mcount was called.
 */
#define __naked			__attribute__((__naked__)) notrace

#define __compiler_offsetof(a, b)	__builtin_offsetof(a, b)

/*
 * Force always-inline if the user requests it so via the .config.
 * GCC does not warn about unused static inline functions for
 * -Wunused-function.  This turns out to avoid the need for complex #ifdef
 * directives.  Suppress the warning in clang as well by using "unused"
 * function attribute, which is redundant but not harmful for gcc.
 * Prefer gnu_inline, so that extern inline functions do not emit an
 * externally visible function. This makes extern inline behave as per gnu89
 * semantics rather than c99. This prevents multiple symbol definition errors
 * of extern inline functions at link time.
 * A lot of inline functions can cause havoc with function tracing.
 * Do not use __always_inline here, since currently it expands to inline again
 * (which would break users of __always_inline).
 */
#if !defined(CONFIG_ARCH_SUPPORTS_OPTIMIZED_INLINING) || \
	!defined(CONFIG_OPTIMIZE_INLINING)
#define inline inline __attribute__((__always_inline__)) __gnu_inline \
	__maybe_unused notrace
#else
#define inline inline                                    __gnu_inline \
	__maybe_unused notrace
#endif

#define __inline__ inline
#define __inline   inline

/*
 * Rather then using noinline to prevent stack consumption, use
 * noinline_for_stack instead.  For documentation reasons.
 */
#define noinline_for_stack noinline

#endif /* __KERNEL__ */

#endif /* !__ASSEMBLY__ */

/*
 * The below symbols may be defined for one or more, but not ALL, of the above
 * compilers. We don't consider that to be an error, so set them to nothing.
 * For example, some of them are for compiler specific plugins.
 */
#ifndef __latent_entropy
#define __latent_entropy
#endif

#ifndef __randomize_layout
#define __randomize_layout __designated_init
#endif

#ifndef __no_randomize_layout
#define __no_randomize_layout
#endif

#ifndef randomized_struct_fields_start
#define randomized_struct_fields_start
#define randomized_struct_fields_end
#endif

#ifndef asm_volatile_goto
#define asm_volatile_goto(x...) asm goto(x)
#endif

/* Are two types/vars the same type (ignoring qualifiers)? */
#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

/* Is this type a native word size -- useful for atomic operations */
#define __native_word(t) \
	(sizeof(t) == sizeof(char) || sizeof(t) == sizeof(short) || \
	 sizeof(t) == sizeof(int) || sizeof(t) == sizeof(long long))

/* Helpers for emitting diagnostics in pragmas. */
#ifndef __diag
#define __diag(string)
#endif

#ifndef __diag_GCC
#define __diag_GCC(version, severity, string)
#endif

#define __diag_push()	__diag(push)
#define __diag_pop()	__diag(pop)

#define __diag_ignore(compiler, version, option, comment) \
	__diag_ ## compiler(version, ignore, option)
#define __diag_warn(compiler, version, option, comment) \
	__diag_ ## compiler(version, warn, option)
#define __diag_error(compiler, version, option, comment) \
	__diag_ ## compiler(version, error, option)

#endif /* !__BASE_COMPILER_TYPES_H_ */