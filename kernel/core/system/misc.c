#include <minix_rt/system.h>
#include <minix_rt/sched.h>
#include <minix_rt/page.h>
#include <minix_rt/mmap.h>
#include <minix_rt/ktime.h>

#include <asm/processor.h>

void system_misc(endpoint_t ep, message_t *m)
{
	int m_type = m->m_type & IPC_M_TYPE_MASK;

	switch (m_type) {
		case IPC_M_TYPE_SYSTEM_TASK_SIZE:
			m->m_u64.data[0] = TASK_SIZE;
			break;
		case IPC_M_TYPE_SYSTEM_SEED:
			m->m_u64.data[0] = ktime_get_cycles();
		default:
			break;
	}
}
