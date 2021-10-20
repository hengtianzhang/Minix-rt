#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <libminix_rt/ipc.h>

#include "exec.h"

unsigned long initrd_start = 0;
unsigned long initrd_size = 0;

void do_exec(endpoint_t ep, message_t *m)
{
	int ret;
	void *filename, *argv, *envp;

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
		goto free_filename;
	}

	printf("filename %s\n", (char *)filename);

	argv = malloc(m->m_vfs_exec.argv_len);
	if (!argv) {
		printf("do_exec malloc failed!\n");
		m->m_vfs_exec.retval = -ENOMEM;
		goto free_filename;
	}

	ret = message_memcpy(argv, m->m_vfs_exec.argv,
				m->m_vfs_exec.argv_len, m->m_source);
	if (ret != m->m_vfs_exec.argv_len) {
		printf("message memcpy failed! retval (%d)\n", ret);
		m->m_vfs_exec.retval = ret;
		goto free_argv;
	}

	envp = malloc(m->m_vfs_exec.envp_len);
	if (!envp) {
		printf("do_exec malloc failed!\n");
		m->m_vfs_exec.retval = -ENOMEM;
		goto free_argv;
	}

	ret = message_memcpy(envp, m->m_vfs_exec.envp,
				m->m_vfs_exec.envp_len, m->m_source);
	if (ret != m->m_vfs_exec.envp_len) {
		printf("message memcpy failed! retval (%d)\n", ret);
		m->m_vfs_exec.retval = ret;
		goto free_envp;
	}

	m->m_vfs_exec.retval = 3;

free_envp:
	free(envp);
free_argv:
	free(argv);
free_filename:
	free(filename);
}
