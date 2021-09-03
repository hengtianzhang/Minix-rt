/*
 * Based on arch/arm/kernel/setup.c
 *
 * Copyright (C) 1995-2001 Russell King
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
#include <base/init.h>
#include <base/cache.h>

#include <base/compiler.h>
#include <base/types.h>

#include <sel4m/memory.h>
#include <sel4m/of_fdt.h>

#include <asm/base/processor.h>

#include <asm/mmu.h>
#include <asm/fixmap.h>
#include <asm/pgtable.h>

phys_addr_t __fdt_pointer __initdata;

static void __init setup_machine_fdt(phys_addr_t dt_phys)
{
	void *dt_virt = fixmap_remap_fdt(dt_phys);
	const char *name;

	if (!dt_virt || !early_init_dt_scan(dt_virt)) {
		printf("\n"
			"Error: invalid device tree blob at physical address 0x%llx (virtual address 0x%p)\n"
			"The dtb must be 8-byte aligned and must not exceed 2 MB in size\n"
			"\nPlease check your bootloader.",
			dt_phys, dt_virt);

		while (true)
			cpu_relax();
	}

	name = of_flat_dt_get_machine_name();
	if (!name)
		return;

	printf("Machine model: %s\n", name);
}

/*
 * The recorded values of x0 .. x3 upon kernel entry.
 */
u64 __cacheline_aligned boot_args[4];

void __init early_arch_platform_init(void)
{
    memblock_init(&memblock_kernel);

    early_fixmap_init();

    setup_machine_fdt(__fdt_pointer);
}
