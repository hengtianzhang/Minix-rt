#include <minix_rt/system.h>
#include <minix_rt/syscalls.h>
#include <minix_rt/slab.h>
#include <minix_rt/sched.h>

#include <uapi/minix_rt/uio.h>

extern void syscall_printf(const char *buf);

SYSCALL_DEFINE3(write, unsigned int, fd, const char __user *, buf,
		size_t, count)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));

	m.m_type = IPC_M_TYPE_VFS_WRITE;
	m.m_vfs_write.fd = fd;
	m.m_vfs_write.buf = buf;
	m.m_vfs_write.count = count;

	ret = __ipc_send(ENDPOINT_VFS, &m);
	if (ret)
		return ret;

	return m.m_vfs_write.retval;
}

SYSCALL_DEFINE3(writev, unsigned long, fd, const struct iovec __user *, vec,
		unsigned long, vlen)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));

	m.m_type = IPC_M_TYPE_VFS_WRITEV;
	m.m_vfs_writev.fd = fd;
	m.m_vfs_writev.vec = vec;
	m.m_vfs_writev.vlen = vlen;

	ret = __ipc_send(ENDPOINT_VFS, &m);
	if (ret)
		return ret;

	return m.m_vfs_write.retval;
}

SYSCALL_DEFINE4(openat, int, dfd, const char __user *, filename, int, flags,
		umode_t, mode)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));

	m.m_type = IPC_M_TYPE_VFS_OPENAT;
	m.m_vfs_openat.dfd = dfd;
	m.m_vfs_openat.filename = filename;
	m.m_vfs_openat.filename_size = strlen(filename) + 1;
	m.m_vfs_openat.flags = flags;
	m.m_vfs_openat.mode = mode;

	ret = __ipc_send(ENDPOINT_VFS, &m);
	if (ret)
		return ret;

	return m.m_vfs_write.retval;
}

SYSCALL_DEFINE3(ioctl, unsigned int, fd, unsigned int, cmd, unsigned long, arg)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));

	m.m_type = IPC_M_TYPE_VFS_IOCTL;
	m.m_vfs_ioctl.fd = fd;
	m.m_vfs_ioctl.cmd = cmd;
	m.m_vfs_ioctl.arg = arg;

	ret = __ipc_send(ENDPOINT_VFS, &m);
	if (ret)
		return ret;

	return m.m_vfs_ioctl.retval;
}

SYSCALL_DEFINE1(chdir, const char __user *, filename)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));

	m.m_type = IPC_M_TYPE_VFS_CHDIR;
	m.m_vfs_chdir.filename = filename;
	m.m_vfs_chdir.filename_size = strlen(filename) + 1;

	ret = __ipc_send(ENDPOINT_VFS, &m);
	if (ret)
		return ret;

	return m.m_vfs_ioctl.retval;
}

SYSCALL_DEFINE0(setsid)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));

	m.m_type = IPC_M_TYPE_PM_SETSID;

	ret = __ipc_send(ENDPOINT_PM, &m);
	if (ret)
		return ret;

	return m.m_s32.data[0];
}
