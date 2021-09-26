#ifndef __LIBSEL4M_OBJECT_TCB_H_
#define __LIBSEL4M_OBJECT_TCB_H_

#include <base/types.h>
#include <base/linkage.h>

#include <sel4m/object/tcb.h>

typedef int (*tcb_thread_fn_t)(void *);

extern pid_t tcb_create_thread(tcb_thread_fn_t fn, void *arg);
extern asmlinkage void __sel4m_exit_c(void);

#endif /* !__LIBSEL4M_OBJECT_TCB_H_ */
