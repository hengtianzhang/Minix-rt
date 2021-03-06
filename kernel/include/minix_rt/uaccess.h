/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MINIX_RT_UACCESS_H_
#define __MINIX_RT_UACCESS_H_

#include <base/string.h>
#include <base/common.h>

#include <minix_rt/thread.h>

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

#define BAD_ADDR(x) ((unsigned long)(x) >= TASK_SIZE)

/*
 * probe_kernel_read(): safely attempt to read from a location
 * @dst: pointer to the buffer that shall take the data
 * @src: address to read from
 * @size: size of the data chunk
 *
 * Safely read from address @src to the buffer at @dst.  If a kernel fault
 * happens, handle that and return -EFAULT.
 */
extern long probe_kernel_read(void *dst, const void *src, size_t size);
extern long __probe_kernel_read(void *dst, const void *src, size_t size);

static __always_inline unsigned long
__copy_from_user_inatomic(void *to, const void __user *from, unsigned long n)
{
	return raw_copy_from_user(to, from, n);
}

/**
 * probe_kernel_address(): safely attempt to read from a location
 * @addr: address to read from
 * @retval: read into this variable
 *
 * Returns 0 on success, or -EFAULT.
 */
#define probe_kernel_address(addr, retval)		\
	probe_kernel_read(&retval, addr, sizeof(retval))

#endif /* !__MINIX_RT_UACCESS_H_ */
