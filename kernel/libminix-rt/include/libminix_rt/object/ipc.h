#ifndef __LIBMINIX_RT_OBJECT_IPC_H_
#define __LIBMINIX_RT_OBJECT_IPC_H_

#include <minix_rt/object/ipc.h>

extern void ipc_set_user_space_ptr(unsigned long ipcptr);
extern unsigned long ipc_get_user_space_ptr(void);

static inline char *ipc_get_debug_buffer(void)
{
	struct ipc_share_struct *ipc_shr;

	ipc_shr = (struct ipc_share_struct *)ipc_get_user_space_ptr();
	return ipc_shr->debug_put_buffer;
}

#endif /* !__LIBMINIX_RT_OBJECT_IPC_H_ */
