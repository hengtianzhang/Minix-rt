#ifndef __MINIX_RT_SYSTEM_H_
#define __MINIX_RT_SYSTEM_H_

#include <minix_rt/ipc.h>

extern void system_brk(endpoint_t ep, message_t *m);

#endif /* !__MINIX_RT_SYSTEM_H_ */