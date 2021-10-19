#include <string.h>

#include <libminix_rt/syscalls.h>
#include <libminix_rt/ipc.h>

int message_memcpy(void *dst, const void *src, int size, pid_t src_pid)
{
	int ret;
	message_t m;

	if (!dst || !src || !size || src_pid < 0)
		return -EINVAL;

	memset(&m, 0, sizeof (message_t));

	m.m_type = IPC_M_TYPE_SYSTEM_STRING;
	m.m_sys_string.direct = DIRECT_CPY_FROM;
	m.m_sys_string.pid = src_pid;
	m.m_sys_string.dest = dst;
	m.m_sys_string.src = src;
	m.m_sys_string.size = size;

	ret = ipc_send(ENDPOINT_SYSTEM, &m);
	if (ret)
		return ret;

	return m.m_sys_string.retval;
}
