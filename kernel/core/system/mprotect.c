#include <minix_rt/system.h>
#include <minix_rt/sched.h>
#include <minix_rt/page.h>
#include <minix_rt/mmap.h>
#include <minix_rt/syscalls.h>

SYSCALL_DEFINE3(mprotect, unsigned long, start, size_t, len,
		unsigned long, prot)
{
	int ret;
	message_t m;

	if (len < PAGE_SIZE || !PAGE_ALIGNED(len) || !PAGE_ALIGNED(start))
		return -EINVAL;

	memset(&m, 0, sizeof (message_t));

	m.m_type = IPC_M_TYPE_SYSTEM_MPROTECT;
	m.m_sys_mprotect.start = start;
	m.m_sys_mprotect.len = len;
	m.m_sys_mprotect.prot = prot;

	ret = __ipc_send(ENDPOINT_SYSTEM, &m);
	if (ret)
		return ret;

	return m.m_sys_mprotect.retval;
}

void system_mprotect(endpoint_t ep, message_t *m)
{
	struct task_struct *tsk;
	struct vm_area_struct *vma;
	int ret;

	tsk = pid_find_process_by_pid(m->m_source);
	BUG_ON(!tsk);
	vma = mmap_find_vma_area(m->m_sys_mprotect.start, tsk->mm);
	if (!vma) {
		m->m_sys_mprotect.retval = -EINVAL;
		return;
	}

	ret = mmap_mprotect_fixup(vma, m->m_sys_mprotect.start,
				m->m_sys_mprotect.start + m->m_sys_mprotect.len,
				m->m_sys_mprotect.prot);
	m->m_sys_mprotect.retval = ret;
}
