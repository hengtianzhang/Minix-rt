#include <minix_rt/system.h>
#include <minix_rt/syscalls.h>
#include <minix_rt/slab.h>
#include <minix_rt/sched.h>

#include <uapi/minix_rt/uio.h>

extern void syscall_printf(const char *buf);

SYSCALL_DEFINE3(write, unsigned int, fd, const char __user *, buf,
		size_t, count)
{
	if (fd <= 2) {
		void *str = kmalloc(count, GFP_KERNEL | GFP_ZERO);
		if (!str)
			return -ENOMEM;
		if (copy_from_user(str, buf, count)) {
			kfree(str);
			return -EINVAL;
		}
		syscall_printf((const char *)str);
		kfree(str);
		return count;
	} else {
		/* TODO */
		printf("write fd 0x%x\n", fd);
		while (1);
	}

	return 0;
}

SYSCALL_DEFINE3(writev, unsigned long, fd, const struct iovec __user *, vec,
		unsigned long, vlen)
{
	int i;
	ssize_t size = 0;

	if (fd <= 2) {
		for (i = 0; i < vlen; i++) {
			void *buf = kmalloc(vec->iov_len, GFP_KERNEL | GFP_ZERO);
			if (!buf)
				return -ENOMEM;
			if (copy_from_user(buf, vec->iov_base, vec->iov_len)) {
				kfree(buf);
				return -EINVAL;
			}
			syscall_printf((const char *)buf);
			kfree(buf);
			size += vec->iov_len;
		}
		return size;
	} else {
		/* TODO */
		printf("writev fd 0x%lx\n", fd);
		while (1);
	}

	return 0;
}
