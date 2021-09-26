#ifndef __UAPI_SEL4M_OBJECT_IPC_H_
#define __UAPI_SEL4M_OBJECT_IPC_H_

#include <asm/base/page-def.h>

#ifndef __KERNEL__
#include <sel4m/object/notifier.h>
#else
#include <uapi/sel4m/object/notifier.h>
#endif /* !__KERNEL__ */

struct ipc_share_struct {
	long notifier_message_info[NOTIFIER_NR_MAX];
	pid_t	notifier[NOTIFIER_NR_MAX];
};

#endif /* !__UAPI_SEL4M_OBJECT_IPC_H_ */
