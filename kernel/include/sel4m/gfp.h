/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_GFP_H_
#define __SEL4M_GFP_H_

#include <base/types.h>

#include <sel4m/mm_types.h>
#include <sel4m/mmzone.h>

typedef unsigned __bitwise gfp_t;

#define ___GFP_ZERO				BIT(3)

#define __GFP_ZERO	((__force gfp_t)___GFP_ZERO)	/* Return zeroed page on success */

extern void __free_pages(struct page *page, unsigned int order);

#endif /* !__SEL4M_GFP_H_ */
