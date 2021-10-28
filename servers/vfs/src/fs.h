#ifndef __SERVERS_VFS_SRC_FS_H_
#define __SERVERS_VFS_SRC_FS_H_

#include <minix_rt/fs.h>

struct filename {
	const char	*name;
	const void *file;
	unsigned long file_size;
};

#endif /* !__SERVERS_VFS_SRC_FS_H_ */
