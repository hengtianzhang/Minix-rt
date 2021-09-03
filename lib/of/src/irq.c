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
#include <base/errno.h>

#include <of/of_irq.h>
#include <of/of.h>

/**
 * irq_of_parse_and_map - Parse and map an interrupt into linux virq space
 * @dev: Device node of the device whose interrupt is to be mapped
 * @index: Index of the interrupt to map
 *
 * This function is a wrapper that chains of_irq_parse_one() and
 * irq_create_of_mapping() to make things easier to callers
 */
int irq_of_parse_and_map(struct device_node *dev, int index, struct of_phandle_args *oirq)
{
	return of_irq_parse_one(dev, index, oirq);
}

/**
 * of_irq_find_parent - Given a device node, find its interrupt parent node
 * @child: pointer to device node
 *
 * Returns a pointer to the interrupt parent node, or NULL if the interrupt
 * parent could not be determined.
 */
struct device_node *of_irq_find_parent(struct device_node *child)
{
	struct device_node *p;
	phandle parent;

	if (!of_node_get(child))
		return NULL;

	do {
		if (of_property_read_u32(child, "interrupt-parent", &parent)) {
			p = of_get_parent(child);
		} else	{
			p = of_find_node_by_phandle(parent);
		}
		child = p;
	} while (p && of_get_property(p, "#interrupt-cells", NULL) == NULL);

	return p;
}

/**
 * of_irq_parse_raw - Low level interrupt tree parsing
 * @addr:	address specifier (start of "reg" property of the device) in be32 format
 * @out_irq:	structure of_phandle_args updated by this function
 *
 * Returns 0 on success and a negative number on error
 *
 * This function is a low-level interrupt tree walking function. It
 * can be used to do a partial walk with synthetized reg and interrupts
 * properties, for example when resolving PCI interrupts when no device
 * node exist for the parent. It takes an interrupt specifier structure as
 * input, walks the tree looking for any interrupt-map properties, translates
 * the specifier for each map, and then returns the translated map.
 */
int of_irq_parse_raw(const __be32 *addr, struct of_phandle_args *out_irq)
{
	struct device_node *ipar, *tnode, *old = NULL, *newpar = NULL;
	__be32 initial_match_array[MAX_PHANDLE_ARGS];
	const __be32 *match_array = initial_match_array;
	const __be32 *tmp, *imap, *imask, dummy_imask[] = { [0 ... MAX_PHANDLE_ARGS] = cpu_to_be32(~0) };
	u32 intsize = 1, addrsize, newintsize = 0, newaddrsize = 0;
	int imaplen, match, i, rc = -EINVAL;

#ifdef DEBUG
	of_print_phandle_args("of_irq_parse_raw: ", out_irq);
#endif

	ipar = of_node_get(out_irq->np);

	/* First get the #interrupt-cells property of the current cursor
	 * that tells us how to interpret the passed-in intspec. If there
	 * is none, we are nice and just walk up the tree
	 */
	do {
		if (!of_property_read_u32(ipar, "#interrupt-cells", &intsize))
			break;
		tnode = ipar;
		ipar = of_irq_find_parent(ipar);
	} while (ipar);
	if (ipar == NULL) {
		of_dbg(" -> no parent found !\n");
		goto fail;
	}

	of_dbg("of_irq_parse_raw: ipar=%pOF, size=%d\n", ipar, intsize);

	if (out_irq->args_count != intsize)
		goto fail;

	/* Look for this #address-cells. We have to implement the old linux
	 * trick of looking for the parent here as some device-trees rely on it
	 */
	old = of_node_get(ipar);
	do {
		tmp = of_get_property(old, "#address-cells", NULL);
		tnode = of_get_parent(old);
		old = tnode;
	} while (old && tmp == NULL);
	old = NULL;
	addrsize = (tmp == NULL) ? 2 : be32_to_cpu(*tmp);

	of_dbg(" -> addrsize=%d\n", addrsize);

	/* Range check so that the temporary buffer doesn't overflow */
	if (WARN_ON(addrsize + intsize > MAX_PHANDLE_ARGS)) {
		rc = -EFAULT;
		goto fail;
	}

	/* Precalculate the match array - this simplifies match loop */
	for (i = 0; i < addrsize; i++)
		initial_match_array[i] = addr ? addr[i] : 0;
	for (i = 0; i < intsize; i++)
		initial_match_array[addrsize + i] = cpu_to_be32(out_irq->args[i]);

	/* Now start the actual "proper" walk of the interrupt tree */
	while (ipar != NULL) {
		/* Now check if cursor is an interrupt-controller and if it is
		 * then we are done
		 */
		if (of_property_read_bool(ipar, "interrupt-controller")) {
			of_dbg(" -> got it !\n");
			return 0;
		}

		/*
		 * interrupt-map parsing does not work without a reg
		 * property when #address-cells != 0
		 */
		if (addrsize && !addr) {
			of_dbg(" -> no reg passed in when needed !\n");
			goto fail;
		}

		/* Now look for an interrupt-map */
		imap = of_get_property(ipar, "interrupt-map", &imaplen);
		/* No interrupt map, check for an interrupt parent */
		if (imap == NULL) {
			of_dbg(" -> no map, getting parent\n");
			newpar = of_irq_find_parent(ipar);
			goto skiplevel;
		}
		imaplen /= sizeof(u32);

		/* Look for a mask */
		imask = of_get_property(ipar, "interrupt-map-mask", NULL);
		if (!imask)
			imask = dummy_imask;

		/* Parse interrupt-map */
		match = 0;
		while (imaplen > (addrsize + intsize + 1) && !match) {
			/* Compare specifiers */
			match = 1;
			for (i = 0; i < (addrsize + intsize); i++, imaplen--)
				match &= !((match_array[i] ^ *imap++) & imask[i]);

			of_dbg(" -> match=%d (imaplen=%d)\n", match, imaplen);

			newpar = of_find_node_by_phandle(be32_to_cpup(imap));
			imap++;
			--imaplen;

			/* Check if not found */
			if (newpar == NULL) {
				of_dbg(" -> imap parent not found !\n");
				goto fail;
			}

			if (!of_device_is_available(newpar))
				match = 0;

			/* Get #interrupt-cells and #address-cells of new
			 * parent
			 */
			if (of_property_read_u32(newpar, "#interrupt-cells",
						 &newintsize)) {
				of_dbg(" -> parent lacks #interrupt-cells!\n");
				goto fail;
			}
			if (of_property_read_u32(newpar, "#address-cells",
						 &newaddrsize))
				newaddrsize = 0;

			of_dbg(" -> newintsize=%d, newaddrsize=%d\n",
			    newintsize, newaddrsize);

			/* Check for malformed properties */
			if (WARN_ON(newaddrsize + newintsize > MAX_PHANDLE_ARGS)
			    || (imaplen < (newaddrsize + newintsize))) {
				rc = -EFAULT;
				goto fail;
			}

			imap += newaddrsize + newintsize;
			imaplen -= newaddrsize + newintsize;

			of_dbg(" -> imaplen=%d\n", imaplen);
		}
		if (!match)
			goto fail;

		/*
		 * Successfully parsed an interrrupt-map translation; copy new
		 * interrupt specifier into the out_irq structure
		 */
		match_array = imap - newaddrsize - newintsize;
		for (i = 0; i < newintsize; i++)
			out_irq->args[i] = be32_to_cpup(imap - newintsize + i);
		out_irq->args_count = intsize = newintsize;
		addrsize = newaddrsize;

	skiplevel:
		/* Iterate again with new parent */
		out_irq->np = newpar;
		of_dbg(" -> new parent: %pOF\n", newpar);
		ipar = newpar;
		newpar = NULL;
	}
	rc = -ENOENT; /* No interrupt-map found */

 fail:

	return rc;
}

/**
 * of_irq_parse_one - Resolve an interrupt for a device
 * @device: the device whose interrupt is to be resolved
 * @index: index of the interrupt to resolve
 * @out_irq: structure of_irq filled by this function
 *
 * This function resolves an interrupt for a node by walking the interrupt tree,
 * finding which interrupt controller node it is attached to, and returning the
 * interrupt specifier that can be used to retrieve a Linux IRQ number.
 */
int of_irq_parse_one(struct device_node *device, int index, struct of_phandle_args *out_irq)
{
	struct device_node *p;
	const __be32 *addr;
	u32 intsize;
	int i, res;

	of_dbg("of_irq_parse_one: dev=%pOF, index=%d\n", device, index);

	/* Get the reg property (if any) */
	addr = of_get_property(device, "reg", NULL);

	/* Try the new-style interrupts-extended first */
	res = of_parse_phandle_with_args(device, "interrupts-extended",
					"#interrupt-cells", index, out_irq);
	if (!res)
		return of_irq_parse_raw(addr, out_irq);

	/* Look for the interrupt parent. */
	p = of_irq_find_parent(device);
	if (p == NULL)
		return -EINVAL;

	/* Get size of interrupt specifier */
	if (of_property_read_u32(p, "#interrupt-cells", &intsize)) {
		res = -EINVAL;
		goto out;
	}

	of_dbg(" parent=%pOF, intsize=%d\n", p, intsize);

	/* Copy intspec into irq structure */
	out_irq->np = p;
	out_irq->args_count = intsize;
	for (i = 0; i < intsize; i++) {
		res = of_property_read_u32_index(device, "interrupts",
						 (index * intsize) + i,
						 out_irq->args + i);
		if (res)
			goto out;
	}

	of_dbg(" intspec=%d\n", *out_irq->args);


	/* Check if there are any interrupt-map translations to process */
	res = of_irq_parse_raw(addr, out_irq);
 out:

	return res;
}
