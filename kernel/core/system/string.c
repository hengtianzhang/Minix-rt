#include <minix_rt/system.h>
#include <minix_rt/sched.h>
#include <minix_rt/page.h>
#include <minix_rt/mmap.h>

static int message_memcpy(const void *dst, const void *src,
						int size, pid_t dst_pid, pid_t src_pid)
{
	return -3;
}

void system_string(endpoint_t ep, message_t *m)
{
	int ret;
	int direct = m->m_sys_string.direct;

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
