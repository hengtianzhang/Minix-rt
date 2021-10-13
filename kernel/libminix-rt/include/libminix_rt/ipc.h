#ifndef __LIBMINIX_RT_IPC_H_
#define __LIBMINIX_RT_IPC_H_

#include <minix_rt/ipc.h>

int ipc_send(endpoint_t dest, message_t *m_ptr);
int ipc_receive(endpoint_t src, message_t *m_ptr);
int ipc_reply(endpoint_t src, message_t *m_ptr);
int ipc_notify(endpoint_t dest, message_t *m_ptr);

#endif /* !__LIBMINIX_RT_IPC_H_ */
