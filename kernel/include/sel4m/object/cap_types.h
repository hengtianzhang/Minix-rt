#ifndef __SEL4M_OBJECT_CAP_TYPES_H_
#define __SEL4M_OBJECT_CAP_TYPES_H_

#include <base/bitmap.h>

enum cap_type {
	cap_null_cap,
	cap_cnode_cap,
	cap_frame_cap,
	cap_untyped_cap,
	cap_pgd_table_cap,
	cap_pud_table_cap,
	cap_pmd_table_cap,
	cap_pte_table_cap,
	cap_notification_cap,
	cap_endpoint_cap,
	cap_reply_cap,
	cap_irq_control_cap,
	cap_irq_handler_cap,
	cap_thread_cap,
	__cap_max_cap_end,
};

#define CAP_NR_MAX	__cap_max_cap_end

/* Don't assign or return these: may not be this big! */
typedef struct cap_table { DECLARE_BITMAP(cap_bits, CAP_NR_MAX); } cap_table_t;

#endif /* !__SEL4M_OBJECT_CAP_TYPES_H_ */
