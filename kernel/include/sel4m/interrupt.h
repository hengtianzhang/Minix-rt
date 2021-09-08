/* SPDX-License-Identifier: GPL-2.0 */
/* interrupt.h */
#ifndef __SEL4M_INTERRUPT_H_
#define __SEL4M_INTERRUPT_H_

/*
 * irq_get_irqchip_state/irq_set_irqchip_state specific flags
 */
enum irqchip_irq_state {
	IRQCHIP_STATE_PENDING,		/* Is interrupt pending? */
	IRQCHIP_STATE_ACTIVE,		/* Is interrupt in progress? */
	IRQCHIP_STATE_MASKED,		/* Is interrupt masked? */
	IRQCHIP_STATE_LINE_LEVEL,	/* Is IRQ line high? */
};

/*
 * These correspond to the IORESOURCE_IRQ_* defines in
 * linux/ioport.h to select the interrupt line behaviour.  When
 * requesting an interrupt without specifying a IRQF_TRIGGER, the
 * setting should be assumed to be "as already configured", which
 * may be as per machine or firmware initialisation.
 */
#define IRQF_TRIGGER_NONE	0x00000000
#define IRQF_TRIGGER_RISING	0x00000001
#define IRQF_TRIGGER_FALLING	0x00000002
#define IRQF_TRIGGER_HIGH	0x00000004
#define IRQF_TRIGGER_LOW	0x00000008
#define IRQF_TRIGGER_MASK	(IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW | \
				 IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)
#define IRQF_TRIGGER_PROBE	0x00000010

#define MAX_IRQ_ID 1024

extern void __local_bh_enable(void);
static inline void local_bh_enable(void)
{
	__local_bh_enable();
}

extern void __local_bh_disable(void);
static inline void local_bh_disable(void)
{
	__local_bh_disable();
}

enum
{
	HI_SOFTIRQ=0,
	TIMER_SOFTIRQ,
	NET_TX_SOFTIRQ,
	NET_RX_SOFTIRQ,
	BLOCK_SOFTIRQ,
	BLOCK_IOPOLL_SOFTIRQ,
	TASKLET_SOFTIRQ,
	SCHED_SOFTIRQ,
	HRTIMER_SOFTIRQ,
	RCU_SOFTIRQ,    /* Preferable RCU should always be the last softirq */

	NR_SOFTIRQS
};

struct softirq_action {
	void	(*action)(void);
};

extern void open_softirq(int nr, void (*action)(void));
extern void __do_softirq(void);
extern void raise_softirq(unsigned int nr);
#endif /* !__SEL4M_INTERRUPT_H_ */
