/*
 * arch/arm64/include/asm/arch_timer.h
 *
 * Copyright (C) 2012 ARM Ltd.
 * Author: Marc Zyngier <marc.zyngier@arm.com>
 *
 * This program is free software: you can redistribute it and/or modify
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
#ifndef __ASM_ARCH_TIMER_H_
#define __ASM_ARCH_TIMER_H_

#include <clocksource/arm_arch_timer.h>

#include <asm/base/barrier.h>

#include <asm/sysreg.h>

#define arch_timer_reg_read_stable(reg)					\
({									\
	u64 _val;							\
											\
	_val = read_sysreg(reg);				\
	_val;								\
})

/*
 * These register accessors are marked inline so the compiler can
 * nicely work out which register we want, and chuck away the rest of
 * the code.
 */
static __always_inline
void arch_timer_reg_write_cp15(int access, enum arch_timer_reg reg, u32 val)
{
	if (access == ARCH_TIMER_PHYS_ACCESS) {
		switch (reg) {
		case ARCH_TIMER_REG_CTRL:
			write_sysreg(val, cntp_ctl_el0);
			break;
		case ARCH_TIMER_REG_TVAL:
			write_sysreg(val, cntp_tval_el0);
			break;
		}
	} else if (access == ARCH_TIMER_VIRT_ACCESS) {
		switch (reg) {
		case ARCH_TIMER_REG_CTRL:
			write_sysreg(val, cntv_ctl_el0);
			break;
		case ARCH_TIMER_REG_TVAL:
			write_sysreg(val, cntv_tval_el0);
			break;
		}
	}

	isb();
}

static __always_inline
u32 arch_timer_reg_read_cp15(int access, enum arch_timer_reg reg)
{
	if (access == ARCH_TIMER_PHYS_ACCESS) {
		switch (reg) {
		case ARCH_TIMER_REG_CTRL:
			return read_sysreg(cntp_ctl_el0);
		case ARCH_TIMER_REG_TVAL:
			return arch_timer_reg_read_stable(cntp_tval_el0);
		}
	} else if (access == ARCH_TIMER_VIRT_ACCESS) {
		switch (reg) {
		case ARCH_TIMER_REG_CTRL:
			return read_sysreg(cntv_ctl_el0);
		case ARCH_TIMER_REG_TVAL:
			return arch_timer_reg_read_stable(cntv_tval_el0);
		}
	}

	BUG();
}

static inline u32 arch_timer_get_cntfrq(void)
{
	return read_sysreg(cntfrq_el0);
}

static inline u32 arch_timer_get_cntkctl(void)
{
	return read_sysreg(cntkctl_el1);
}

static inline void arch_timer_set_cntkctl(u32 cntkctl)
{
	write_sysreg(cntkctl, cntkctl_el1);
	isb();
}

static inline u64 arch_counter_get_cntpct(void)
{
	isb();
	return arch_timer_reg_read_stable(cntpct_el0);
}

static inline u64 arch_counter_get_cntvct(void)
{
	isb();
	return arch_timer_reg_read_stable(cntvct_el0);
}

static inline int arch_timer_arch_init(void)
{
	return 0;
}

#endif /* !__ASM_ARCH_TIMER_H_ */
