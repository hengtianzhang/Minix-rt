/*
 * Copyright (C) 2012 Thomas Petazzoni
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __MINIX_RT_IRQCHIP_H_
#define __MINIX_RT_IRQCHIP_H_

#include <of/of.h>

/*
 * This macro must be used by the different irqchip drivers to declare
 * the association between their DT compatible string and their
 * initialization function.
 *
 * @name: name that must be unique across all IRQCHIP_DECLARE of the
 * same file.
 * @compstr: compatible string of the irqchip driver
 * @fn: initialization function
 */
#define IRQCHIP_DECLARE(name, compat, fn) OF_DECLARE_2(irqchip, name, compat, fn)

#ifdef CONFIG_IRQCHIP
void irqchip_init(void);
#else
static inline void irqchip_init(void) {}
#endif

#endif /* !__MINIX_RT_IRQCHIP_H_ */
