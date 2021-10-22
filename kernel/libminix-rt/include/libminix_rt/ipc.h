#ifndef __LIBMINIX_RT_IPC_H_
#define __LIBMINIX_RT_IPC_H_

#include <minix_rt/ipc.h>

int ipc_send(endpoint_t dest, message_t *m_ptr);
int ipc_receive(endpoint_t src, message_t *m_ptr);
int ipc_reply(endpoint_t src, message_t *m_ptr);
int ipc_notify(endpoint_t dest, message_t *m_ptr);

/*
 * Misc
 */
extern unsigned long get_task_size(void);
extern void get_random_bytes(void *buf, int nbytes);

extern u64 get_arch_auxvec_cnt(void);
extern int get_arch_auxvec(u64 *auxvec, int cnt);
extern u64 get_arch_elf_hwcap(void);

#endif /* !__LIBMINIX_RT_IPC_H_ */
