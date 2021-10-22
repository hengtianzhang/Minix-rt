#include <string.h>

#include <libminix_rt/syscalls.h>
#include <libminix_rt/ipc.h>

int ipc_send(endpoint_t dest, message_t *m_ptr)
{
	if (dest > ENDPOINT_END || !m_ptr)
		return -EINVAL;

	return __syscall(__NR_ipc_send, dest, m_ptr);
}

int ipc_receive(endpoint_t src, message_t *m_ptr)
{
	if (src > ENDPOINT_END || !m_ptr)
		return -EINVAL;

	return __syscall(__NR_ipc_receive, src, m_ptr);
}

int ipc_reply(endpoint_t src, message_t *m_ptr)
{
	if (src > ENDPOINT_END || !m_ptr)
		return -EINVAL;

	return __syscall(__NR_ipc_reply, src, m_ptr);
}

int ipc_notify(endpoint_t dest, message_t *m_ptr)
{
	if (dest > ENDPOINT_END || !m_ptr)
		return -EINVAL;

	return __syscall(__NR_ipc_notify, dest, m_ptr);
}

unsigned long get_task_size(void)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));
	m.m_type = IPC_M_TYPE_SYSTEM_TASK_SIZE;
	ret = ipc_send(ENDPOINT_SYSTEM, &m);
	if (ret)
		return 0;

	return m.m_u64.data[0];
}

u64 get_arch_auxvec_cnt(void)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));
	m.m_type = IPC_M_TYPE_SYSTEM_AUXVEC_CNT;
	ret = ipc_send(ENDPOINT_SYSTEM, &m);
	if (ret)
		return 0;

	return m.m_u64.data[0];
}

int get_arch_auxvec(u64 *auxvec, int cnt)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));
	m.m_type = IPC_M_TYPE_SYSTEM_AUXVEC;
	m.m_u64.data[0] = cnt;
	ret = ipc_send(ENDPOINT_SYSTEM, &m);
	if (ret)
		return ret;

	auxvec[0] = m.m_u64.data[0];
	auxvec[1] = m.m_u64.data[1];

	return 0;
}

u64 get_arch_elf_hwcap(void)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));
	m.m_type = IPC_M_TYPE_SYSTEM_ELF_HWCAP;
	ret = ipc_send(ENDPOINT_SYSTEM, &m);
	if (ret)
		return 0;

	return m.m_u64.data[0];
}
