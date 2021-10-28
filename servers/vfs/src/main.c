#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libminix_rt/mmap.h>
#include <libminix_rt/ipc.h>

#include "binfmt.h"
#include "exec.h"
#include "read_write.h"
#include "open.h"

struct task_struct *vfs_find_task(pid_t pid)
{
	struct pid_struct *pids;

	pids = pid_find(pid);
	if (!pids)
		return NULL;

	return container_of(pids, struct task_struct, pid);
}

static void vfs_handle_ipc_message(endpoint_t ep, message_t *m)
{
	switch (m->m_type & IPC_M_TYPE_MASK) {
		case IPC_M_TYPE_VFS_EXEC:
			do_exec(ep, m);
			break;
		case IPC_M_TYPE_VFS_WRITE:
			do_write(ep, m);
			break;
		case IPC_M_TYPE_VFS_WRITEV:
			do_writev(ep, m);
			break;
		case IPC_M_TYPE_VFS_OPENAT:
			do_openat(ep, m);
			break;	
		case IPC_M_TYPE_VFS_IOCTL:
			do_ioctl(ep, m);
			break;
		case IPC_M_TYPE_VFS_CHDIR:
			do_chdir(ep, m);
			break;
		default:
			break;
	}
}

int main(void)
{
	int ret = 0;
	message_t m;
	struct task_struct *tsk;

	tsk = malloc(sizeof (struct task_struct));
	BUG_ON(!tsk);
	tsk->pid.pid = 1;
	__set_bit(0, tsk->file.fdtable);
	__set_bit(1, tsk->file.fdtable);
	__set_bit(2, tsk->file.fdtable);
	BUG_ON(!pid_insert(&tsk->pid));

	init_elf_binfmt();

	ret = mmap_initrd(&initrd_start, &initrd_size);
	BUG_ON(ret);

	while (1) {
		memset(&m, 0, sizeof (message_t));

		ret = ipc_receive(ENDPOINT_VFS, &m);
		if (ret) {
			panic("VFS receive message fail!\n");
		}

		vfs_handle_ipc_message(ENDPOINT_VFS, &m);

		ret = ipc_reply(ENDPOINT_VFS, &m);
		if (ret) {
			panic("VFS reply message fail!\n");
		}
	};
	BUG();

	return 0;
}
