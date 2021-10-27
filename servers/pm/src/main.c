#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libminix_rt/ipc.h>

#include "pm.h"

struct task_struct *pm_find_task(pid_t pid)
{
	struct pid_struct *pids;

	pids = pid_find(pid);
	if (!pids)
		return NULL;

	return container_of(pids, struct task_struct, pid);
}

static void pm_get_tsk_id(int type, message_t *m)
{
	struct task_struct *tsk;

	tsk = pm_find_task(m->m_source);
	if (!tsk) {
		m->m_u64.data[0] = 0;
		return;
	}

	switch (type) {
		case IPC_M_TYPE_PM_GETUID:
			m->m_u64.data[0] = tsk->uid;
			break;
		case IPC_M_TYPE_PM_GETEUID:
			m->m_u64.data[0] = tsk->euid;
			break;
		case IPC_M_TYPE_PM_GETGID:
			m->m_u64.data[0] = tsk->gid;
			break;
		case IPC_M_TYPE_PM_GETEGID:
			m->m_u64.data[0] = tsk->egid;
			break;
		default:
				break;
	}
}

static void pm_handle_ipc_message(endpoint_t ep, message_t *m)
{
	switch (m->m_type & IPC_M_TYPE_MASK) {
		case IPC_M_TYPE_PM_GETUID:
		case IPC_M_TYPE_PM_GETEUID:
		case IPC_M_TYPE_PM_GETGID:
		case IPC_M_TYPE_PM_GETEGID:
			pm_get_tsk_id(m->m_type & IPC_M_TYPE_MASK, m);
			break;
		default:
			break;
	}
}

int main(void)
{
	int ret = 0;
	message_t m;
	struct task_struct *tsk;

	tsk = malloc(sizeof (struct task_struct));
	BUG_ON(!tsk);
	tsk->uid = 0;
	tsk->euid = 0;
	tsk->gid = 0;
	tsk->egid = 0;
	tsk->pid.pid = 1;
	BUG_ON(!pid_insert(&tsk->pid));

	while (1) {
		memset(&m, 0, sizeof (message_t));

		ret = ipc_receive(ENDPOINT_PM, &m);
		if (ret) {
			panic("PM receive message fail!\n");
		}

		pm_handle_ipc_message(ENDPOINT_PM, &m);

		ret = ipc_reply(ENDPOINT_PM, &m);
		if (ret) {
			panic("PM reply message fail!\n");
		}
	};
	BUG();

	return 0;
}
