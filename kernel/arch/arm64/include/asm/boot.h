/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_BOOT_H_
#define __ASM_BOOT_H_

#include <base/sizes.h>

#define BOOT_CPU_MODE_EL1	(0xe11)
#define BOOT_CPU_MODE_EL2	(0xe12)

/*
 * arm64 requires the kernel image to placed
 * TEXT_OFFSET bytes beyond a 2 MB aligned base
 */
#define MIN_KIMG_ALIGN		SZ_2M

#ifndef __ASSEMBLY__

#include <base/types.h>
#include <base/init.h>

/*
 * __boot_cpu_mode records what mode CPUs were booted in.
 * A correctly-implemented bootloader must start all CPUs in the same mode:
 * In this case, both 32bit halves of __boot_cpu_mode will contain the
 * same value (either 0 if booted in EL1, BOOT_CPU_MODE_EL2 if booted in EL2).
 *
 * Should the bootloader fail to do this, the two values will be different.
 * This allows the kernel to flag an error when the secondaries have come up.
 */
extern u32 __boot_cpu_mode[2];

static inline bool is_boot_el2(void)
{
	return (__boot_cpu_mode[0] == BOOT_CPU_MODE_EL2 &&
		__boot_cpu_mode[1] == BOOT_CPU_MODE_EL2);
}

extern u64 __cacheline_aligned boot_args[4];
extern phys_addr_t __fdt_pointer __initdata;

#endif /* !__ASSEMBLY__ */

#endif /* !__ASM_BOOT_H_ */
