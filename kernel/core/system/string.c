#include <minix_rt/system.h>
#include <minix_rt/sched.h>
#include <minix_rt/page.h>
#include <minix_rt/mmap.h>
#include <minix_rt/slab.h>

static int message_memcpy(const void *dst, const void *src,
						int size, pid_t dst_pid, pid_t src_pid)
{
	int ret = 0;
	struct task_struct *dst_tsk, *src_tsk;
	void *tmp_buf;

	dst_tsk = pid_find_process_by_pid(dst_pid);
	if (!dst_tsk) {
		ret = -ESRCH;
		goto out;
	}

	src_tsk = pid_find_process_by_pid(src_pid);
	if (!src_tsk) {
		ret = -ESRCH;
		goto out;
	}

	if (!size) {
		ret = -EINVAL;
		goto out;
	}

	tmp_buf = kmalloc(size, GFP_KERNEL | GFP_ZERO);
	if (!tmp_buf) {
		ret = -ENOMEM;
		goto out;
	}

	ret = mmap_memcpy_from_vma(tmp_buf, (unsigned long)src, size, src_tsk);
	if (ret != size) {
		ret = -EINVAL;
		goto free_out;
	}

	ret = mmap_memcpy_to_vma((unsigned long)dst, size, tmp_buf, dst_tsk);
	if (ret != size) {
		ret = -EINVAL;
		goto free_out;
	}

free_out:
	kfree(tmp_buf);
out:
	return ret;
}

void system_string(endpoint_t ep, message_t *m)
{
	int ret;
	int direct = m->m_sys_string.direct;
	struct task_struct *tsk;

	tsk = pid_find_process_by_pid(m->m_source);
	BUG_ON(!tsk);
	if (unlikely(!(tsk->flags & PF_SYSTEMSERVICE))) {
		m->m_sys_string.retval = -EACCES;
		return ;
	}

	switch (direct) {
		case DIRECT_CPY_FROM:
			ret = message_memcpy(m->m_sys_string.dest, m->m_sys_string.src,
						m->m_sys_string.size, m->m_source, m->m_sys_string.pid);
			m->m_sys_string.retval = ret;
			break;
		case DIRECT_CPY_TO:
			ret = message_memcpy(m->m_sys_string.dest, m->m_sys_string.src,
						m->m_sys_string.size, m->m_sys_string.pid, m->m_source);
			m->m_sys_string.retval = ret;
			break;
		default:
			break;
	}
}
