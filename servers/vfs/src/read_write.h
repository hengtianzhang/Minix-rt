#ifndef __SERVERS_VFS_SRC_READ_WRITE_H_
#define __SERVERS_VFS_SRC_READ_WRITE_H_

extern void do_write(endpoint_t ep, message_t *m);
extern void do_writev(endpoint_t ep, message_t *m);

#endif /* !__SERVERS_VFS_SRC_READ_WRITE_H_ */
