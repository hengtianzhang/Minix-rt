#ifndef __SEL4M_PREEMPT_H_
#define __SEL4M_PREEMPT_H_

#define preempt_enable()
#define preempt_disable()

#define in_interrupt() 0

static inline void irq_exit(void)
{

}

static inline void irq_enter(void)
{

}

#endif /* !__SEL4M_PREEMPT_H_ */
