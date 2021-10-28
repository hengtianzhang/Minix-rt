#ifndef __SERVERS_VFS_SRC_IOCTL_H_
#define __SERVERS_VFS_SRC_IOCTL_H_

#include <minix_rt/ioctl.h>

#define FIGETBSZ   _IO(0x00,2)	/* get the block size used for bmap */
#define FITHAW		_IOWR('X', 120, int)	/* Thaw */
#define FICLONE		_IOW(0x94, 9, int)

#define	FS_IOC_GETFLAGS			_IOR('f', 1, long)
#define	FS_IOC_SETFLAGS			_IOW('f', 2, long)
#define	FS_IOC_GETVERSION		_IOR('v', 1, long)
#define	FS_IOC_SETVERSION		_IOW('v', 2, long)

#endif /* !__SERVERS_VFS_SRC_IOCTL_H_ */
