/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __OF_OF_IRQ_H_
#define __OF_OF_IRQ_H_

#include <base/types.h>
#include <base/errno.h>

#include <of/ioport.h>
#include <of/of.h>

extern int of_irq_parse_raw(const __be32 *addr, struct of_phandle_args *out_irq);
extern int of_irq_parse_one(struct device_node *device, int index,
			  struct of_phandle_args *out_irq);
extern int irq_of_parse_and_map(struct device_node *dev, int index,
			  struct of_phandle_args *oirq);
extern struct device_node *of_irq_find_parent(struct device_node *child);

#endif /* !__OF_OF_IRQ_H_ */
