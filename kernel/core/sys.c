#include <minix_rt/syscalls.h>
#include <minix_rt/uts.h>
#include <minix_rt/sched.h>
#include <minix_rt/ipc.h>

SYSCALL_DEFINE0(getpid)
{
	return current->pid.pid;
}

SYSCALL_DEFINE0(getuid)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));
	m.m_type = IPC_M_TYPE_PM_GETUID;

	ret = __ipc_send(ENDPOINT_PM, &m);
	if (ret)
		return -EINVAL;

	return m.m_u64.data[0];
}

SYSCALL_DEFINE0(geteuid)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));
	m.m_type = IPC_M_TYPE_PM_GETEUID;

	ret = __ipc_send(ENDPOINT_PM, &m);
	if (ret)
		return -EINVAL;

	return m.m_u64.data[0];
}

SYSCALL_DEFINE0(getgid)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));
	m.m_type = IPC_M_TYPE_PM_GETGID;

	ret = __ipc_send(ENDPOINT_PM, &m);
	if (ret)
		return -EINVAL;

	return m.m_u64.data[0];
}

SYSCALL_DEFINE0(getegid)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));
	m.m_type = IPC_M_TYPE_PM_GETEGID;

	ret = __ipc_send(ENDPOINT_PM, &m);
	if (ret)
		return -EINVAL;

	return m.m_u64.data[0];
}

SYSCALL_DEFINE1(newuname, struct new_utsname __user *, name)
{
	struct new_utsname tmp;

	memcpy(&tmp, &utsname, sizeof(tmp));
	if (copy_to_user(name, &tmp, sizeof(tmp)))
		return -EFAULT;

	return 0;
}
