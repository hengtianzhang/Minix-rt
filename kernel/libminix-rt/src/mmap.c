#include <string.h>

#include <libminix_rt/syscalls.h>
#include <libminix_rt/ipc.h>
#include <libminix_rt/mmap.h>

int mmap_initrd(unsigned long *mmap_base, unsigned long *size)
{
	int ret;
	message_t m;

	if (!mmap_base || !size)
		return -EINVAL;

	memset(&m, 0, sizeof (message_t));

	m.m_type = IPC_M_TYPE_SYSTEM_MMAP;
	m.m_sys_mmap.vm_flags = VM_READ;
	m.m_sys_mmap.flags = MMAP_INITRD;

	ret = ipc_send(ENDPOINT_SYSTEM, &m);
	if (ret)
		return ret;

	if (m.m_sys_mmap.retval)
		return m.m_sys_mmap.retval;

	*mmap_base = m.m_sys_mmap.mmap_base;
	*size = m.m_sys_mmap.size;

	return 0;
}
