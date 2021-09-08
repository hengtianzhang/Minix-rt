#include <base/common.h>

#include <sel4m/reboot.h>
#include <sel4m/irqflags.h>
#include <sel4m/sched.h>

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
	//smp_send_stop();

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
	//smp_send_stop();
	if (pm_power_off)
		pm_power_off();
}

struct task_struct *__entry_task[CONFIG_NR_CPUS];

struct task_struct *__switch_to(struct task_struct *prev,
				struct task_struct *next)
{
	return NULL;
}
