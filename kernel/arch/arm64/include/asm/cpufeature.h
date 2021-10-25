/*
 * Copyright (C) 2014 Linaro Ltd. <ard.biesheuvel@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_CPUFEATURE_H_
#define __ASM_CPUFEATURE_H_

#include <base/types.h>
#include <base/bitmap.h>

#include <asm/cpucaps.h>

/*
 * CPU feature register tracking
 *
 * The safe value of a CPUID feature field is dependent on the implications
 * of the values assigned to it by the architecture. Based on the relationship
 * between the values, the features are classified into 3 types - LOWER_SAFE,
 * HIGHER_SAFE and EXACT.
 *
 * The lowest value of all the CPUs is chosen for LOWER_SAFE and highest
 * for HIGHER_SAFE. It is expected that all CPUs have the same value for
 * a field when EXACT is specified, failing which, the safe value specified
 * in the table is chosen.
 */

enum ftr_type {
	FTR_EXACT,	/* Use a predefined safe value */
	FTR_LOWER_SAFE,	/* Smaller value is safe */
	FTR_HIGHER_SAFE,/* Bigger value is safe */
};

struct arm64_ftr_bits {
	bool		sign;	/* Value is signed ? */
	bool		visible;
	bool		strict;	/* CPU Sanity check: strict matching required ? */
	enum ftr_type	type;
	u8		shift;
	u8		width;
	s64		safe_val; /* safe value for FTR_EXACT features */
};

/*
 * @arm64_ftr_reg - Feature register
 * @strict_mask		Bits which should match across all CPUs for sanity.
 * @sys_val		Safe value across the CPUs (system view)
 */
struct arm64_ftr_reg {
	const char			*name;
	u64				strict_mask;
	u64				user_mask;
	u64				sys_val;
	u64				user_val;
	const struct arm64_ftr_bits	*ftr_bits;
};

#define FTR_STRICT	true	/* SANITY check strict matching required */
#define FTR_NONSTRICT	false	/* SANITY check ignored */

#define FTR_SIGNED	true	/* Value should be treated as signed */
#define FTR_UNSIGNED	false	/* Value should be treated as unsigned */

#define FTR_VISIBLE	true	/* Feature visible to the user space */
#define FTR_HIDDEN	false	/* Feature is hidden from the user */

/*
 * Decide how the capability is detected.
 * On any local CPU vs System wide vs the primary boot CPU
 */
#define ARM64_CPUCAP_SCOPE_LOCAL_CPU		((u16)BIT(0))
#define ARM64_CPUCAP_SCOPE_SYSTEM		((u16)BIT(1))
/*
 * The capabilitiy is detected on the Boot CPU and is used by kernel
 * during early boot. i.e, the capability should be "detected" and
 * "enabled" as early as possibly on all booting CPUs.
 */
#define ARM64_CPUCAP_SCOPE_BOOT_CPU		((u16)BIT(2))
#define ARM64_CPUCAP_SCOPE_MASK			\
	(ARM64_CPUCAP_SCOPE_SYSTEM	|	\
	 ARM64_CPUCAP_SCOPE_LOCAL_CPU	|	\
	 ARM64_CPUCAP_SCOPE_BOOT_CPU)

#define SCOPE_SYSTEM				ARM64_CPUCAP_SCOPE_SYSTEM
#define SCOPE_LOCAL_CPU				ARM64_CPUCAP_SCOPE_LOCAL_CPU
#define SCOPE_BOOT_CPU				ARM64_CPUCAP_SCOPE_BOOT_CPU
#define SCOPE_ALL				ARM64_CPUCAP_SCOPE_MASK

/*
 * Is it permitted for a late CPU to have this capability when system
 * hasn't already enabled it ?
 */
#define ARM64_CPUCAP_PERMITTED_FOR_LATE_CPU	((u16)BIT(4))
/* Is it safe for a late CPU to miss this capability when system has it */
#define ARM64_CPUCAP_OPTIONAL_FOR_LATE_CPU	((u16)BIT(5))

/*
 * CPU errata workarounds that need to be enabled at boot time if one or
 * more CPUs in the system requires it. When one of these capabilities
 * has been enabled, it is safe to allow any CPU to boot that doesn't
 * require the workaround. However, it is not safe if a "late" CPU
 * requires a workaround and the system hasn't enabled it already.
 */
#define ARM64_CPUCAP_LOCAL_CPU_ERRATUM		\
	(ARM64_CPUCAP_SCOPE_LOCAL_CPU | ARM64_CPUCAP_OPTIONAL_FOR_LATE_CPU)

#define ARM64_CPUCAP_SYSTEM_FEATURE	\
	(ARM64_CPUCAP_SCOPE_SYSTEM | ARM64_CPUCAP_PERMITTED_FOR_LATE_CPU)
/*
 * CPU feature detected at boot time based on feature of one or more CPUs.
 * All possible conflicts for a late CPU are ignored.
 */
#define ARM64_CPUCAP_WEAK_LOCAL_CPU_FEATURE		\
	(ARM64_CPUCAP_SCOPE_LOCAL_CPU		|	\
	 ARM64_CPUCAP_OPTIONAL_FOR_LATE_CPU	|	\
	 ARM64_CPUCAP_PERMITTED_FOR_LATE_CPU)

/*
 * CPU feature detected at boot time, on one or more CPUs. A late CPU
 * is not allowed to have the capability when the system doesn't have it.
 * It is Ok for a late CPU to miss the feature.
 */
#define ARM64_CPUCAP_BOOT_RESTRICTED_CPU_LOCAL_FEATURE	\
	(ARM64_CPUCAP_SCOPE_LOCAL_CPU		|	\
	 ARM64_CPUCAP_OPTIONAL_FOR_LATE_CPU)

/*
 * CPU feature used early in the boot based on the boot CPU. All secondary
 * CPUs must match the state of the capability as detected by the boot CPU.
 */
#define ARM64_CPUCAP_STRICT_BOOT_CPU_FEATURE ARM64_CPUCAP_SCOPE_BOOT_CPU

struct arm64_cpu_capabilities {
	const char *desc;
	u16 capability;
	u16 type;
	bool (*matches)(const struct arm64_cpu_capabilities *caps, int scope);
	/*
	 * Take the appropriate actions to enable this capability for this CPU.
	 * For each successfully booted CPU, this method is called for each
	 * globally detected capability.
	 */
	void (*cpu_enable)(const struct arm64_cpu_capabilities *cap);
	union {
		struct {	/* Feature register checking */
			u32 sys_reg;
			u8 field_pos;
			u8 min_field_value;
			u8 hwcap_type;
			bool sign;
			unsigned long hwcap;
		};
	};
};

static inline int __attribute_const__
cpuid_feature_extract_signed_field_width(u64 features, int field, int width)
{
	return (s64)(features << (64 - width - field)) >> (64 - width);
}

static inline unsigned int __attribute_const__
cpuid_feature_extract_unsigned_field_width(u64 features, int field, int width)
{
	return (u64)(features << (64 - width - field)) >> (64 - width);
}

static inline int __attribute_const__
cpuid_feature_extract_field_width(u64 features, int field, int width, bool sign)
{
	return (sign) ?
		cpuid_feature_extract_signed_field_width(features, field, width) :
		cpuid_feature_extract_unsigned_field_width(features, field, width);
}

static inline int __attribute_const__
cpuid_feature_extract_signed_field(u64 features, int field)
{
	return cpuid_feature_extract_signed_field_width(features, field, 4);
}

static inline int __attribute_const__
cpuid_feature_extract_field(u64 features, int field, bool sign)
{
	return cpuid_feature_extract_field_width(features, field, 4, sign);
}

static inline u64 arm64_ftr_mask(const struct arm64_ftr_bits *ftrp)
{
	return (u64)GENMASK(ftrp->shift + ftrp->width - 1, ftrp->shift);
}

static inline s64 arm64_ftr_value(const struct arm64_ftr_bits *ftrp, u64 val)
{
	return (s64)cpuid_feature_extract_field_width(val, ftrp->shift, ftrp->width, ftrp->sign);
}

static inline u64 arm64_ftr_reg_user_value(const struct arm64_ftr_reg *reg)
{
	return (reg->user_val | (reg->sys_val & reg->user_mask));
}

static inline int cpucap_default_scope(const struct arm64_cpu_capabilities *cap)
{
	return cap->type & ARM64_CPUCAP_SCOPE_MASK;
}

static inline bool
cpucap_late_cpu_optional(const struct arm64_cpu_capabilities *cap)
{
	return !!(cap->type & ARM64_CPUCAP_OPTIONAL_FOR_LATE_CPU);
}

static inline bool
cpucap_late_cpu_permitted(const struct arm64_cpu_capabilities *cap)
{
	return !!(cap->type & ARM64_CPUCAP_PERMITTED_FOR_LATE_CPU);
}

extern DECLARE_BITMAP(cpu_hwcaps, ARM64_NCAPS);

static inline void cpus_set_cap(unsigned int num)
{
	if (num >= ARM64_NCAPS) {
		printf("Attempt to set an illegal CPU capability (%d >= %d)\n",
			num, ARM64_NCAPS);
	} else {
		__set_bit(num, cpu_hwcaps);
	}
}

static inline bool cpus_have_cap(unsigned int num)
{
	if (num >= ARM64_NCAPS)
		return false;
	return test_bit(num, cpu_hwcaps);
}

#define for_each_available_cap(cap)		\
	for_each_set_bit(cap, cpu_hwcaps, ARM64_NCAPS)

void __init setup_cpu_features(void);
void check_local_cpu_capabilities(void);

int do_emulate_mrs(struct pt_regs *regs, u32 sys_reg, u32 rt);
#endif /* !__ASM_CPUFEATURE_H_ */
