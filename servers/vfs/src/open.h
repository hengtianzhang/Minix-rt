#ifndef __SERVERS_VFS_SRC_OPEN_H_
#define __SERVERS_VFS_SRC_OPEN_H_

#include <base/types.h>

#define MAX_FILES	1024
#define FILES_WORDS	(MAX_FILES / __BITS_PER_LONG)

struct file {
	const char *filename;
};

struct files_struct {
	unsigned long fdtable[FILES_WORDS];
	struct file *fd_array[MAX_FILES];
};

extern void do_openat(endpoint_t ep, message_t *m);
extern void do_ioctl(endpoint_t ep, message_t *m);
extern void do_chdir(endpoint_t ep, message_t *m);

#endif /* !__SERVERS_VFS_SRC_OPEN_H_ */
