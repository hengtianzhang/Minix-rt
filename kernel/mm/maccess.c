/*
 * Access kernel memory without faulting.
 */
#include <minix_rt/mm.h>
#include <minix_rt/uaccess.h>

#include <asm/processor.h>

/**
 * probe_kernel_read(): safely attempt to read from a location
 * @dst: pointer to the buffer that shall take the data
 * @src: address to read from
 * @size: size of the data chunk
 *
 * Safely read from address @src to the buffer at @dst.  If a kernel fault
 * happens, handle that and return -EFAULT.
 *
 * We ensure that the copy_from_user is executed in atomic context so that
 * do_page_fault() doesn't attempt to take mmap_sem.  This makes
 * probe_kernel_read() suitable for use within regions where the caller
 * already holds mmap_sem, or other locks which nest inside mmap_sem.
 */
long __weak probe_kernel_read(void *dst, const void *src, size_t size)
    __attribute__((alias("__probe_kernel_read")));

long __probe_kernel_read(void *dst, const void *src, size_t size)
{
	long ret;
	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);
	ret = __copy_from_user_inatomic(dst,
			(__force const void __user *)src, size);
	set_fs(old_fs);

	return ret ? -EFAULT : 0;
}
