/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_BOOT_H_
#define __ASM_BOOT_H_

#include <sel4m/sizes.h>

#define BOOT_CPU_MODE_EL1	(0xe11)
#define BOOT_CPU_MODE_EL2	(0xe12)

/*
 * arm64 requires the kernel image to placed
 * TEXT_OFFSET bytes beyond a 2 MB aligned base
 */
#define MIN_KIMG_ALIGN		SZ_2M

#ifndef __ASSEMBLY__

extern u64 __cacheline_aligned boot_args[4];

#endif /* !__ASSEMBLY__ */

#endif /* !__ASM_BOOT_H_ */
