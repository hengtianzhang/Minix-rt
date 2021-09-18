#ifndef __LIBSEL4M_SYSCALLS_H_
#define __LIBSEL4M_SYSCALLS_H_

#include <base/errno.h>

#include <asm/libsel4m/syscalls.h>

static inline int debug_printf(const char *ptr, int len)
{
	if (!ptr)
		return -EFAULT;

	if (!len)
		return -EINVAL;

	return __debug_printf(ptr, len);
}

#endif /* !__LIBSEL4M_SYSCALLS_H_ */
