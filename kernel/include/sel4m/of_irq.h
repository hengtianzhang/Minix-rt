#ifndef __MINIX_RT_OF_IRQ_H_
#define __MINIX_RT_OF_IRQ_H_

#include <of/of_irq.h>

extern unsigned int irq_create_of_mapping(struct of_phandle_args *irq_data);

typedef int (*of_irq_init_cb_t)(struct device_node *, struct device_node *);

void of_irq_init(const struct of_device_id *matches);

#endif /* !__MINIX_RT_OF_IRQ_H_ */
