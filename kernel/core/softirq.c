
#include <sel4m/interrupt.h>
#include <sel4m/preempt.h>
#include <sel4m/irq.h>

void raise_softirq(unsigned int nr)
{
}

void open_softirq(int nr, void (*action)(struct softirq_action *), struct softirq_action *h)
{

}

/*
 * Call a function on all processors
 */
int on_each_cpu(void (*func) (void *info), void *info, int retry, int wait)
{
	int ret = 0;

	preempt_disable();
//	ret = smp_call_function(func, info, retry, wait);
	local_irq_disable();
	func(info);
	local_irq_enable();
	preempt_enable();
	return ret;
}
