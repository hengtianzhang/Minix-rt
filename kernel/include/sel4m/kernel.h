#ifndef __SEL4M_KERNEL_H_
#define __SEL4M_KERNEL_H_

#include <base/common.h>

__printf(3, 0)
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

__printf(3, 0)
int vscnprintf(char *buf, size_t size, const char *fmt, va_list args);

__printf(1, 2)
void panic(const char *fmt, ...) __noreturn __cold;

#endif /* !__SEL4M_KERNEL_H_ */
