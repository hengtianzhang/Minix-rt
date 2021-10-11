#ifndef __MINIX_RT_OBJECT_IPC_H_
#define __MINIX_RT_OBJECT_IPC_H_

#include <uapi/minix_rt/object/ipc.h>

#include <minix_rt/sched.h>

#include <asm/current.h>

struct task_struct;

extern int
ipc_create_task_ipcptr(struct task_struct *tsk, unsigned long ipcptr);

static inline char *ipc_get_debug_buffer(void)
{
	struct ipc_share_struct *ipc_shr = current->kernel_ipcptr;

	return ipc_shr->debug_put_buffer;
}
#endif /* !__MINIX_RT_OBJECT_IPC_H_ */