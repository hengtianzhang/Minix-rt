#ifndef __UAPI_SEL4M_OBJECT_IPC_H_
#define __UAPI_SEL4M_OBJECT_IPC_H_

#include <asm/base/page-def.h>

#ifndef __KERNEL__
#include <sel4m/object/notifier.h>
#else
#include <uapi/sel4m/object/notifier.h>
#endif /* !__KERNEL__ */

#define IPC_DEBUG_PRINTF_BUFFER_MAX 1024

struct ipc_share_struct {
	char	debug_put_buffer[IPC_DEBUG_PRINTF_BUFFER_MAX];
	long notifier_message_info[NOTIFIER_NR_MAX];
	pid_t	notifier[NOTIFIER_NR_MAX];
};

#endif /* !__UAPI_SEL4M_OBJECT_IPC_H_ */
