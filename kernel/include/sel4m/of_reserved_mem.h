/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_OF_RESERVED_MEM_H_
#define __SEL4M_OF_RESERVED_MEM_H_

#include <base/types.h>

struct reserved_mem {
	const char			*name;
	unsigned long			fdt_node;
	unsigned long			phandle;
	phys_addr_t			base;
	phys_addr_t			size;
	void				*priv;
};

int early_init_dt_alloc_reserved_memory_arch(phys_addr_t size,
	phys_addr_t align, phys_addr_t start, phys_addr_t end, bool nomap,
	phys_addr_t *res_base);
void fdt_reserved_mem_save_node(unsigned long node, const char *uname,
				      phys_addr_t base, phys_addr_t size);
void fdt_init_reserved_mem(void);

#endif /* !__SEL4M_OF_RESERVED_MEM_H_ */
