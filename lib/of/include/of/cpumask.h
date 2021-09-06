#ifndef __OF_CPUMASK_H_
#define __OF_CPUMASK_H_

#include <base/bitmap.h>

#ifndef __KERNEL__

/* Don't assign or return these: may not be this big! */
typedef struct cpumask { DECLARE_BITMAP(bits, CONFIG_NR_CPUS); } cpumask_t;

#define for_each_possible_cpu(cpu)	\
	for ((cpu) = 0;	\
		(cpu) < CONFIG_NR_CPUS;	\
		(cpu)++)

#endif /* !__KERNEL__ */
#endif /* !__OF_CPUMASK_H_ */
