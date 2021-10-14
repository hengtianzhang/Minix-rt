#include <string.h>

#include <libminix_rt/brk.h>
#include <libminix_rt/syscalls.h>
#include <libminix_rt/ipc.h>

void *sbrk(long increment)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));

	m.m_type = IPC_M_TYPE_SBRK;
	m.m_sys_brk.brk = increment;

	ret = ipc_send(ENDPOINT_SYSTEM, &m);
	if (ret)
		return NULL;

	if (m.m_sys_brk.state == 0)
		return (void *)m.m_sys_brk.brk;
	else
		return NULL;
}

int brk(void *addr)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));

	m.m_type = IPC_M_TYPE_BRK;
	m.m_sys_brk.brk = (unsigned long)addr;
	ret = ipc_send(ENDPOINT_SYSTEM, &m);
	if (ret)
		return -1;

	return m.m_sys_brk.state == 0 ? 0 : -1;
}
