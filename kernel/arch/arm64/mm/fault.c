/*
 * Based on arch/arm/mm/fault.c
 *
 * Copyright (C) 1995  Linus Torvalds
 * Copyright (C) 1995-2004 Russell King
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
#include <base/common.h>
#include <base/linkage.h>

#include <minix_rt/smp.h>
#include <minix_rt/stat.h>
#include <minix_rt/object/notifier.h>

#include <asm/traps.h>
#include <asm/pgtable.h>
#include <asm/esr.h>
#include <asm/daifflags.h>
#include <asm/ptrace.h>
#include <asm/deubg-monitors.h>
#include <asm/exception.h>
#include <asm/tlbflush.h>
#include <asm/current.h>
#include <asm/system_misc.h>
#include <asm/extable.h>

struct fault_info {
	int	(*fn)(unsigned long addr, unsigned int esr,
		      struct pt_regs *regs);
	int	sig;
	int	code;
	const char *name;
};

static const struct fault_info fault_info[];
static struct fault_info debug_fault_info[];

static inline const struct fault_info *esr_to_fault_info(unsigned int esr)
{
	return fault_info + (esr & ESR_ELx_FSC);
}

static inline const struct fault_info *esr_to_debug_fault_info(unsigned int esr)
{
	return debug_fault_info + DBG_ESR_EVT(esr);
}

static void data_abort_decode(unsigned int esr)
{
	printf("Data abort info:\n");

	if (esr & ESR_ELx_ISV) {
		printf("  Access size = %u byte(s)\n",
			 1U << ((esr & ESR_ELx_SAS) >> ESR_ELx_SAS_SHIFT));
		printf("  SSE = %llu, SRT = %llu\n",
			 (esr & ESR_ELx_SSE) >> ESR_ELx_SSE_SHIFT,
			 (esr & ESR_ELx_SRT_MASK) >> ESR_ELx_SRT_SHIFT);
		printf("  SF = %llu, AR = %llu\n",
			 (esr & ESR_ELx_SF) >> ESR_ELx_SF_SHIFT,
			 (esr & ESR_ELx_AR) >> ESR_ELx_AR_SHIFT);
	} else {
		printf("  ISV = 0, ISS = 0x%08llx\n", esr & ESR_ELx_ISS_MASK);
	}

	printf("  CM = %llu, WnR = %llu\n",
		 (esr & ESR_ELx_CM) >> ESR_ELx_CM_SHIFT,
		 (esr & ESR_ELx_WNR) >> ESR_ELx_WNR_SHIFT);
}

static void mem_abort_decode(unsigned int esr)
{
	printf("Mem abort info:\n");

	printf("  ESR = 0x%08x\n", esr);
	printf("  Exception class = %s, IL = %u bits\n",
		 esr_get_class_string(esr),
		 (esr & ESR_ELx_IL) ? 32 : 16);
	printf("  SET = %llu, FnV = %llu\n",
		 (esr & ESR_ELx_SET_MASK) >> ESR_ELx_SET_SHIFT,
		 (esr & ESR_ELx_FnV) >> ESR_ELx_FnV_SHIFT);
	printf("  EA = %llu, S1PTW = %llu\n",
		 (esr & ESR_ELx_EA) >> ESR_ELx_EA_SHIFT,
		 (esr & ESR_ELx_S1PTW) >> ESR_ELx_S1PTW_SHIFT);

	if (esr_is_data_abort(esr))
		data_abort_decode(esr);
}

static inline bool is_ttbr0_addr(unsigned long addr)
{
	/* entry assembly clears tags for TTBR0 addrs */
	return addr < TASK_SIZE;
}

static inline bool is_ttbr1_addr(unsigned long addr)
{
	/* TTBR1 addresses may have a tag if KASAN_SW_TAGS is in use */
	return addr >= VA_START;
}

/*
 * Dump out the page tables associated with 'addr' in the currently active mm.
 */
void show_pte(unsigned long addr)
{
	struct mm_struct *mm;
	pgd_t *pgdp;
	pgd_t pgd;

	if (is_ttbr0_addr(addr)) {
		/* TTBR0 */
		mm = current->active_mm;
		if (mm == &init_mm) {
			printf("[%016lx] user address but mm is kernel\n",
				 addr);
			return;
		}
	} else if (is_ttbr1_addr(addr)) {
		/* TTBR1 */
		mm = &init_mm;
	} else {
		printf("[%016lx] address between user and kernel address ranges\n",
			 addr);
		return;
	}

	printf("%s pgtable: %luk pages, %u-bit VAs, pgdp = %p\n",
		 mm == &init_mm ? "kernel" : "user", PAGE_SIZE / SZ_1K,
		 mm == &init_mm ? VA_BITS : (int) vabits_user, mm->pgd);
	pgdp = pgd_offset(mm->pgd, addr);
	pgd = READ_ONCE(*pgdp);
	printf("[%016lx] pgd=%016llx\n", addr, pgd_val(pgd));

	do {
		pud_t *pudp, pud;
		pmd_t *pmdp, pmd;
		pte_t *ptep, pte;

		if (pgd_none(pgd) || pgd_bad(pgd))
			break;

		pudp = pud_offset(pgdp, addr);
		pud = READ_ONCE(*pudp);
		printf(KERN_CONT ", pud=%016llx", pud_val(pud));
		if (pud_none(pud) || pud_bad(pud))
			break;

		pmdp = pmd_offset(pudp, addr);
		pmd = READ_ONCE(*pmdp);
		printf(KERN_CONT ", pmd=%016llx", pmd_val(pmd));
		if (pmd_none(pmd) || pmd_bad(pmd))
			break;

		ptep = pte_offset_kernel(pmdp, addr);
		pte = READ_ONCE(*ptep);
		printf(KERN_CONT ", pte=%016llx", pte_val(pte));
		printf(KERN_CONT "\n");
	} while(0);
}

/*
 * This function sets the access flags (dirty, accessed), as well as write
 * permission, and only to a more permissive setting.
 *
 * It needs to cope with hardware update of the accessed/dirty state by other
 * agents in the system and can safely skip the __sync_icache_dcache() call as,
 * like set_pte_at(), the PTE is never changed from no-exec to exec here.
 *
 * Returns whether or not the PTE actually changed.
 */
int ptep_set_access_flags(struct vm_area_struct *vma,
			  unsigned long address, pte_t *ptep,
			  pte_t entry, int dirty)
{
	pteval_t old_pteval, pteval;
	pte_t pte = READ_ONCE(*ptep);

	if (pte_same(pte, entry))
		return 0;

	/* only preserve the access flags and write permission */
	pte_val(entry) &= PTE_RDONLY | PTE_AF | PTE_WRITE | PTE_DIRTY;

	/*
	 * Setting the flags must be done atomically to avoid racing with the
	 * hardware update of the access/dirty state. The PTE_RDONLY bit must
	 * be set to the most permissive (lowest value) of *ptep and entry
	 * (calculated as: a & b == ~(~a | ~b)).
	 */
	pte_val(entry) ^= PTE_RDONLY;
	pteval = pte_val(pte);
	do {
		old_pteval = pteval;
		pteval ^= PTE_RDONLY;
		pteval |= pte_val(entry);
		pteval ^= PTE_RDONLY;
		pteval = cmpxchg_relaxed(&pte_val(*ptep), old_pteval, pteval);
	} while (pteval != old_pteval);

	flush_tlb_page(vma->vm_mm, address);
	return 1;
}

static bool is_el1_instruction_abort(unsigned int esr)
{
	return ESR_ELx_EC(esr) == ESR_ELx_EC_IABT_CUR;
}

static inline bool is_el1_permission_fault(unsigned long addr, unsigned int esr,
					   struct pt_regs *regs)
{
	unsigned int ec       = ESR_ELx_EC(esr);
	unsigned int fsc_type = esr & ESR_ELx_FSC_TYPE;

	if (ec != ESR_ELx_EC_DABT_CUR && ec != ESR_ELx_EC_IABT_CUR)
		return false;

	if (fsc_type == ESR_ELx_FSC_PERM)
		return true;

	return false;
}

static void die_kernel_fault(const char *msg, unsigned long addr,
			     unsigned int esr, struct pt_regs *regs)
{
	printf("Unable to handle kernel %s at virtual address %016lx\n", msg,
		 addr);

	mem_abort_decode(esr);

	show_pte(addr);
	die("Oops", regs, esr);

	do_exit(SIGKILL);
}

static void __do_kernel_fault(unsigned long addr, unsigned int esr,
			      struct pt_regs *regs)
{
	const char *msg;

	/*
	 * Are we prepared to handle this kernel fault?
	 * We are almost certainly not prepared to handle instruction faults.
	 */
	if (!is_el1_instruction_abort(esr) && fixup_exception(regs))
		return;

	if (is_el1_permission_fault(addr, esr, regs)) {
		if (esr & ESR_ELx_WNR)
			msg = "write to read-only memory";
		else
			msg = "read from unreadable memory";
	} else if (addr < PAGE_SIZE) {
		msg = "NULL pointer dereference";
	} else {
		msg = "paging request";
	}

	die_kernel_fault(msg, addr, esr, regs);
}

static void set_thread_esr(unsigned long address, unsigned int esr)
{
	current->thread.fault_address = address;

	/*
	 * If the faulting address is in the kernel, we must sanitize the ESR.
	 * From userspace's point of view, kernel-only mappings don't exist
	 * at all, so we report them as level 0 translation faults.
	 * (This is not quite the way that "no mapping there at all" behaves:
	 * an alignment fault not caused by the memory type would take
	 * precedence over translation fault for a real access to empty
	 * space. Unfortunately we can't easily distinguish "alignment fault
	 * not caused by memory type" from "alignment fault caused by memory
	 * type", so we ignore this wrinkle and just return the translation
	 * fault.)
	 */
	if (!is_ttbr0_addr(current->thread.fault_address)) {
		switch (ESR_ELx_EC(esr)) {
		case ESR_ELx_EC_DABT_LOW:
			/*
			 * These bits provide only information about the
			 * faulting instruction, which userspace knows already.
			 * We explicitly clear bits which are architecturally
			 * RES0 in case they are given meanings in future.
			 * We always report the ESR as if the fault was taken
			 * to EL1 and so ISV and the bits in ISS[23:14] are
			 * clear. (In fact it always will be a fault to EL1.)
			 */
			esr &= ESR_ELx_EC_MASK | ESR_ELx_IL |
				ESR_ELx_CM | ESR_ELx_WNR;
			esr |= ESR_ELx_FSC_FAULT;
			break;
		case ESR_ELx_EC_IABT_LOW:
			/*
			 * Claim a level 0 translation fault.
			 * All other bits are architecturally RES0 for faults
			 * reported with that DFSC value, so we clear them.
			 */
			esr &= ESR_ELx_EC_MASK | ESR_ELx_IL;
			esr |= ESR_ELx_FSC_FAULT;
			break;
		default:
			/*
			 * This should never happen (entry.S only brings us
			 * into this code for insn and data aborts from a lower
			 * exception level). Fail safe by not providing an ESR
			 * context record at all.
			 */
			WARN(1, "ESR 0x%x is not DABT or IABT from EL0\n", esr);
			esr = 0;
			break;
		}
	}

	current->thread.fault_code = esr;
}

static void do_bad_area(unsigned long addr, unsigned int esr, struct pt_regs *regs)
{
	/*
	 * If we are in kernel mode at this point, we have no context to
	 * handle this fault with.
	 */
	if (user_mode(regs)) {
		const struct fault_info *inf = esr_to_fault_info(esr);

		set_thread_esr(addr, esr);
		arm64_force_sig_fault(inf->sig, inf->code, (void __user *)addr,
				      inf->name);
	} else {
		__do_kernel_fault(addr, esr, regs);
	}
}

#define VM_FAULT_BADMAP		0x010000
#define VM_FAULT_BADACCESS	0x020000

static int __do_page_fault(struct mm_struct *mm, unsigned long addr,
			   unsigned int mm_flags, unsigned long vm_flags,
			   struct task_struct *tsk)
{
	/* TODO  Do page fault */
	return -1;
}

static bool is_el0_instruction_abort(unsigned int esr)
{
	return ESR_ELx_EC(esr) == ESR_ELx_EC_IABT_LOW;
}

static int do_page_fault(unsigned long addr, unsigned int esr,
				   struct pt_regs *regs)
{
	int fault;

	is_el0_instruction_abort(esr);
	/* TODO */
	fault = __do_page_fault(NULL, addr, 0, 0, NULL);
	return fault;
}

static int do_translation_fault(unsigned long addr,
					  unsigned int esr,
					  struct pt_regs *regs)
{
	if (is_ttbr0_addr(addr))
		return do_page_fault(addr, esr, regs);

	do_bad_area(addr, esr, regs);
	return 0;
}

static int do_alignment_fault(unsigned long addr, unsigned int esr,
			      struct pt_regs *regs)
{
	do_bad_area(addr, esr, regs);
	return 0;
}

static int do_bad(unsigned long addr, unsigned int esr, struct pt_regs *regs)
{
	return 1; /* "fault" */
}

static int do_sea(unsigned long addr, unsigned int esr, struct pt_regs *regs)
{
	const struct fault_info *inf;
	void __user *siaddr;

	inf = esr_to_fault_info(esr);

	if (esr & ESR_ELx_FnV)
		siaddr = NULL;
	else
		siaddr  = (void __user *)addr;
	arm64_notify_die(inf->name, regs, inf->sig, inf->code, siaddr, esr);

	return 0;
}

static const struct fault_info fault_info[] = {
	{ do_bad,		SIGKILL, SI_KERNEL,	"ttbr address size fault"	},
	{ do_bad,		SIGKILL, SI_KERNEL,	"level 1 address size fault"	},
	{ do_bad,		SIGKILL, SI_KERNEL,	"level 2 address size fault"	},
	{ do_bad,		SIGKILL, SI_KERNEL,	"level 3 address size fault"	},
	{ do_translation_fault,	SIGSEGV, SEGV_MAPERR,	"level 0 translation fault"	},
	{ do_translation_fault,	SIGSEGV, SEGV_MAPERR,	"level 1 translation fault"	},
	{ do_translation_fault,	SIGSEGV, SEGV_MAPERR,	"level 2 translation fault"	},
	{ do_translation_fault,	SIGSEGV, SEGV_MAPERR,	"level 3 translation fault"	},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 8"			},
	{ do_page_fault,	SIGSEGV, SEGV_ACCERR,	"level 1 access flag fault"	},
	{ do_page_fault,	SIGSEGV, SEGV_ACCERR,	"level 2 access flag fault"	},
	{ do_page_fault,	SIGSEGV, SEGV_ACCERR,	"level 3 access flag fault"	},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 12"			},
	{ do_page_fault,	SIGSEGV, SEGV_ACCERR,	"level 1 permission fault"	},
	{ do_page_fault,	SIGSEGV, SEGV_ACCERR,	"level 2 permission fault"	},
	{ do_page_fault,	SIGSEGV, SEGV_ACCERR,	"level 3 permission fault"	},
	{ do_sea,		SIGBUS,  BUS_OBJERR,	"synchronous external abort"	},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 17"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 18"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 19"			},
	{ do_sea,		SIGKILL, SI_KERNEL,	"level 0 (translation table walk)"	},
	{ do_sea,		SIGKILL, SI_KERNEL,	"level 1 (translation table walk)"	},
	{ do_sea,		SIGKILL, SI_KERNEL,	"level 2 (translation table walk)"	},
	{ do_sea,		SIGKILL, SI_KERNEL,	"level 3 (translation table walk)"	},
	{ do_sea,		SIGBUS,  BUS_OBJERR,	"synchronous parity or ECC error" },	// Reserved when RAS is implemented
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 25"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 26"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 27"			},
	{ do_sea,		SIGKILL, SI_KERNEL,	"level 0 synchronous parity error (translation table walk)"	},	// Reserved when RAS is implemented
	{ do_sea,		SIGKILL, SI_KERNEL,	"level 1 synchronous parity error (translation table walk)"	},	// Reserved when RAS is implemented
	{ do_sea,		SIGKILL, SI_KERNEL,	"level 2 synchronous parity error (translation table walk)"	},	// Reserved when RAS is implemented
	{ do_sea,		SIGKILL, SI_KERNEL,	"level 3 synchronous parity error (translation table walk)"	},	// Reserved when RAS is implemented
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 32"			},
	{ do_alignment_fault,	SIGBUS,  BUS_ADRALN,	"alignment fault"		},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 34"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 35"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 36"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 37"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 38"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 39"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 40"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 41"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 42"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 43"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 44"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 45"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 46"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 47"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"TLB conflict abort"		},
	{ do_bad,		SIGKILL, SI_KERNEL,	"Unsupported atomic hardware update fault"	},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 50"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 51"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"implementation fault (lockdown abort)" },
	{ do_bad,		SIGBUS,  BUS_OBJERR,	"implementation fault (unsupported exclusive)" },
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 54"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 55"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 56"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 57"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 58" 			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 59"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 60"			},
	{ do_bad,		SIGKILL, SI_KERNEL,	"section domain fault"		},
	{ do_bad,		SIGKILL, SI_KERNEL,	"page domain fault"		},
	{ do_bad,		SIGKILL, SI_KERNEL,	"unknown 63"			},
};

asmlinkage void __exception do_mem_abort(unsigned long addr, unsigned int esr,
					 struct pt_regs *regs)
{
	const struct fault_info *inf = esr_to_fault_info(esr);

	if (!inf->fn(addr, esr, regs))
		return;

	if (!user_mode(regs)) {
		printf("Unhandled fault at 0x%016lx\n", addr);
		mem_abort_decode(esr);
		show_pte(addr);
	}

	arm64_notify_die(inf->name, regs,
			 inf->sig, inf->code, (void __user *)addr, esr);
}

asmlinkage void __exception do_el0_irq_bp_hardening(void)
{
}

asmlinkage void __exception do_el0_ia_bp_hardening(unsigned long addr,
						   unsigned int esr,
						   struct pt_regs *regs)
{
	local_daif_restore(DAIF_PROCCTX);
	do_mem_abort(addr, esr, regs);
}

asmlinkage void __exception do_sp_pc_abort(unsigned long addr,
					   unsigned int esr,
					   struct pt_regs *regs)
{
	if (user_mode(regs)) {
		local_daif_restore(DAIF_PROCCTX);
	}

	arm64_notify_die("SP/PC alignment exception", regs,
			 SIGBUS, BUS_ADRALN, (void __user *)addr, esr);
}

int __init early_brk64(unsigned long addr, unsigned int esr,
		       struct pt_regs *regs);

static struct fault_info debug_fault_info[] = {
	{ do_bad,	SIGTRAP,	TRAP_HWBKPT,	"hardware breakpoint"	},
	{ do_bad,	SIGTRAP,	TRAP_HWBKPT,	"hardware single-step"	},
	{ do_bad,	SIGTRAP,	TRAP_HWBKPT,	"hardware watchpoint"	},
	{ do_bad,	SIGKILL,	SI_KERNEL,	"unknown 3"		},
	{ do_bad,	SIGTRAP,	TRAP_BRKPT,	"aarch32 BKPT"		},
	{ do_bad,	SIGKILL,	SI_KERNEL,	"aarch32 vector catch"	},
	{ early_brk64,	SIGTRAP,	TRAP_BRKPT,	"aarch64 BRK"		},
	{ do_bad,	SIGKILL,	SI_KERNEL,	"unknown 7"		},
};

asmlinkage int __exception do_debug_exception(unsigned long addr,
					      unsigned int esr,
					      struct pt_regs *regs)
{
	const struct fault_info *inf = esr_to_debug_fault_info(esr);
	int rv;

	if (!inf->fn(addr, esr, regs)) {
		rv = 1;
	} else {
		arm64_notify_die(inf->name, regs,
				 inf->sig, inf->code, (void __user *)addr, esr);
		rv = 0;
	}

	return rv;
}
