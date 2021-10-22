/*
 * Generic helpers for smp ipi calls
 *
 * (C) Jens Axboe <jens.axboe@oracle.com> 2008
 */

#include <base/linkage.h>
#include <base/init.h>

#include <of/of.h>

#include <minix_rt/smp.h>
#include <minix_rt/cpumask.h>
#include <minix_rt/ktime.h>
#include <minix_rt/cpu.h>
#include <minix_rt/irq.h>
#include <minix_rt/delay.h>

#include <asm/cache.h>
#include <asm/cacheflush.h>
#include <asm/cpu.h>
#include <asm/cputype.h>
#include <asm/cpu_ops.h>
#include <asm/mmu_context.h>
#include <asm/daifflags.h>
#include <asm/cpufeature.h>

#define NR_IPI	7

static int __initdata cpu_running[CONFIG_NR_CPUS];

/*
 * as from 2.5, kernels no longer have an init_tasks structure
 * so we need some other way of telling a new secondary core
 * where to place its SVC stack
 */
struct secondary_data secondary_data;
/* Number of CPUs which aren't online, but looping in kernel text. */
int cpus_stuck_in_kernel;

enum ipi_msg_type {
	IPI_RESCHEDULE,
	IPI_CALL_FUNC,
	IPI_CPU_STOP,
	IPI_CPU_CRASH_STOP,
	IPI_TIMER,
	IPI_IRQ_WORK,
	IPI_WAKEUP
};

static inline int op_cpu_kill(unsigned int cpu)
{
	return -ENOSYS;
}

/*
 * Boot a secondary CPU, and assign it the specified idle task.
 * This also gives us the initial stack to use for this CPU.
 */
static int boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	if (cpu_ops[cpu]->cpu_boot)
		return cpu_ops[cpu]->cpu_boot(cpu);

	return -EOPNOTSUPP;
}

int __init __cpu_up(unsigned int cpu, struct task_struct *idle)
{
	int ret;
	s64 status;

	/*
	 * We need to tell the secondary core where to find its stack and the
	 * page tables.
	 */
	secondary_data.task = idle;
	secondary_data.stack = task_stack_page(idle) + THREAD_SIZE;
	update_cpu_boot_status(CPU_MMU_OFF);
	__flush_dcache_area(&secondary_data, sizeof(secondary_data));

	/*
	 * Now bring the CPU into our world.
	 */
	ret = boot_secondary(cpu, idle);
	if (ret == 0) {
		ktime_t delta = 1 * NSEC_PER_SEC;
		ktime_t start_time = ktime_get();

		while (start_time + delta > ktime_get()) {
			wfi();
			wfe();
			if (cpu_running[cpu] == 1) 
				break;
		}

		if (!cpu_online(cpu)) {
			printf("CPU%u: failed to come online\n", cpu);
			ret = -EIO;
		}
	} else {
		printf("CPU%u: failed to boot: %d\n", cpu, ret);
		return ret;
	}

	secondary_data.task = NULL;
	secondary_data.stack = NULL;
	status = READ_ONCE(secondary_data.status);

	if (ret && status) {

		if (status == CPU_MMU_OFF)
			status = READ_ONCE(__early_cpu_boot_status);

		switch (status & CPU_BOOT_STATUS_MASK) {
		default:
			printf("CPU%u: failed in unknown state : 0x%llx\n",
					cpu, status);
			break;
		case CPU_KILL_ME:
			if (!op_cpu_kill(cpu)) {
				printf("CPU%u: died during early boot\n", cpu);
				break;
			}
			/* Fall through */
			printf("CPU%u: may not have shut down cleanly\n", cpu);
		case CPU_STUCK_IN_KERNEL:
			printf("CPU%u: is stuck in kernel\n", cpu);
			if (status & CPU_STUCK_REASON_52_BIT_VA)
				printf("CPU%u: does not support 52-bit VAs\n", cpu);
			if (status & CPU_STUCK_REASON_NO_GRAN)
				printf("CPU%u: does not support %luK granule \n", cpu, PAGE_SIZE / SZ_1K);
			break;
		case CPU_PANIC_KERNEL:
			panic("CPU%u detected unsupported configuration\n", cpu);
		}
	}

	return ret;
}

/*
 * This is the secondary CPU boot entry.  We're using this CPUs
 * idle thread stack, but a set of temporary page tables.
 */
asmlinkage void secondary_start_kernel(void)
{
	u64 mpidr = read_cpuid_mpidr() & MPIDR_HWID_BITMASK;
	unsigned int cpu;

	cpu = task_cpu(current);
	set_my_cpu_offset(cpu);

	/*
	 * TTBR0 is only used for the identity mapping at this stage. Make it
	 * point to zero page to avoid speculatively fetching new entries.
	 */
	cpu_uninstall_idmap();

	preempt_disable();

	/*
	 * If the system has established the capabilities, make sure
	 * this CPU ticks all of those. If it doesn't, the CPU will
	 * fail to come online.
	 */
	check_local_cpu_capabilities();

	if (cpu_ops[cpu]->cpu_postboot)
		cpu_ops[cpu]->cpu_postboot();

	/*
	 * Log the CPU info before it is marked online and might get read.
	 */
	cpuinfo_store_cpu();

	/*
	 * Enable GIC and timers.
	 */
	notify_cpu_starting(cpu);

	/*
	 * OK, now it's safe to let the boot CPU continue.  Wait for
	 * the CPU migration code to notice that the CPU is online
	 * before we continue.
	 */
	printf("CPU%u: Booted secondary processor 0x%010llx [0x%08x]\n",
					 cpu, (u64)mpidr,
					 read_cpuid_id());
	update_cpu_boot_status(CPU_BOOT_SUCCESS);
	set_cpu_online(cpu, true);
	cpu_running[cpu] = 1;

	local_daif_restore(DAIF_PROCCTX);

	system_tick_init();

	cpu_idle();
}

void __init smp_cpus_done(unsigned int max_cpus)
{
	printf("SMP: Total of %d processors activated.\n", num_online_cpus());
	setup_cpu_features();
	mark_linear_text_alias_ro();
}

/*
 * Kill the calling secondary CPU, early in bringup before it is turned
 * online.
 */
void cpu_die_early(void)
{
	int cpu = smp_processor_id();

	printf("CPU%d: will not boot\n", cpu);

	/* Mark this CPU absent */
	set_cpu_present(cpu, 0);

	update_cpu_boot_status(CPU_STUCK_IN_KERNEL);

	cpu_park_loop();
}

void __init smp_prepare_boot_cpu(void)
{
	set_my_cpu_offset(smp_processor_id());

	cpuinfo_store_boot_cpu();
}

static u64 __init of_get_cpu_mpidr(struct device_node *dn)
{
	const __be32 *cell;
	u64 hwid;

	/*
	 * A cpu node with missing "reg" property is
	 * considered invalid to build a cpu_logical_map
	 * entry.
	 */
	cell = of_get_property(dn, "reg", NULL);
	if (!cell) {
		printf("%pOF: missing reg property\n", dn);
		return INVALID_HWID;
	}

	hwid = of_read_number(cell, of_n_addr_cells(dn));
	/*
	 * Non affinity bits must be set to 0 in the DT
	 */
	if (hwid & ~MPIDR_HWID_BITMASK) {
		printf("%pOF: invalid reg property\n", dn);
		return INVALID_HWID;
	}
	return hwid;
}

/*
 * Duplicate MPIDRs are a recipe for disaster. Scan all initialized
 * entries and check for duplicates. If any is found just ignore the
 * cpu. cpu_logical_map was initialized to INVALID_HWID to avoid
 * matching valid MPIDR values.
 */
static bool __init is_mpidr_duplicate(unsigned int cpu, u64 hwid)
{
	unsigned int i;

	for (i = 1; (i < cpu) && (i < CONFIG_NR_CPUS); i++)
		if (cpu_logical_map(i) == hwid)
			return true;
	return false;
}

/*
 * Initialize cpu operations for a logical cpu and
 * set it in the possible mask on success
 */
static int __init smp_cpu_setup(int cpu)
{
	if (cpu_read_ops(cpu))
		return -ENODEV;

	if (cpu_ops[cpu]->cpu_init(cpu))
		return -ENODEV;

	set_cpu_possible(cpu, true);

	return 0;
}

static bool bootcpu_valid __initdata;
static unsigned int cpu_count = 1;

/*
 * Enumerate the possible CPU set from the device tree and build the
 * cpu logical map array containing MPIDR values related to logical
 * cpus. Assumes that cpu_logical_map(0) has already been initialized.
 */
static void __init of_parse_and_init_cpus(void)
{
	struct device_node *dn;

	for_each_of_cpu_node(dn) {
		u64 hwid = of_get_cpu_mpidr(dn);

		if (hwid == INVALID_HWID)
			goto next;

		if (is_mpidr_duplicate(cpu_count, hwid)) {
			printf("%pOF: duplicate cpu reg properties in the DT\n",
				dn);
			goto next;
		}

		/*
		 * The numbering scheme requires that the boot CPU
		 * must be assigned logical id 0. Record it so that
		 * the logical map built from DT is validated and can
		 * be used.
		 */
		if (hwid == cpu_logical_map(0)) {
			if (bootcpu_valid) {
				printf("%pOF: duplicate boot cpu reg property in DT\n",
					dn);
				goto next;
			}

			bootcpu_valid = true;

			/*
			 * cpu_logical_map has already been
			 * initialized and the boot cpu doesn't need
			 * the enable-method so continue without
			 * incrementing cpu.
			 */
			continue;
		}

		if (cpu_count >= CONFIG_NR_CPUS)
			goto next;

		pr_debug("cpu logical map 0x%llx\n", hwid);
		cpu_logical_map(cpu_count) = hwid;

next:
		cpu_count++;
	}
}

/*
 * Enumerate the possible CPU set from the device tree or ACPI and build the
 * cpu logical map array containing MPIDR values related to logical
 * cpus. Assumes that cpu_logical_map(0) has already been initialized.
 */
void __init smp_init_cpus(void)
{
	int i;

	of_parse_and_init_cpus();

	if (cpu_count > nr_cpu_ids)
		printf("Number of cores (%d) exceeds configured maximum of %lu - clipping\n",
			cpu_count, nr_cpu_ids);

	if (!bootcpu_valid) {
		printf("missing boot CPU MPIDR, not enabling secondaries\n");
		return;
	}

	/*
	 * We need to set the cpu_logical_map entries before enabling
	 * the cpus so that cpu processor description entries (DT cpu nodes
	 * and ACPI MADT entries) can be retrieved by matching the cpu hwid
	 * with entries in cpu_logical_map while initializing the cpus.
	 * If the cpu set-up fails, invalidate the cpu_logical_map entry.
	 */
	for (i = 1; i < nr_cpu_ids; i++) {
		if (cpu_logical_map(i) != INVALID_HWID) {
			if (smp_cpu_setup(i))
				cpu_logical_map(i) = INVALID_HWID;
		}
	}
}

void __init smp_prepare_cpus(unsigned int max_cpus)
{
	int err;
	unsigned int cpu;
	unsigned int this_cpu;

	this_cpu = smp_processor_id();

	/*
	 * If UP is mandated by "nosmp" (which implies "maxcpus=0"), don't set
	 * secondary CPUs present.
	 */
	if (max_cpus == 0)
		return;

	/*
	 * Initialise the present map (which describes the set of CPUs
	 * actually populated at the present time) and release the
	 * secondaries from the bootloader.
	 */
	for_each_possible_cpu(cpu) {
		if (cpu == smp_processor_id())
			continue;

		if (!cpu_ops[cpu])
			continue;

		err = cpu_ops[cpu]->cpu_prepare(cpu);
		if (err)
			continue;

		set_cpu_present(cpu, true);
	}
}

void (*__smp_cross_call)(const struct cpumask *, unsigned int);

void __init set_smp_cross_call(void (*fn)(const struct cpumask *, unsigned int))
{
	__smp_cross_call = fn;
}

static void smp_cross_call(const struct cpumask *target, unsigned int ipinr)
{
	__smp_cross_call(target, ipinr);
}

void arch_send_call_function_ipi_mask(const struct cpumask *mask)
{
	smp_cross_call(mask, IPI_CALL_FUNC);
}

void arch_send_call_function_single_ipi(int cpu)
{
	smp_cross_call(cpumask_of(cpu), IPI_CALL_FUNC);
}

/*
 * ipi_cpu_stop - handle IPI from smp_send_stop()
 */
static void ipi_cpu_stop(unsigned int cpu)
{
	set_cpu_online(cpu, false);

	local_daif_mask();

	while (1)
		cpu_relax();
}

/*
 * Main handler for inter-processor interrupts
 */
void handle_IPI(int ipinr, struct pt_regs *regs)
{
	unsigned int cpu = smp_processor_id();
	struct pt_regs *old_regs = set_irq_regs(regs);

	switch (ipinr) {
	case IPI_RESCHEDULE:
		//scheduler_ipi();
		break;

	case IPI_CALL_FUNC:
		irq_enter();
		generic_smp_call_function_interrupt();
		irq_exit();
		break;

	case IPI_CPU_STOP:
		irq_enter();
		ipi_cpu_stop(cpu);
		irq_exit();
		break;

	case IPI_CPU_CRASH_STOP:
		break;

	default:
		printf("CPU%u: Unknown IPI message 0x%x\n", cpu, ipinr);
		break;
	}

	set_irq_regs(old_regs);
}

void smp_send_reschedule(int cpu)
{
	smp_cross_call(cpumask_of(cpu), IPI_RESCHEDULE);
}

void smp_send_stop(void)
{
	unsigned long timeout;

	if (num_online_cpus() > 1) {
		cpumask_t mask;

		cpumask_copy(&mask, cpu_online_mask);
		cpumask_clear_cpu(smp_processor_id(), &mask);

		if (system_state <= SYSTEM_RUNNING)
			printf("SMP: stopping secondary CPUs\n");
		smp_cross_call(&mask, IPI_CPU_STOP);
	}

	/* Wait up to one second for other CPUs to stop */
	timeout = USEC_PER_SEC;
	while (num_online_cpus() > 1 && timeout--)
		udelay(1);

	if (num_online_cpus() > 1)
		printf("SMP: failed to stop secondary CPUs %ld %p\n",
			   cpumask_pr_args(cpu_online_mask));
}
