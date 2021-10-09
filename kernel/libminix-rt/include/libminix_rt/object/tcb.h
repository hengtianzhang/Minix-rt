#ifndef __LIBMINIX_RT_OBJECT_TCB_H_
#define __LIBMINIX_RT_OBJECT_TCB_H_

#include <base/types.h>
#include <base/linkage.h>

#include <minix_rt/object/tcb.h>

typedef int (*tcb_thread_fn_t)(void *);

extern pid_t tcb_create_thread(tcb_thread_fn_t fn, void *arg);
extern pid_t tcb_get_pid_info(void);

extern asmlinkage void __minix_rt_exit_c(void);

#endif /* !__LIBMINIX_RT_OBJECT_TCB_H_ */
