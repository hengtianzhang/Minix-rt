#ifndef __SEL4M_KERNEL_H_
#define __SEL4M_KERNEL_H_

#include <stdarg.h>
#include <sel4m/compiler.h>
#include <sel4m/linkage.h>

/*
 * 'kernel.h' contains some often-used function prototypes etc
 */
#define __ALIGN_KERNEL(x, a)		__ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask)	(((x) + (mask)) & ~(mask))

/* @a is a power of 2 value */
#define ALIGN(x, a)		__ALIGN_KERNEL((x), (a))
#define ALIGN_DOWN(x, a)	__ALIGN_KERNEL((x) - ((a) - 1), (a))

__printf(3, 0)
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

__printf(3, 0)
int vscnprintf(char *buf, size_t size, const char *fmt, va_list args);

__printf(1, 2)
void panic(const char *fmt, ...) __noreturn __cold;

asmlinkage __printf(1, 2) __cold
int printf(const char *fmt, ...);

#endif /* !__SEL4M_KERNEL_H_ */
