#ifndef __LIBSEL4M_OBJECT_TCB_H_
#define __LIBSEL4M_OBJECT_TCB_H_

#include <base/types.h>

#include <sel4m/object/tcb.h>

typedef int (*tcb_thread_fn_t)(void *);

extern int tcb_create_thread(pid_t pid, tcb_thread_fn_t fn, void *arg);

#endif /* !__LIBSEL4M_OBJECT_TCB_H_ */
