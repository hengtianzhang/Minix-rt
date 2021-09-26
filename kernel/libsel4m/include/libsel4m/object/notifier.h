#ifndef __LIBSEL4M_OBJECT_NOTIFIER_H_
#define __LIBSEL4M_OBJECT_NOTIFIER_H_

#include <sel4m/object/notifier.h>

extern asmlinkage void __kernel_rt_sigreturn(void);

extern int notifier_register_handler(unsigned int notifier, __sighandler_t fn);

extern int notifier_send_child_exit(int flags);
extern int notifier_send_notifier(int notifier, pid_t receiver, void *private);

#endif /* !__LIBSEL4M_OBJECT_NOTIFIER_H_ */
