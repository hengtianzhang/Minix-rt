/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_UACCESS_H_
#define __SEL4M_UACCESS_H_

#include <base/string.h>
#include <base/common.h>

#include <sel4m/thread.h>

#include <asm/uaccess.h>

static __always_inline unsigned long
__copy_from_user(void *to, const void __user *from, unsigned long n)
{
	return raw_copy_from_user(to, from, n);
}

static __always_inline unsigned long
__copy_to_user(void __user *to, const void *from, unsigned long n)
{
	return raw_copy_to_user(to, from, n);
}

#ifdef INLINE_COPY_FROM_USER
static inline unsigned long
_copy_from_user(void *to, const void __user *from, unsigned long n)
{
	unsigned long res = n;

	if (likely(access_ok(from, n)))
		res = raw_copy_from_user(to, from, n);

	if (unlikely(res))
		memset(to + (n - res), 0, res);
	return res;
}
#else
extern unsigned long
_copy_from_user(void *, const void __user *, unsigned long);
#endif

#ifdef INLINE_COPY_TO_USER
static inline unsigned long
_copy_to_user(void __user *to, const void *from, unsigned long n)
{
	if (access_ok(to, n))
		n = raw_copy_to_user(to, from, n);

	return n;
}
#else
extern unsigned long
_copy_to_user(void __user *, const void *, unsigned long);
#endif

extern void __compiletime_error("copy source size is too small")
__bad_copy_from(void);
extern void __compiletime_error("copy destination size is too small")
__bad_copy_to(void);

static inline void copy_overflow(int size, unsigned long count)
{
	WARN(1, "Buffer overflow detected (%d < %lu)!\n", size, count);
}

static __always_inline bool
check_copy_size(const void *addr, size_t bytes, bool is_source)
{
	int sz = __compiletime_object_size(addr);
	if (unlikely(sz >= 0 && sz < bytes)) {
		if (!__builtin_constant_p(bytes))
			copy_overflow(sz, bytes);
		else if (is_source)
			__bad_copy_from();
		else
			__bad_copy_to();
		return false;
	}

	return true;
}

static __always_inline unsigned long
copy_from_user(void *to, const void __user *from, unsigned long n)
{
	if (likely(check_copy_size(to, n, false)))
		n = _copy_from_user(to, from, n);
	return n;
}

static __always_inline unsigned long
copy_to_user(void __user *to, const void *from, unsigned long n)
{
	if (likely(check_copy_size(from, n, true)))
		n = _copy_to_user(to, from, n);
	return n;
}

#endif /* !__SEL4M_UACCESS_H_ */
