/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BASE_BUG_H_
#define __BASE_BUG_H_

#include <base/compiler.h>

/* Force a compilation error if a constant expression is not a power of 2 */
#define BUILD_BUG_ON_NOT_POWER_OF_2(n)			\
	BUILD_BUG_ON((n) == 0 || (((n) & ((n) - 1)) != 0))

/* Force a compilation error if condition is true, but also produce a
   result (of value 0 and type size_t), so the expression can be used
   e.g. in a structure initializer (or where-ever else comma expressions
   aren't permitted). */
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define BUILD_BUG_ON_NULL(e) ((void *)sizeof(struct { int:-!!(e); }))

/*
 * BUILD_BUG_ON_INVALID() permits the compiler to check the validity of the
 * expression but avoids the generation of any code, even if that expression
 * has side-effects.
 */
#define BUILD_BUG_ON_INVALID(e) ((void)(sizeof((__force s64)(e))))

/**
 * BUILD_BUG_ON_MSG - break compile if a condition is true & emit supplied
 *		      error message.
 * @condition: the condition which the compiler should know is false.
 *
 * See BUILD_BUG_ON for description.
 */
#define BUILD_BUG_ON_MSG(cond, msg) compiletime_assert(!(cond), msg)

/**
 * BUILD_BUG_ON - break compile if a condition is true.
 * @condition: the condition which the compiler should know is false.
 *
 * If you have some code which relies on certain constants being equal, or
 * some other compile-time-evaluated condition, you should use BUILD_BUG_ON to
 * detect if someone changes it.
 *
 * The implementation uses gcc's reluctance to create a negative array, but gcc
 * (as of 4.4) only emits that error for obvious cases (e.g. not arguments to
 * inline functions).  Luckily, in 4.3 they added the "error" function
 * attribute just for this type of case.  Thus, we use a negative sized array
 * (should always create an error on gcc versions older than 4.4) and then call
 * an undefined function with the error attribute (should always create an
 * error on gcc 4.3 and later).  If for some reason, neither creates a
 * compile-time error, we'll still have a link-time error, which is harder to
 * track down.
 */
#ifndef __OPTIMIZE__
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#else
#define BUILD_BUG_ON(condition) \
	BUILD_BUG_ON_MSG(condition, "BUILD_BUG_ON failed: " #condition)
#endif

/**
 * BUILD_BUG - break compile if used.
 *
 * If you have some code that you expect the compiler to eliminate at
 * build time, you should use BUILD_BUG to detect if it is
 * unexpectedly used.
 */
#define BUILD_BUG() BUILD_BUG_ON_MSG(1, "BUILD_BUG failed")

#ifndef __ASSEMBLY__

#define BUG() do { \
	printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
	barrier_before_unreachable();	\
	hang("BUG!"); \
} while (0)

#define BUG_ON(condition) do { if (unlikely(condition)) BUG(); } while (0)

#define __WARN() do { \
	printf("WARNING: warn at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); } while (0)
#define __WARN_printf(arg...)	do { printf(arg); __WARN(); } while (0)

#ifndef WARN_ON
#define WARN_ON(condition) ({						\
	int __ret_warn_on = !!(condition);				\
	if (unlikely(__ret_warn_on))					\
		__WARN();						\
	unlikely(__ret_warn_on);					\
})
#endif

#ifndef WARN
#define WARN(condition, format...) ({						\
	int __ret_warn_on = !!(condition);				\
	if (unlikely(__ret_warn_on))					\
		__WARN_printf(format);					\
	unlikely(__ret_warn_on);					\
})
#endif

#define WARN_ON_ONCE(condition)	({				\
	static bool __section(.data.once) __warned;		\
	int __ret_warn_once = !!(condition);			\
								\
	if (unlikely(__ret_warn_once))				\
		if (WARN_ON(!__warned)) 			\
			__warned = true;			\
	unlikely(__ret_warn_once);				\
})

#define WARN_ONCE(condition, format...)	({			\
	static bool __section(.data.once) __warned;		\
	int __ret_warn_once = !!(condition);			\
								\
	if (unlikely(__ret_warn_once))				\
		if (WARN(!__warned, format)) 			\
			__warned = true;			\
	unlikely(__ret_warn_once);				\
})

/*
 * Since detected data corruption should stop operation on the affected
 * structures. Return value must be checked and sanely acted on by caller.
 */
static inline bool check_data_corruption(bool v) { return v; }
#define CHECK_DATA_CORRUPTION(condition, fmt, ...)			 \
	check_data_corruption(({					 \
		bool corruption = unlikely(condition);			 \
		if (corruption) {					 \
			WARN(1, fmt, ##__VA_ARGS__);		 \
		}							 \
		corruption;						 \
	}))

#endif /* !__ASSEMBLY__ */
#endif /* !__BASE_BUG_H_ */
