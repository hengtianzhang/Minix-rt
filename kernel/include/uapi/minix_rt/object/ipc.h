#ifndef __UAPI_MINIX_RT_OBJECT_IPC_H_
#define __UAPI_MINIX_RT_OBJECT_IPC_H_

#include <asm/base/page-def.h>

#ifndef __KERNEL__
#include <minix_rt/notifier.h>
#else
#include <uapi/minix_rt/notifier.h>
#endif /* !__KERNEL__ */

#define IPC_DEBUG_PRINTF_BUFFER_MAX 1024

struct ipc_share_struct {
	char	debug_put_buffer[IPC_DEBUG_PRINTF_BUFFER_MAX];
	long notifier_message_info[NOTIFIER_NR_MAX];
	pid_t	notifier[NOTIFIER_NR_MAX];
};

#endif /* !__UAPI_MINIX_RT_OBJECT_IPC_H_ */
