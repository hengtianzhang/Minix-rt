#ifndef __LIBMINIX_RT_NOTIFIER_H_
#define __LIBMINIX_RT_NOTIFIER_H_

#include <minix_rt/notifier.h>

extern asmlinkage void __kernel_rt_sigreturn(void);

extern int notifier_register_handler(unsigned int notifier, __sighandler_t fn);
extern int notifier_remove_handler(unsigned int notifier);

extern int notifier_send_child_exit(int flags);
extern int notifier_send_notifier(int notifier, pid_t receiver, void *private);

#endif /* !__LIBMINIX_RT_NOTIFIER_H_ */
