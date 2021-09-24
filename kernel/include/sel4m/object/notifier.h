#ifndef __SEL4M_OBJECT_NOTIFIER_H_
#define __SEL4M_OBJECT_NOTIFIER_H_

#include <uapi/sel4m/object/notifier.h>

#include <asm/siginfo.h>

struct notifier_struct {
	notifier_table_t notifier_table;
	struct k_sigaction action[NOTIFIER_NR_MAX];
	__sigrestore_t return_fn;
};

struct ksignal {
	struct k_sigaction ka;
	int sig;
};

#endif /* !__SEL4M_OBJECT_NOTIFIER_H_ */
