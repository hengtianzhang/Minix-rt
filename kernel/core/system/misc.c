#include <minix_rt/system.h>
#include <minix_rt/sched.h>
#include <minix_rt/page.h>
#include <minix_rt/mmap.h>
#include <minix_rt/ktime.h>

#include <asm/processor.h>
#include <asm/hwcap.h>

void system_misc(endpoint_t ep, message_t *m)
{
	int cnt;
	int m_type = m->m_type & IPC_M_TYPE_MASK;

	switch (m_type) {
		case IPC_M_TYPE_SYSTEM_TASK_SIZE:
			m->m_u64.data[0] = TASK_SIZE;
			break;
		case IPC_M_TYPE_SYSTEM_SEED:
			m->m_u64.data[0] = ktime_get_cycles();
			break;
		case IPC_M_TYPE_SYSTEM_AUXVEC_CNT:
			m->m_u64.data[0] = get_arch_auxvec_cnt();
			break;
		case IPC_M_TYPE_SYSTEM_AUXVEC:
			cnt = m->m_u64.data[0];
			get_arch_auxvec(&m->m_u64.data[0], cnt);
			break;
		case IPC_M_TYPE_SYSTEM_ELF_HWCAP:
			m->m_u64.data[0] = ELF_HWCAP;
		default:
			break;
	}
}
