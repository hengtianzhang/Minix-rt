#ifndef __SEL4M_KERNEL_H_
#define __SEL4M_KERNEL_H_

#include <stdarg.h>
#include <sel4m/compiler.h>
#include <sel4m/linkage.h>

__printf(3, 0)
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

__printf(3, 0)
int vscnprintf(char *buf, size_t size, const char *fmt, va_list args);

__printf(1, 2)
void panic(const char *fmt, ...) __noreturn __cold;

asmlinkage __printf(1, 2) __cold
int printf(const char *fmt, ...);

#endif /* !__SEL4M_KERNEL_H_ */
