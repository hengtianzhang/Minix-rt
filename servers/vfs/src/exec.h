#ifndef __SERVERS_VFS_SRC_EXEC_H_
#define __SERVERS_VFS_SRC_EXEC_H_

#include "fs.h"

extern unsigned long initrd_start;
extern unsigned long initrd_size;

extern void do_exec(endpoint_t ep, message_t *m);
extern int do_execve(struct filename *filename, message_t *m);
extern struct filename *get_filename(const char *name);

#endif /* !__SERVERS_VFS_SRC_EXEC_H_ */
