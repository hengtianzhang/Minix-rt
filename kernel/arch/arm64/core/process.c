#include <base/common.h>

#include <sel4m/reboot.h>
#include <sel4m/irqflags.h>
#include <sel4m/sched.h>
#include <sel4m/stat.h>

#include <asm/stacktrace.h>
#include <asm/mmu_context.h>
#include <asm/ptrace.h>

/*
 * Function pointers to optional machine specific functions
 */
void (*pm_power_off)(void);

void (*arm_pm_restart)(enum reboot_mode reboot_mode, const char *cmd);

/*
 * Restart requires that the secondary CPUs stop performing any activity
 * while the primary CPU resets the system. Systems with multiple CPUs must
 * provide a HW restart implementation, to ensure that all CPUs reset at once.
 * This is required so that any code running after reset on the primary CPU
 * doesn't have to co-ordinate with other CPUs to ensure they aren't still
 * executing pre-reset code, and using RAM that the primary CPU's code wishes
 * to use. Implementing such co-ordination would be essentially impossible.
 */
void machine_restart(char *cmd)
{
	/* Disable interrupts first */
	local_irq_disable();
	smp_send_stop();

	/* Now call the architecture specific reboot code. */
	if (arm_pm_restart)
		arm_pm_restart(reboot_mode, cmd);
	else
		printf("Reboot failed -- System halted\n");

	while (1);
}

/*
 * Power-off simply requires that the secondary CPUs stop performing any
 * activity (executing tasks, handling interrupts). smp_send_stop()
 * achieves this. When the system power is turned off, it will take all CPUs
 * with it.
 */
void machine_power_off(void)
{
	local_irq_disable();
	smp_send_stop();
	if (pm_power_off)
		pm_power_off();
}

/*
 * This is our default idle handler.
 */
void arch_cpu_idle(void)
{
	cpu_do_idle();
	local_irq_enable();
}

struct task_struct *__entry_task[CONFIG_NR_CPUS];

struct task_struct *__switch_to(struct task_struct *prev,
				struct task_struct *next)
{
	return NULL;
}

static void print_pstate(struct pt_regs *regs)
{
	u64 pstate = regs->pstate;

	printf("pstate: %08llx (%c%c%c%c %c%c%c%c %cPAN %cUAO)\n",
			pstate,
			pstate & PSR_N_BIT ? 'N' : 'n',
			pstate & PSR_Z_BIT ? 'Z' : 'z',
			pstate & PSR_C_BIT ? 'C' : 'c',
			pstate & PSR_V_BIT ? 'V' : 'v',
			pstate & PSR_D_BIT ? 'D' : 'd',
			pstate & PSR_A_BIT ? 'A' : 'a',
			pstate & PSR_I_BIT ? 'I' : 'i',
			pstate & PSR_F_BIT ? 'F' : 'f',
			pstate & PSR_PAN_BIT ? '+' : '-',
			pstate & PSR_UAO_BIT ? '+' : '-');
}

void __show_regs(struct pt_regs *regs)
{
	int i, top_reg;
	u64 lr, sp;

	lr = regs->regs[30];
	sp = regs->sp;
	top_reg = 29;

	show_regs_print_info();
	print_pstate(regs);

	if (!user_mode(regs)) {
		printf("pc : %p\n", (void *)regs->pc);
		printf("lr : %p\n", (void *)lr);
	} else {
		printf("pc : %016llx\n", regs->pc);
		printf("lr : %016llx\n", lr);
	}

	printf("sp : %016llx\n", sp);

	i = top_reg;

	while (i >= 0) {
		printf("x%-2d: %016llx ", i, regs->regs[i]);
		i--;

		if (i % 2 == 0) {
			printf(KERN_CONT "x%-2d: %016llx ", i, regs->regs[i]);
			i--;
		}

		printf(KERN_CONT "\n");
	}
}

void show_regs(struct pt_regs * regs)
{
	__show_regs(regs);
	dump_backtrace(regs, NULL);
}
