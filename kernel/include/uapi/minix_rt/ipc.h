#ifndef __UAPI_MINIX_RT_IPC_H_
#define __UAPI_MINIX_RT_IPC_H_

#ifndef __KERNEL__
#include <minix_rt/ipcconst.h>
#else
#include <uapi/minix_rt/ipcconst.h>
#endif

typedef struct {
	u8	data[56];
} mess_u8;
IPC_ASSERT_MSG_SIZE(mess_u8);

typedef struct {
	u16	data[28];
} mess_u16;
IPC_ASSERT_MSG_SIZE(mess_u16);

typedef struct {
	u32 data[14];
} mess_u32;
IPC_ASSERT_MSG_SIZE(mess_u32);

typedef struct {
	u64 data[7];
} mess_u64;
IPC_ASSERT_MSG_SIZE(mess_u64);

typedef struct {
	int state;
	u64 brk;
	u8 payload[34];
} mess_system_brk;
IPC_ASSERT_MSG_SIZE(mess_system_brk);

typedef struct {
	const char *filename;
	const char *const *argv;
	const char *const *envp;
	int retval;
	u8 payload[28];
} mess_vfs_exec;
IPC_ASSERT_MSG_SIZE(mess_vfs_exec);

typedef struct {
	pid_t m_source; /* who sent the message */
	int	m_type;		/* what kind of message is it */
	union {
		mess_u8		m_u8;
		mess_u16	m_u16;
		mess_u32	m_u32;
		mess_u64	m_u64;

		mess_system_brk		m_sys_brk;

		mess_vfs_exec		m_vfs_exec;

		u8			size[IPC_MAX_MESSAGE_BYPE];	/* message payload may have 56 bytes at most */
	};
} message_t __aligned(16);

/* Ensure the complete union respects the IPC assumptions. */
typedef int _ASSERT_message_t[/* CONSTCOND */sizeof(message_t) == 64 ? 1 : -1];

#define IPC_M_TYPE_MASK			0x7fffffff
#define IPC_M_TYPE_NOTIFIER 	0x80000000

#define IPC_M_TYPE_SYSTEM_SBRK			0x1
#define IPC_M_TYPE_SYSTEM_BRK			0x2

#define IPC_M_TYPE_VFS_EXEC				0x3

enum {
	ENDPOINT_SYSTEM,
	ENDPOINT_I2C,
	ENDPOINT_PM,
	ENDPOINT_VFS,
	ENDPOINT_END,
};

#endif /* !__UAPI_MINIX_RT_IPC_H_ */
