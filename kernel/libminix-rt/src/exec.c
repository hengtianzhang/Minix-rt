#include <string.h>
#include <errno.h>

#include <minix_rt/binfmt.h>

#include <libminix_rt/exec.h>
#include <libminix_rt/syscalls.h>
#include <libminix_rt/ipc.h>

int execve(struct minix_rt_binprm *bprm)
{
	int ret;
	message_t m;

	if (!bprm)
		return -EINVAL;

	memset(&m, 0, sizeof (message_t));

	m.m_type = IPC_M_TYPE_SYSTEM_EXEC;
	m.m_sys_exec.bprm = (u64)bprm;
	ret = ipc_send(ENDPOINT_SYSTEM, &m);
	if (ret)
		return ret;

	return m.m_sys_exec.retval;
}
