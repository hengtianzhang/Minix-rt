/*
 * Based on arch/arm/mm/ioremap.c
 *
 * (C) Copyright 1995 1996 Linus Torvalds
 * Hacked for ARM by Phil Blundell <philb@gnu.org>
 * Hacked to allow all architectures to build, and various cleanups
 * by Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <base/compiler.h>

#include <asm/io.h>
#include <asm/memory.h>

#define MAX_IOREMAP_SIZE = VIOMAP_START + VIOMAP_SIZE - 1;

u64 current_ioremap_base = VIOMAP_START;

void __iounmap(volatile void __iomem *addr)
{
	printf("__iounmap: Current not support unmap IO area!\n");
	BUG();
}

void __iomem *ioremap_cache(phys_addr_t phys_addr, size_t size)
{
	printf("ioremap_cache: Current not support IO map cache!\n");
	BUG();
}

void __iomem *__ioremap(phys_addr_t phys_addr, size_t size, pgprot_t prot)
{
	const u64 virt_base = current_ioremap_base;
	int offset;
	void *virt;
	int ret;

	if (phys_addr % sizeof (u64))
		return NULL;

	BUG_ON(virt_base % SZ_2M);

	offset = phys_addr % PAGE_SIZE;
	virt = (void *)virt_base + offset;

	ret = __create_iomap_remap(round_down(phys_addr, PAGE_SIZE),
			virt_base, round_up(size, PAGE_SIZE), prot, 0);
	
	if (ret)
		return NULL;

	current_ioremap_base += round_up(round_up(size, PAGE_SIZE), SZ_2M);

	return virt;
}

void __iomem *ioremap(phys_addr_t phys_addr, size_t size)
{
	return __ioremap(phys_addr, size, __pgprot(PROT_DEVICE_nGnRE));
}
