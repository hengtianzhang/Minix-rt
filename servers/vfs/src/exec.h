#ifndef __SERVERS_VFS_SRC_EXEC_H_
#define __SERVERS_VFS_SRC_EXEC_H_

extern unsigned long initrd_start;
extern unsigned long initrd_size;

extern void do_exec(endpoint_t ep, message_t *m);

#endif /* !__SERVERS_VFS_SRC_EXEC_H_ */
