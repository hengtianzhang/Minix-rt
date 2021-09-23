#ifndef __SEL4M_OBJECT_NOTIFIER_H_
#define __SEL4M_OBJECT_NOTIFIER_H_

#include <uapi/sel4m/object/notifier.h>

struct notifier_struct {
	notifier_table_t notifier_table;
	struct k_sigaction action[NOTIFIER_NR_MAX];
};

#endif /* !__SEL4M_OBJECT_NOTIFIER_H_ */
