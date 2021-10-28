#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libminix_rt/mmap.h>
#include <libminix_rt/ipc.h>

#include "binfmt.h"
#include "exec.h"
#include "read_write.h"
#include "open.h"

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
		default:
			break;
	}
}

int main(void)
{
	int ret = 0;
	message_t m;

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
