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
#include <sel4m/psci.h>
#include <sel4m/stat.h>

#include <asm/base/processor.h>

#include <asm/daifflags.h>
#include <asm/smp.h>
#include <asm/cputype.h>
#include <asm/mmu.h>
#include <asm/fixmap.h>
#include <asm/pgtable.h>
#include <asm/mmu_context.h>
#include <asm/cpu_ops.h>

phys_addr_t __fdt_pointer __initdata;

u64 __cpu_logical_map[CONFIG_NR_CPUS] = { [0 ... CONFIG_NR_CPUS-1] = INVALID_HWID };

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
	dump_stack_set_arch_desc("%s (DT)", name);
}

static void __init setup_fixmap_console(void)
{
	early_init_dt_scan_chosen_stdout();
}

/*
 * The recorded values of x0 .. x3 upon kernel entry.
 */
u64 __cacheline_aligned boot_args[4];

void __init smp_setup_processor_id(void)
{
	u64 mpidr = read_cpuid_mpidr() & MPIDR_HWID_BITMASK;
	cpu_logical_map(0) = mpidr;

	/*
	 * clear __my_cpu_offset on boot CPU to avoid hang caused by
	 * using percpu variable early, for example, lockdep will
	 * access percpu variable inside lock_release
	 */
	set_my_cpu_offset(0);
	printf("Booting Linux on physical CPU 0x%010llx [0x%08x]\n",
		(u64)mpidr, read_cpuid_id());
}

void __init early_arch_platform_init(void)
{
    memblock_init(&memblock_kernel);

    early_fixmap_init();

    setup_machine_fdt(__fdt_pointer);

    setup_fixmap_console();
}

void __init setup_arch(char **cmdline_p)
{
	/*
	 * Unmask asynchronous aborts and fiq after bringing up possible
	 * earlycon. (Report possible System Errors once we can report this
	 * occurred).
	 */
	local_daif_restore(DAIF_PROCCTX_NOIRQ);

	/*
	 * TTBR0 is only used for the identity mapping at this stage. Make it
	 * point to zero page to avoid speculatively fetching new entries.
	 */
	cpu_uninstall_idmap();

	arm64_memblock_init();
	paging_init();

	unflatten_device_tree();

	bootmem_init();

	psci_dt_init();

	cpu_read_bootcpu_ops();
	smp_init_cpus();

	asids_init();

	if (boot_args[1] || boot_args[2] || boot_args[3]) {
		printf("WARNING: x1-x3 nonzero in violation of boot protocol:\n"
			"\tx1: %016llx\n\tx2: %016llx\n\tx3: %016llx\n"
			"This indicates a broken bootloader or old kernel\n",
			boot_args[1], boot_args[2], boot_args[3]);
	}
}
