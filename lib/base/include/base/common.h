#ifndef __BASE_COMMON_H_
#define __BASE_COMMON_H_

#include <stdarg.h>
#include <base/compiler.h>
#include <base/linkage.h>
#include <base/bug.h>

__printf(1, 2)
void hang(const char *fmt, ...) __noreturn __cold;

asmlinkage __printf(1, 2) __cold
int printf(const char *fmt, ...);

#endif /* !__BASE_COMMON_H_ */
