// SPDX-License-Identifier: GPL-2.0+
/*
 *  Derived from arch/i386/kernel/irq.c
 *    Copyright (C) 1992 Linus Torvalds
 *  Adapted from arch/i386 by Gary Thomas
 *    Copyright (C) 1995-1996 Gary Thomas (gdt@linuxppc.org)
 *  Updated and modified by Cort Dougan <cort@fsmlabs.com>
 *    Copyright (C) 1996-2001 Cort Dougan
 *  Adapted for Power Macintosh by Paul Mackerras
 *    Copyright (C) 1996 Paul Mackerras (paulus@cs.anu.edu.au)
 *
 * This file contains the code used to make IRQ descriptions in the
 * device tree to actual irq numbers on an interrupt controller
 * driver.
 */
#include <base/string.h>
#include <base/list.h>
#include <base/init.h>

#include <of/of.h>
#include <of/of_irq.h>

#include <sel4m/of_irq.h>

struct of_intc_desc {
	struct list_head	list;
	of_irq_init_cb_t	irq_init_cb;
	struct device_node	*dev;
	struct device_node	*interrupt_parent;
};

/**
 * of_irq_init - Scan and init matching interrupt controllers in DT
 * @matches: 0 terminated array of nodes to match and init function to call
 *
 * This function scans the device tree for matching interrupt controller nodes,
 * and calls their initialization functions in order with parents first.
 */
void __init of_irq_init(const struct of_device_id *matches)
{
}
