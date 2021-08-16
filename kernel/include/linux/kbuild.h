#ifndef __LINUX_KBUILD_H_
#define __LINUX_KBUILD_H_

#include <linux/stddef.h>

#define DEFINE(sym, val) \
        asm volatile("\n->" #sym " %0 " #val : : "i" (val))

#define BLANK() asm volatile("\n->" : : )

#define OFFSET(sym, str, mem) \
	DEFINE(sym, offsetof(struct str, mem))

#define COMMENT(x) \
	asm volatile("\n->#" x)

#endif /* !__LINUX_KBUILD_H_ */
