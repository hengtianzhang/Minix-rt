#ifndef __SEL4M_OF_IRQ_H_
#define __SEL4M_OF_IRQ_H_

typedef int (*of_irq_init_cb_t)(struct device_node *, struct device_node *);

void of_irq_init(const struct of_device_id *matches);
#endif /* !__SEL4M_OF_IRQ_H_ */
