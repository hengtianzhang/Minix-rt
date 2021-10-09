// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 1992, 1998-2006 Linus Torvalds, Ingo Molnar
 * Copyright (C) 2005-2006, Thomas Gleixner, Russell King
 *
 * This file contains the core interrupt handling code. Detailed
 * information is available in Documentation/core-api/genericirq.rst
 *
 */
#include <base/types.h>
#include <base/cache.h>

#include <of/of.h>

#include <minix_rt/irq.h>
#include <minix_rt/interrupt.h>
#include <minix_rt/smp.h>

#include <asm/ptrace.h>

void (*handle_arch_irq)(struct pt_regs *) __ro_after_init;

static struct irq_data irq_data_0[MAX_IRQ_ID + 1] = {0};
static struct irq_domain domain_0 = {
	.name = "IrqDomain 0",
	.flags = 0,
	.mapcount = 0,
	.irq_data = irq_data_0,
	.parent = NULL,
	.hwirq_max = MAX_IRQ_ID,
};

struct pt_regs *__irq_regs[CONFIG_NR_CPUS];

struct pt_regs *get_irq_regs(void)
{
	return __irq_regs[smp_processor_id()];
}

struct pt_regs *set_irq_regs(struct pt_regs *new_regs)
{
	struct pt_regs *old_regs;

	old_regs = __irq_regs[smp_processor_id()];
	__irq_regs[smp_processor_id()] = new_regs;

	return old_regs;
}

static u32 irq_none_counter = 0;

int handle_domain_irq(struct irq_domain *domain,
				    unsigned int hwirq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	struct irq_data *irqd = NULL;
	int cpu, ret;

	BUG_ON(hwirq > MAX_IRQ_ID);
	BUG_ON(hwirq > domain->hwirq_max);
	BUG_ON(domain != &domain_0);

	irq_enter();

	irqd = &irq_data_0[hwirq];
	cpu = smp_processor_id();
	if (unlikely(!cpumask_test_cpu(cpu, &irqd->cpumask))) {
		irq_none_counter++;
		goto none;
	}

	if (cpumask_weight(&irqd->cpumask) > 1) {
		if (irqd->chip->irq_ack)
			irqd->chip->irq_ack(irqd);
		
		if (irqd->handler.handler)
			ret = irqd->handler.handler(hwirq, irqd->percpu_dev_id + (irqd->percpu_size * smp_processor_id()));
		else {
			irq_none_counter++;
			goto none;
		}

		if (irqd->chip->irq_eoi)
			irqd->chip->irq_eoi(irqd);
	} else {
		if (irqd->handler.handler)
			ret = irqd->handler.handler(hwirq, irqd->handler.arg);
		else {
			irq_none_counter++;
			goto none;
		}
	}

	if (unlikely(ret != IRQ_HANDLED)) {
		irq_none_counter++;
		goto none;
	}

	set_irq_regs(old_regs);
	irq_exit();

	return 0;

none:
	if (irq_none_counter >= 100) {
		irq_none_counter = 0;
		mask_irq(irqd);
	}

	set_irq_regs(old_regs);

	WARN(1, "IRQ %d over max irqnr!\n", hwirq);
	irq_exit();

	return -1;
}

int __init set_handle_irq(void (*handle_irq)(struct pt_regs *))
{
	if (handle_arch_irq)
		return -EBUSY;

	handle_arch_irq = handle_irq;
	return 0;
}

struct irq_data *handle_get_irq_data(int hwirq)
{
	if (0 < hwirq && hwirq < MAX_IRQ_ID + 1)
		return &irq_data_0[hwirq];

	return NULL;
}

struct irq_domain *handle_get_irq_domain(void)
{
	return &domain_0;
}

static void of_phandle_args_to_fwspec(struct of_phandle_args *irq_data,
				      struct irq_fwspec *fwspec)
{
	int i;

	fwspec->param_count = irq_data->args_count;

	for (i = 0; i < irq_data->args_count; i++)
		fwspec->param[i] = irq_data->args[i];
}

static int irq_domain_translate(struct irq_domain *d,
				struct irq_fwspec *fwspec,
				irq_hw_number_t *hwirq, unsigned int *type)
{
	if (d->ops->translate)
		return d->ops->translate(d, fwspec, hwirq, type);
	
	return 0;
}

unsigned int irq_create_fwspec_mapping(struct irq_fwspec *fwspec)
{
	struct irq_domain *domain;
	struct irq_data *irq_data;
	irq_hw_number_t hwirq;
	unsigned int type = IRQ_TYPE_NONE;

	domain = &domain_0;

	if (irq_domain_translate(domain, fwspec, &hwirq, &type))
		return 0;

	/*
	 * WARN if the irqchip returns a type with bits
	 * outside the sense mask set and clear these bits.
	 */
	if (WARN_ON(type & ~IRQ_TYPE_SENSE_MASK))
		type &= IRQ_TYPE_SENSE_MASK;

	if (hwirq > domain->hwirq_max)
		return -1;

	irq_data = &irq_data_0[hwirq];
	if (irq_data->used)
		return -1;

	cpumask_clear(&irq_data->cpumask);
	irq_data->irq = hwirq;
	irq_data->hwirq = hwirq;
	irq_data->chip = domain->chip;
	irq_data->domain = domain;
	irq_data->parent_data = NULL;
	irq_data->used = 1;
	irq_data->handler.irqflags = type;
	irq_data->chip_data = domain->pri_data;

	mask_irq(irq_data);

	return hwirq;
}

unsigned int irq_create_of_mapping(struct of_phandle_args *irq_data)
{
	struct irq_fwspec fwspec;

	of_phandle_args_to_fwspec(irq_data, &fwspec);

	return irq_create_fwspec_mapping(&fwspec);
}

int request_percpu_irq(unsigned int irq, irq_handler_t handler,
		   const char *devname, void __percpu *percpu_dev_id, int percpu_size)
{
	struct irq_domain *domain = &domain_0;
	struct irq_data *irq_data;

	if (irq > domain->hwirq_max)
		return -1;

	irq_data = &irq_data_0[irq];
	if (!irq_data->used || irq_data->hwirq != irq || !handler)
		return -1;

	irq_data->name = devname;
	cpumask_setall(&irq_data->cpumask);
	irq_set_type(irq_data, irq_data->handler.irqflags);
	irq_set_affinity(irq_data, &irq_data->cpumask, 1);
	unmask_irq(irq_data);
	irq_data->percpu_dev_id = percpu_dev_id;
	irq_data->percpu_size = percpu_size;
	irq_data->handler.handler = handler;

	return 0;
}

void enable_percpu_irq(unsigned int irq)
{
	struct irq_domain *domain = &domain_0;
	struct irq_data *irq_data;

	if (irq > domain->hwirq_max)
		return ;

	irq_data = &irq_data_0[irq];
	if (!irq_data->used || irq_data->hwirq != irq)
		return ;

	irq_set_type(irq_data, irq_data->handler.irqflags);
	irq_set_affinity(irq_data, &irq_data->cpumask, 1);

	if (irq_data->chip->irq_enable)
		irq_data->chip->irq_enable(irq_data);
	else
		irq_data->chip->irq_unmask(irq_data);
}

void irq_percpu_disable(struct irq_data *irqd, unsigned int cpu)
{
	if (irqd->chip->irq_disable)
		irqd->chip->irq_disable(irqd);
	else
		irqd->chip->irq_mask(irqd);
}

void mask_irq(struct irq_data *d)
{
	if (d->chip->irq_mask)
		d->chip->irq_mask(d);
}

void unmask_irq(struct irq_data *d)
{
	if (d->chip->irq_unmask)
		d->chip->irq_unmask(d);
}

void irq_eoi(struct irq_data *d)
{
	if (d->chip->irq_eoi)
		d->chip->irq_eoi(d);
}

int irq_set_type(struct irq_data *d, unsigned int type)
{
	int ret = 0;

	if (d->chip->irq_set_type)
		ret = d->chip->irq_set_type(d, type);

	return ret;
}
/**
 *	irq_set_type - set the irq trigger type for an irq
 *	@irq:	irq number
 *	@type:	IRQ_TYPE_{LEVEL,EDGE}_* value - see include/linux/irq.h
 */
int irq_set_irq_type(unsigned int irq, unsigned int type)
{
	struct irq_data *irqd = handle_get_irq_data(irq);

	if (!irqd)
		return -EINVAL;

	return irq_set_type(irqd, type);
}

int irq_get_irqchip_state(struct irq_data *d,
				      enum irqchip_irq_state which, bool *val)
{
	int ret = 0;

	if (d->chip->irq_get_irqchip_state)
		ret = d->chip->irq_get_irqchip_state(d, which, val);

	return ret;
}

int irq_set_irqchip_state(struct irq_data *d,
				     enum irqchip_irq_state which, bool val)
{
	int ret = 0;

	if (d->chip->irq_set_irqchip_state)
		ret = d->chip->irq_set_irqchip_state(d, which, val);

	return ret;
}

int irq_set_affinity(struct irq_data *d, const struct cpumask *mask_val,
			    bool force)
{
	int ret = 0;

	if (d->chip->irq_set_affinity)
		ret = d->chip->irq_set_affinity(d, mask_val, force);

	return ret;
}
