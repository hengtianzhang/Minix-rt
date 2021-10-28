#ifndef __MINIX_RT_NOTIFIER_H_
#define __MINIX_RT_NOTIFIER_H_

#include <minix_rt/spinlock.h>

#include <uapi/minix_rt/notifier.h>

#include <asm/siginfo.h>

struct notifier_struct {
	notifier_table_t notifier_table;
	struct k_sigaction action[NOTIFIER_NR_MAX];
	__sigrestore_t return_fn;
	sigset_t blocked;
	spinlock_t		siglock;
	unsigned long private[NOTIFIER_NR_MAX];
	pid_t 			pid[NOTIFIER_NR_MAX];
};

struct ksignal {
	struct k_sigaction ka;
	int sig;
};

#endif /* !__MINIX_RT_NOTIFIER_H_ */
