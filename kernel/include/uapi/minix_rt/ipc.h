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
	u64 brk;
	int retval;
	u8 padding[44];
} mess_system_brk;
IPC_ASSERT_MSG_SIZE(mess_system_brk);

typedef struct {
#define DIRECT_MEMCPY_FROM		0x1
#define DIRECT_MEMCPY_TO		0x2
#define DIRECT_STRLEN			0x3
#define DIRECT_STRCPY_FROM		0x4
#define DIRECT_STRCPY_TO		0x5
	int direct;
	pid_t pid;
	const void *dest;
	const void *src;
	int size;
	int retval;
#if BITS_PER_LONG == 64
	u8 padding[24];
#elif BITS_PER_LONG == 32
	u8 padding[32];
#else
#error "Not availd sys string size btypes"
#endif
} mess_system_string;
IPC_ASSERT_MSG_SIZE(mess_system_string);

typedef struct {
	u64 mmap_base;
	u64 size;
	unsigned long vm_flags;
#define MMAP_INITRD		0x1
	int flags;
	int retval;
	u8 padding[24];
} mess_system_mmap;
IPC_ASSERT_MSG_SIZE(mess_system_mmap);

typedef struct {
	u64 bprm;
	int retval;
	u8 padding[44];
} mess_system_exec;
IPC_ASSERT_MSG_SIZE(mess_system_exec);

typedef struct {
	const char *filename;
	const char *const *argv;
	const char *const *envp;
	int filename_len;
	int argv_len;
	int envp_len;
	int retval;
#if BITS_PER_LONG == 64
	u8 padding[16];
#elif BITS_PER_LONG == 32
	u8 padding[28];
#else
#error "Not availd vfs exec size btypes"
#endif
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
		mess_system_string	m_sys_string;
		mess_system_mmap	m_sys_mmap;
		mess_system_exec	m_sys_exec;

		mess_vfs_exec		m_vfs_exec;

		u8			size[IPC_MAX_MESSAGE_BYPE];	/* message payload may have 56 bytes at most */
	};
} message_t __aligned(16);

/* Ensure the complete union respects the IPC assumptions. */
typedef int _ASSERT_message_t[/* CONSTCOND */sizeof(message_t) == 64 ? 1 : -1];

#define IPC_M_TYPE_MASK			0x7fffffff
#define IPC_M_TYPE_NOTIFIER 	0x80000000

#define IPC_M_TYPE_SYSTEM_SBRK			1
#define IPC_M_TYPE_SYSTEM_BRK			2
#define IPC_M_TYPE_SYSTEM_STRING		3
#define IPC_M_TYPE_SYSTEM_MMAP			4
#define IPC_M_TYPE_SYSTEM_EXEC			5
#define IPC_M_TYPE_SYSTEM_TASK_SIZE		6
#define IPC_M_TYPE_SYSTEM_SEED			7
#define IPC_M_TYPE_SYSTEM_AUXVEC_CNT	8
#define IPC_M_TYPE_SYSTEM_AUXVEC		9
#define IPC_M_TYPE_SYSTEM_ELF_HWCAP		10

#define IPC_M_TYPE_VFS_EXEC				11

enum {
	ENDPOINT_SYSTEM,
	ENDPOINT_I2C,
	ENDPOINT_PM,
	ENDPOINT_VFS,
	ENDPOINT_END,
};

#endif /* !__UAPI_MINIX_RT_IPC_H_ */
