#include <base/cache.h>

#include <minix_rt/interrupt.h>
#include <minix_rt/preempt.h>
#include <minix_rt/irq.h>
#include <minix_rt/smp.h>
#include <minix_rt/ktime.h>

static struct softirq_action softirq_vec[NR_SOFTIRQS] __cacheline_aligned_in_smp;

static u64 __read_mostly softirq_pending[CONFIG_NR_CPUS] = {[0 ... CONFIG_NR_CPUS - 1] = 0};

void raise_softirq(unsigned int nr)
{
	u64 flags;

	local_irq_save(flags);
	softirq_pending[smp_processor_id()] |= BIT(nr);
	local_irq_restore(flags);
	
}

void open_softirq(int nr, void (*action)(struct softirq_action *), struct softirq_action *h)
{
	softirq_vec[nr].action = action;
}

/*
 * Special-case - softirqs can safely be enabled by __do_softirq(),
 * without processing still-pending softirqs:
 */
void __local_bh_enable(void)
{
	WARN_ON_ONCE(in_irq());
	__preempt_count_sub(SOFTIRQ_DISABLE_OFFSET);
}

void __local_bh_disable(void)
{
	__preempt_count_add(SOFTIRQ_DISABLE_OFFSET);
	barrier();
}

static int local_softirq_pending(void)
{
	return softirq_pending[smp_processor_id()] != 0;
}

#define MAX_SOFTIRQ_TIME  msecs_to_jiffies(2)
#define MAX_SOFTIRQ_RESTART 10

void __do_softirq(void)
{
	if (!in_interrupt() && local_softirq_pending()) {
		u64 end = jiffies + MAX_SOFTIRQ_TIME;
		int max_restart = MAX_SOFTIRQ_RESTART;
		int softirq_bit;
		u64 *pending;
		struct softirq_action *h;

		pending = &softirq_pending[smp_processor_id()];
		local_bh_disable();
restart:
		BUG_ON(!irqs_disabled());
		local_irq_enable();

		h = softirq_vec;

		while ((softirq_bit = ffs(*pending))) {
			unsigned int vec_nr;
			int prev_count;

			h += softirq_bit - 1;

			vec_nr = h - softirq_vec;
			prev_count = preempt_count();

			if (softirq_vec[softirq_bit - 1].action)
				softirq_vec[softirq_bit - 1].action(NULL);

			if (unlikely(prev_count != preempt_count())) {
				printf("huh, entered softirq with preempt_count %08x, exited with %08x?\n",
						prev_count, preempt_count());
				preempt_count_set(prev_count);
			}
			h++;
			*pending >>= softirq_bit;
		}

		local_irq_disable();

		*pending = local_softirq_pending();
		if (*pending) {
			if (time_before(jiffies, end) && !need_resched() &&
		   	 --max_restart)
				goto restart;
		}
		local_bh_enable();

		WARN_ON_ONCE(in_interrupt());
	}
}
