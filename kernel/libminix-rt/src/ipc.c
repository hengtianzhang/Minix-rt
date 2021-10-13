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
