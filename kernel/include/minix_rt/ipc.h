#ifndef __MINIX_RT_IPC_H_
#define __MINIX_RT_IPC_H_

#include <base/list.h>

#include <minix_rt/wait.h>

#include <uapi/minix_rt/ipc.h>

struct endpoint_info {
	spinlock_t	ep_lock;
	u32 ipc_req_count;
	int state;
	wait_queue_head_t wait;
	const endpoint_t endpoint;
	const char *const name;
	struct task_struct *tsk;
	struct list_head	ep_list;
	spinlock_t	ep_list_lock;
};

#define EP_STATE_WAITTING 	0x1
#define EP_STATE_RUNNING	0x2	

struct ipc_mess_node {
	int is_finish;
	message_t m;
	wait_queue_head_t wait;
	struct list_head queue_list;
};

extern void ipc_init(void);
extern int ipc_register_endpoint_by_tsk(struct task_struct *tsk);

extern int __ipc_send(endpoint_t dest, message_t *m_ptr);
extern int __ipc_receive(endpoint_t src, message_t *m_ptr);
extern int __ipc_reply(endpoint_t src, message_t *m_ptr);
extern int __ipc_notify(endpoint_t dest, message_t *m_ptr);

#endif /* !__MINIX_RT_IPC_H_ */
