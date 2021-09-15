#ifndef __SEL4M_PREEMPT_H_
#define __SEL4M_PREEMPT_H_

#define preempt_enable()
#define preempt_disable()

#define preempt_count() 0
#define in_interrupt() 0

#define in_atomic_preempt_off() 1

#define PREEMPT_ACTIVE		0x10000000

static inline void irq_exit(void)
{

}

static inline void irq_enter(void)
{

}

#define nmi_enter()
#define nmi_exit()

# define add_preempt_count(val)	
# define sub_preempt_count(val)	

#define preempt_enable_no_resched()

#define preemptible()				0

#endif /* !__SEL4M_PREEMPT_H_ */
