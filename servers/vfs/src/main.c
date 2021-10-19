#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libminix_rt/ipc.h>

static void vfs_handle_ipc_message(endpoint_t ep, message_t *m)
{
	int ret;
	void *buf;

	switch (m->m_type & IPC_M_TYPE_MASK) {
		case IPC_M_TYPE_VFS_EXEC:
			buf = malloc(m->m_vfs_exec.filename_len);
			if (!buf)
				panic("malloc failed!\n");

			ret = message_memcpy(buf, m->m_vfs_exec.filename,
						m->m_vfs_exec.filename_len, m->m_source);
			if (ret <= 0)
				panic("message memcopy failed! (%d)\n", ret);

			printf("filename %s\n", (char *)buf);
			printf("src pid %d\n", m->m_source);

			m->m_vfs_exec.retval = 3;

			free(buf);

			break;
		default:
			break;
	}
}

int main(void)
{
	int ret = 0;
	message_t m;

	printf("This is vfs\n");

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
