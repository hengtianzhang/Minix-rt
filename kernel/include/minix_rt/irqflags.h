/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/linux/irqflags.h
 *
 * IRQ flags tracing: follow the state of the hardirq and softirq flags and
 * provide callbacks for transitions between ON and OFF states.
 *
 * This file gets included from lowlevel asm headers too, to provide
 * wrapped versions of the local_irq_*() APIs, based on the
 * raw_local_irq_*() macros from the lowlevel headers.
 */
#ifndef __MINIX_RT_IRQFLAGS_H_
#define __MINIX_RT_IRQFLAGS_H_

#include <base/typecheck.h>

#include <asm/irqflags.h>

/*
 * Wrap the arch provided IRQ routines to provide appropriate checks.
 */
#define raw_local_irq_disable()		arch_local_irq_disable()
#define raw_local_irq_enable()		arch_local_irq_enable()

#define raw_local_irq_save(flags)			\
	do {						\
		typecheck(u64, flags);	\
		flags = arch_local_irq_save();		\
	} while (0)
#define raw_local_irq_restore(flags)			\
	do {						\
		typecheck(u64, flags);	\
		arch_local_irq_restore(flags);		\
	} while (0)

#define raw_local_save_flags(flags)			\
	do {						\
		typecheck(u64, flags);	\
		flags = arch_local_save_flags();	\
	} while (0)

#define raw_irqs_disabled_flags(flags)			\
	({						\
		typecheck(u64, flags);	\
		arch_irqs_disabled_flags(flags);	\
	})

#define raw_irqs_disabled()		(arch_irqs_disabled())

#define local_irq_disable()	do { raw_local_irq_disable(); } while (0)
#define local_irq_enable()	do { raw_local_irq_enable(); } while (0)

#define local_irq_save(flags) do { raw_local_irq_save(flags); } while (0)
#define local_irq_restore(flags) do { raw_local_irq_restore(flags); } while (0)

#define local_save_flags(flags)	raw_local_save_flags(flags)

#define irqs_disabled()	raw_irqs_disabled()

#define irqs_disabled_flags(flags) raw_irqs_disabled_flags(flags)

#endif /* !__MINIX_RT_IRQFLAGS_H_ */
