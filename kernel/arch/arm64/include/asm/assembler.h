/*
 * Based on arch/arm/include/asm/assembler.h, arch/arm/mm/proc-macros.S
 *
 * Copyright (C) 1996-2000 Russell King
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
#ifndef __ASSEMBLY__
#error "Only include this from assembly code"
#endif

#ifndef __ASM_ASSEMBLER_H_
#define __ASM_ASSEMBLER_H_

#include <base/linkage.h>

#include <asm/base/assembler.h>

#include <asm/kernel-pgtable.h>
#include <asm/deubg-monitors.h>
#include <asm/thread_info.h>
#include <asm/ptrace.h>

#include <generated/asm-offsets.h>

	.macro save_and_disable_daif, flags
	mrs	\flags, daif
	msr	daifset, #0xf
	.endm

	.macro disable_daif
	msr	daifset, #0xf
	.endm

	.macro enable_daif
	msr	daifclr, #0xf
	.endm

	.macro	restore_daif, flags:req
	msr	daif, \flags
	.endm

	/* Only on aarch64 pstate, PSR_D_BIT is different for aarch32 */
	.macro	inherit_daif, pstate:req, tmp:req
	and	\tmp, \pstate, #(PSR_D_BIT | PSR_A_BIT | PSR_I_BIT | PSR_F_BIT)
	msr	daif, \tmp
	.endm

	/* IRQ is the lowest priority flag, unconditionally unmask the rest. */
	.macro enable_da_f
	msr	daifclr, #(8 | 4 | 1)
	.endm

/*
 * Enable and disable interrupts.
 */
	.macro	disable_irq
	msr	daifset, #2
	.endm

	.macro	enable_irq
	msr	daifclr, #2
	.endm

	.macro	save_and_disable_irq, flags
	mrs	\flags, daif
	msr	daifset, #2
	.endm

	.macro	restore_irq, flags
	msr	daif, \flags
	.endm

	.macro	enable_dbg
	msr	daifclr, #8
	.endm

	.macro	disable_step_tsk, flgs, tmp
	tbz	\flgs, #TIF_SINGLESTEP, 9990f
	mrs	\tmp, mdscr_el1
	bic	\tmp, \tmp, #DBG_MDSCR_SS
	msr	mdscr_el1, \tmp
	isb	// Synchronise with enable_dbg
9990:
	.endm

	/* call with daif masked */
	.macro	enable_step_tsk, flgs, tmp
	tbz	\flgs, #TIF_SINGLESTEP, 9990f
	mrs	\tmp, mdscr_el1
	orr	\tmp, \tmp, #DBG_MDSCR_SS
	msr	mdscr_el1, \tmp
9990:
	.endm

/*
 * SMP data memory barrier
 */
	.macro	smp_dmb, opt
	dmb	\opt
	.endm

/*
 * RAS Error Synchronization barrier
 */
	.macro  esb
	hint    #16
	.endm

/*
 * Value prediction barrier
 */
	.macro	csdb
	hint	#20
	.endm

/*
 * Speculation barrier
 */
	.macro	sb
	dsb	nsh
	isb
	.endm

	/*
	 * Emit a 64-bit absolute little endian symbol reference in a way that
	 * ensures that it will be resolved at build time, even when building a
	 * PIE binary. This requires cooperation from the linker script, which
	 * must emit the lo32/hi32 halves individually.
	 */
	.macro	le64sym, sym
	.long	\sym\()_lo32
	.long	\sym\()_hi32
	.endm

	/*
	 * mov_q - move an immediate constant into a 64-bit register using
	 *         between 2 and 4 movz/movk instructions (depending on the
	 *         magnitude and sign of the operand)
	 */
	.macro	mov_q, reg, val
	.if (((\val) >> 31) == 0 || ((\val) >> 31) == 0x1ffffffff)
	movz	\reg, :abs_g1_s:\val
	.else
	.if (((\val) >> 47) == 0 || ((\val) >> 47) == 0x1ffff)
	movz	\reg, :abs_g2_s:\val
	.else
	movz	\reg, :abs_g3:\val
	movk	\reg, :abs_g2_nc:\val
	.endif
	movk	\reg, :abs_g1_nc:\val
	.endif
	movk	\reg, :abs_g0_nc:\val
	.endm

	/*
	 * @dst: destination register (64 bit wide)
	 * @sym: name of the symbol
	 */
	.macro	adr_l, dst, sym
	adrp	\dst, \sym
	add	\dst, \dst, :lo12:\sym
	.endm

	/*
	 * @dst: destination register (32 or 64 bit wide)
	 * @sym: name of the symbol
	 * @tmp: optional 64-bit scratch register to be used if <dst> is a
	 *       32-bit wide register, in which case it cannot be used to hold
	 *       the address
	 */
	.macro	ldr_l, dst, sym, tmp=
	.ifb	\tmp
	adrp	\dst, \sym
	ldr	\dst, [\dst, :lo12:\sym]
	.else
	adrp	\tmp, \sym
	ldr	\dst, [\tmp, :lo12:\sym]
	.endif
	.endm

	/*
	 * @src: source register (32 or 64 bit wide)
	 * @sym: name of the symbol
	 * @tmp: mandatory 64-bit scratch register to calculate the address
	 *       while <src> needs to be preserved.
	 */
	.macro	str_l, src, sym, tmp
	adrp	\tmp, \sym
	str	\src, [\tmp, :lo12:\sym]
	.endm

/*
 * read_ctr - read CTR_EL0. If the system has mismatched register fields,
 * provide the system wide safe value from arm64_ftr_reg_ctrel0.sys_val
 */
	.macro	read_ctr, reg
	mrs	\reg, ctr_el0			// read CTR
	nop
	.endm

/*
 * mmid - get context id from mm pointer (mm->context.id)
 */
	.macro	mmid, rd, rn
	ldr	\rd, [\rn, #MM_CONTEXT_ID]
	.endm

/*
 * dcache_line_size - get the safe D-cache line size across all CPUs
 */
	.macro	dcache_line_size, reg, tmp
	read_ctr	\tmp
	ubfm		\tmp, \tmp, #16, #19	// cache line size encoding
	mov		\reg, #4		// bytes per word
	lsl		\reg, \reg, \tmp	// actual cache line size
	.endm

/*
 * Return the current thread_info.
 */
	.macro	get_thread_info, rd
	mrs	\rd, sp_el0
	.endm

/*
 * Annotate a function as position independent, i.e., safe to be called before
 * the kernel virtual mapping is activated.
 */
#define ENDPIPROC(x)			\
	.globl	__pi_##x;		\
	.type 	__pi_##x, %function;	\
	.set	__pi_##x, x;		\
	.size	__pi_##x, . - x;	\
	ENDPROC(x)

/*
 * Arrange a physical address in a TTBR register, taking care of 52-bit
 * addresses.
 *
 * 	phys:	physical address, preserved
 * 	ttbr:	returns the TTBR value
 */
	.macro	phys_to_ttbr, ttbr, phys
	mov	\ttbr, \phys
	.endm

	.macro	phys_to_pte, pte, phys
	mov	\pte, \phys
	.endm

	.macro	pte_to_phys, phys, pte
	and	\phys, \pte, #PTE_ADDR_MASK
	.endm

/*
 * reset_pmuserenr_el0 - reset PMUSERENR_EL0 if PMUv3 present
 */
	.macro	reset_pmuserenr_el0, tmpreg
	mrs	\tmpreg, id_aa64dfr0_el1	// Check ID_AA64DFR0_EL1 PMUVer
	sbfx	\tmpreg, \tmpreg, #8, #4
	cmp	\tmpreg, #1			// Skip if no PMU present
	b.lt	9000f
	msr	pmuserenr_el0, xzr		// Disable PMU access from EL0
9000:
	.endm

/*
 * tcr_set_t0sz - update TCR.T0SZ so that we can load the ID map
 */
	.macro	tcr_set_t0sz, valreg, t0sz
	bfi	\valreg, \t0sz, #TCR_T0SZ_OFFSET, #TCR_TxSZ_WIDTH
	.endm

/*
 * tcr_compute_pa_size - set TCR.(I)PS to the highest supported
 * ID_AA64MMFR0_EL1.PARange value
 *
 *	tcr:		register with the TCR_ELx value to be updated
 *	pos:		IPS or PS bitfield position
 *	tmp{0,1}:	temporary registers
 */
	.macro	tcr_compute_pa_size, tcr, pos, tmp0, tmp1
	mrs	\tmp0, ID_AA64MMFR0_EL1
	// Narrow PARange to fit the PS field in TCR_ELx
	ubfx	\tmp0, \tmp0, #ID_AA64MMFR0_PARANGE_SHIFT, #3
	mov	\tmp1, #ID_AA64MMFR0_PARANGE_MAX
	cmp	\tmp0, \tmp1
	csel	\tmp0, \tmp1, \tmp0, hi
	bfi	\tcr, \tmp0, \pos, #3
	.endm

/*
 * Offset ttbr1 to allow for 48-bit kernel VAs set with 52-bit PTRS_PER_PGD.
 * orr is used as it can cover the immediate value (and is idempotent).
 * In future this may be nop'ed out when dealing with 52-bit kernel VAs.
 * 	ttbr: Value of ttbr to set, modified.
 */
	.macro	offset_ttbr1, ttbr
	.endm


/*
 * Macro to perform a data cache maintenance for the interval
 * [kaddr, kaddr + size)
 *
 * 	op:		operation passed to dc instruction
 * 	domain:		domain used in dsb instruciton
 * 	kaddr:		starting virtual address of the region
 * 	size:		size of the region
 * 	Corrupts:	kaddr, size, tmp1, tmp2
 */
	.macro __dcache_op_workaround_clean_cache, op, kaddr
	dc	\op, \kaddr
	.endm

	.macro dcache_by_line_op op, domain, kaddr, size, tmp1, tmp2
	dcache_line_size \tmp1, \tmp2
	add	\size, \kaddr, \size
	sub	\tmp2, \tmp1, #1
	bic	\kaddr, \kaddr, \tmp2
9998:
	.ifc	\op, cvau
	__dcache_op_workaround_clean_cache \op, \kaddr
	.else
	.ifc	\op, cvac
	__dcache_op_workaround_clean_cache \op, \kaddr
	.else
	.ifc	\op, cvap
	sys	3, c7, c12, 1, \kaddr	// dc cvap
	.else
	dc	\op, \kaddr
	.endif
	.endif
	.endif
	add	\kaddr, \kaddr, \tmp1
	cmp	\kaddr, \size
	b.lo	9998b
	dsb	\domain
	.endm

/*
 * Remove the address tag from a virtual address, if present.
 */
	.macro	clear_address_tag, dst, addr
	tst	\addr, #(1 << 55)
	bic	\dst, \addr, #(0xff << 56)
	csel	\dst, \dst, \addr, eq
	.endm

#endif /* !__ASM_ASSEMBLER_H_ */
