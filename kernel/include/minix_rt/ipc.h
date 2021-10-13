#ifndef __MINIX_RT_IPC_H_
#define __MINIX_RT_IPC_H_

#include <base/list.h>

#include <uapi/minix_rt/ipc.h>

enum {
	ENDPOINT_SYSTEM,
	ENDPOINT_I2C,
	ENDPOINT_PM,
	ENDPOINT_VFS,
	ENDPOINT_END,
};

struct endpoint_info {
	const endpoint_t endpoint;
	const char *const name;
	struct task_struct *tsk;
};

struct ipc_mess_node {
	struct list_head queue_list;
};

extern int ipc_register_endpoint_by_tsk(struct task_struct *tsk);

#endif /* !__MINIX_RT_IPC_H_ */
