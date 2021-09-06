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

#include <sel4m/memory.h>
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
	const struct of_device_id *match;
	struct device_node *np, *parent = NULL;
	struct of_intc_desc *desc, *temp_desc;
	struct list_head intc_desc_list, intc_parent_list;

	INIT_LIST_HEAD(&intc_desc_list);
	INIT_LIST_HEAD(&intc_parent_list);

	for_each_matching_node_and_match(np, matches, &match) {
		if (!of_property_read_bool(np, "interrupt-controller") ||
				!of_device_is_available(np))
			continue;

		if (WARN(!match->data, "of_irq_init: no init function for %s\n",
			 match->compatible))
			continue;

		/*
		 * Here, we allocate and populate an of_intc_desc with the node
		 * pointer, interrupt-parent device_node etc.
		 */
		desc = memblock_alloc_virt(&memblock_kernel, sizeof(*desc), __alignof__(struct of_intc_desc));
		if (WARN_ON(!desc)) {
			goto err;
		}

		desc->irq_init_cb = match->data;
		desc->dev = of_node_get(np);
		desc->interrupt_parent = of_irq_find_parent(np);
		if (desc->interrupt_parent == np)
			desc->interrupt_parent = NULL;
		list_add_tail(&desc->list, &intc_desc_list);
	}

	/*
	 * The root irq controller is the one without an interrupt-parent.
	 * That one goes first, followed by the controllers that reference it,
	 * followed by the ones that reference the 2nd level controllers, etc.
	 */
	while (!list_empty(&intc_desc_list)) {
		/*
		 * Process all controllers with the current 'parent'.
		 * First pass will be looking for NULL as the parent.
		 * The assumption is that NULL parent means a root controller.
		 */
		list_for_each_entry_safe(desc, temp_desc, &intc_desc_list, list) {
			int ret;

			if (desc->interrupt_parent != parent)
				continue;
			
			list_del(&desc->list);

			of_node_set_flag(desc->dev, OF_POPULATED);

			pr_debug("of_irq_init: init %pOF (%p), parent %p\n",
				 desc->dev,
				 desc->dev, desc->interrupt_parent);
			ret = desc->irq_init_cb(desc->dev,
						desc->interrupt_parent);
			if (ret) {
				of_node_clear_flag(desc->dev, OF_POPULATED);
				memblock_free(&memblock_kernel, __pa(desc), sizeof(*desc));
				continue;
			}

			/*
			 * This one is now set up; add it to the parent list so
			 * its children can get processed in a subsequent pass.
			 */
			list_add_tail(&desc->list, &intc_parent_list);
		}

		/* Get the next pending parent that might have children */
		desc = list_first_entry_or_null(&intc_parent_list,
						typeof(*desc), list);
		if (!desc) {
			printf("of_irq_init: children remain, but no parents\n");
			break;
		}
		list_del(&desc->list);
		parent = desc->dev;
		memblock_free(&memblock_kernel, __pa(desc), sizeof(*desc));
	}

	list_for_each_entry_safe(desc, temp_desc, &intc_parent_list, list) {
		list_del(&desc->list);
		memblock_free(&memblock_kernel, __pa(desc), sizeof(*desc));
	}
err:
	list_for_each_entry_safe(desc, temp_desc, &intc_desc_list, list) {
		list_del(&desc->list);
		memblock_free(&memblock_kernel, __pa(desc), sizeof(*desc));
	}
}
