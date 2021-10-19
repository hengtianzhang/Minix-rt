#include <base/linkage.h>
#include <base/common.h>
#include <base/compiler.h>
#include <base/init.h>

#include <minix_rt/smp.h>
#include <minix_rt/spinlock.h>

#include <asm/current.h>
#include <asm/system_misc.h>

#ifdef CONFIG_STACKPROTECTOR
/*
 * Called when gcc's -fstack-protector feature is used, and
 * gcc detects corruption of the on-stack canary value
 */
__visible void __stack_chk_fail(void)
{
	panic("stack-protector: Kernel stack is corrupted in: %pB",
		__builtin_return_address(0));
}
#endif

static char dump_stack_arch_desc_str[128];

/**
 * dump_stack_set_arch_desc - set arch-specific str to show with task dumps
 * @fmt: printf-style format string
 * @...: arguments for the format string
 *
 * The configured string will be printed right after utsname during task
 * dumps.  Usually used to add arch-specific system identifiers.  If an
 * arch wants to make use of such an ID string, it should initialize this
 * as soon as possible during boot.
 */
void __init dump_stack_set_arch_desc(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vsnprintf(dump_stack_arch_desc_str, sizeof(dump_stack_arch_desc_str),
		  fmt, args);
	va_end(args);
}

void panic(const char *fmt, ...)
{
	int this_cpu = smp_processor_id();
	static char buf[1024];
	s64 len;
	va_list args;

	/*
	 * Disable local interrupts. This will prevent panic_smp_self_stop
	 * from deadlocking the first cpu that invokes the panic, since
	 * there is nothing to prevent an interrupt handler (that runs
	 * after setting panic_cpu) from invoking panic() again.
	 */
	local_irq_disable();

	va_start(args, fmt);
	len = vscnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (len && buf[len - 1] == '\n')
		buf[len - 1] = '\0';

	smp_send_stop();

	printf("Kernel panic on CPU%d - not syncing: %s\n", this_cpu, buf);
	printf("---[ end Kernel panic - not syncing: %s ]---\n", buf);
	while (1)
		cpu_relax();
}

/**
 * dump_stack_print_info - print generic debug info for dump_stack()
 * @log_lvl: log level
 *
 * Arch-specific dump_stack() implementations can use this function to
 * print out the same debug information as the generic dump_stack().
 */
void dump_stack_print_info(void)
{
	printf("CPU: %lld PID: %d Comm: %.20s\n",
	       smp_processor_id(), current->pid.pid, current->comm);

	if (dump_stack_arch_desc_str[0] != '\0')
		printf("Hardware name: %s\n", dump_stack_arch_desc_str);
}

/**
 * show_regs_print_info - print generic debug info for show_regs()
 * @log_lvl: log level
 *
 * show_regs() implementations can use this function to print out generic
 * debug information.
 */
void show_regs_print_info(void)
{
	dump_stack_print_info();
}

static void __dump_stack(void)
{
	dump_stack_print_info();
	show_stack(NULL, NULL);
}

static atomic_t dump_lock = ATOMIC_INIT(-1);

asmlinkage __visible void dump_stack(void)
{
	u64 flags;
	int was_locked;
	int old;
	int cpu;

	/*
	 * Permit this cpu to perform nested stack dumps while serialising
	 * against other CPUs
	 */
retry:
	local_irq_save(flags);
	cpu = smp_processor_id();
	old = atomic_cmpxchg(&dump_lock, -1, cpu);
	if (old == -1) {
		was_locked = 0;
	} else if (old == cpu) {
		was_locked = 1;
	} else {
		local_irq_restore(flags);
		cpu_relax();
		goto retry;
	}

	__dump_stack();

	if (!was_locked)
		atomic_set(&dump_lock, -1);

	local_irq_restore(flags);
}
