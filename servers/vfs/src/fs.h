#ifndef __SERVERS_VFS_SRC_FS_H_
#define __SERVERS_VFS_SRC_FS_H_

#include <libminix_rt/pid.h>

#include <minix_rt/fs.h>

#include "open.h"

struct task_struct {
	struct files_struct file;
	struct pid_struct pid;
};

struct task_struct *vfs_find_task(pid_t pid);

struct filename {
	const char	*name;
	const void *file;
	unsigned long file_size;
};

#endif /* !__SERVERS_VFS_SRC_FS_H_ */
