#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <libminix_rt/ipc.h>

#include "exec.h"

void do_exec(endpoint_t ep, message_t *m)
{
	int ret;
	void *filename;

	filename = malloc(m->m_vfs_exec.filename_len);
	if (!filename) {
		printf("do_exec malloc failed!\n");
		m->m_vfs_exec.retval = -ENOMEM;
		return;
	}

	ret = message_memcpy(filename, m->m_vfs_exec.filename,
				m->m_vfs_exec.filename_len, m->m_source);
	if (ret != m->m_vfs_exec.filename_len) {
		printf("message memcpy failed! retval (%d)\n", ret);
		m->m_vfs_exec.retval = ret;
		goto free_out;
	}

	printf("filename %s\n", (char *)filename);
	printf("src pid %d\n", m->m_source);

	m->m_vfs_exec.retval = 3;

free_out:
	free(filename);
}
