#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <libminix_rt/mmap.h>
#include <libminix_rt/ipc.h>

#include "fs.h"
#include "open.h"
#include "ioctl.h"

void do_openat(endpoint_t ep, message_t *m)
{
	int ret;
	pid_t pid = m->m_source;
	struct task_struct *tsk;
	const char *filename;
	int new_fd;

	tsk = vfs_find_task(pid);
	BUG_ON(!tsk);

	filename = malloc(m->m_vfs_openat.filename_size);
	if (!filename) {
		m->m_vfs_openat.retval = -ENOMEM;
		return;
	}

	ret = message_memcpy((void *)filename, m->m_vfs_openat.filename,
			m->m_vfs_openat.filename_size, pid);
	if (ret != m->m_vfs_openat.filename_size) {
		m->m_vfs_openat.retval = -EINVAL;
		goto free_filename;
	}

	new_fd = find_last_bit(tsk->file.fdtable, MAX_FILES) + 1;
	if (strcmp(filename, "/dev/null") == 0) {
		__set_bit(new_fd, tsk->file.fdtable);
		m->m_vfs_openat.retval = new_fd;
		goto free_filename;
	} else {
		printf("TODO open file %s\n", filename);
		while (1);
	}

free_filename:
	free((void *)filename);
}

void do_ioctl(endpoint_t ep, message_t *m)
{
	/* TODO */
}

void do_chdir(endpoint_t ep, message_t *m)
{
	m->m_vfs_chdir.retval = 0;
}
