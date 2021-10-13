#ifndef __UAPI_MINIX_RT_IPCCONST_H_
#define __UAPI_MINIX_RT_IPCCONST_H_

#include <base/compiler.h>
#include <base/types.h>

#define IPC_MAX_MESSAGE_BYPE	56

/* Check that the message payload type doesn't grow past the maximum IPC payload size.
 * This is a compile time check. */
#define IPC_ASSERT_MSG_SIZE(msg_type) \
    typedef int _ASSERT_##msg_type[/* CONSTCOND */sizeof(msg_type) == IPC_MAX_MESSAGE_BYPE ? 1 : -1]

#endif /* !__UAPI_MINIX_RT_IPCCONST_H_ */
