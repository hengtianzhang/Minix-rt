/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_EXTABLE_H_
#define __SEL4M_EXTABLE_H_

#include <base/stddef.h>	/* for NULL */
#include <base/types.h>

#include <asm/extable.h>

struct exception_table_entry;

const struct exception_table_entry *
search_extable(const struct exception_table_entry *base,
	       const size_t num,
	       unsigned long value);
void sort_extable(struct exception_table_entry *start,
		  struct exception_table_entry *finish);
void sort_main_extable(void);

/* Given an address, look for it in the exception tables */
const struct exception_table_entry *search_exception_tables(unsigned long add);

#endif /* !__SEL4M_EXTABLE_H_ */
