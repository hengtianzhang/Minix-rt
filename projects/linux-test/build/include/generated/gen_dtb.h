
#ifndef __GENERATED_GEN_DTB_H_
#define __GENERATED_GEN_DTB_H_

#ifndef __ASSEMBLY__

#include <base/types.h>
#include <base/init.h>

static const u32 fdt_totalsize __initconst = 0x100000;

struct memory_reg {
	phys_addr_t base;
	size_t     size;
#define NO_MAP  1
	int         flags;
};

struct devcie_node {
	char *compatile;
	struct memory_reg reg;
};

struct psci_devices {
	phys_addr_t migrate;
	phys_addr_t cpu_on;
	phys_addr_t cpu_off;
	phys_addr_t cpu_suspend;
#define HVC_METHOD  1
#define SMC_METHOD  2
	int			method;
	int			valid;
	char *compatible;
};

struct interrupt_devices {
	struct memory_reg dist_base;
    struct memory_reg cpu_base;
    struct interrupt_devices *child;
	char *compatible;
};

static const char machine_name[] __initconst = "linux,dummy-virt";

static const
phys_addr_t phys_initrd_start __initconst = 0x0;

static const
phys_addr_t phys_initrd_end __initconst = 0x0;

static const
struct memory_reg dtb_memory[] __initconst = {
	{ .base = 0x40000000, .size = 0x100000000, .flags = 0 },
};

static const
struct memory_reg dtb_reserverd_memory[] __initconst = {
	{ .base = 0x0, .size = 0x0, .flags = 0 },
};

static const
struct devcie_node dtb_stdout_path __initconst = {
	.compatile = "arm,pl011 arm,primecell ",
	.reg = { .base = 0x9000000, .size = 0x1000, .flags = 0 },
};

static const
struct psci_devices psci_dt __initconst = {
	.migrate = 0xc4000005,
	.cpu_on = 0xc4000003,
	.cpu_off = 0x84000002,
	.cpu_suspend = 0xc4000001,
	.method = HVC_METHOD,
	.compatible = "arm,psci-0.2 arm,psci ",
	.valid = 1,
};

struct cpus_desc {
    u32 reg;
#define ENABLE_METHOD_PSCI 1
#define ENABLE_METHOD_SPIN_TABLE 2
    u32 enable_method;
    char *compatible;
};

static const
struct cpus_desc cpus_dt[] __initconst = {
	{ .reg = 0x00000000,.enable_method = ENABLE_METHOD_PSCI,.compatible = "arm,cortex-a57 "},
	{ .reg = 0x00000001,.enable_method = ENABLE_METHOD_PSCI,.compatible = "arm,cortex-a57 "},
	{ .reg = 0x00000002,.enable_method = ENABLE_METHOD_PSCI,.compatible = "arm,cortex-a57 "},
	{ .reg = 0x00000003,.enable_method = ENABLE_METHOD_PSCI,.compatible = "arm,cortex-a57 "},
	{ .reg = 0x00000004,.enable_method = ENABLE_METHOD_PSCI,.compatible = "arm,cortex-a57 "},
	{ .reg = 0x00000005,.enable_method = ENABLE_METHOD_PSCI,.compatible = "arm,cortex-a57 "},
	{ .reg = 0x00000006,.enable_method = ENABLE_METHOD_PSCI,.compatible = "arm,cortex-a57 "},
	{ .reg = 0x00000007,.enable_method = ENABLE_METHOD_PSCI,.compatible = "arm,cortex-a57 "},
	
};

static const
struct interrupt_devices interrupt_dt __initconst = {
	.dist_base = { .base = 0x8000000, .size = 0x10000, .flags = 0 },.cpu_base = { .base = 0x8010000, .size = 0x10000, .flags = 0 },
	.child = NULL,
	.compatible = "arm,cortex-a15-gic",
};

struct interrupt_desc {
	u32		irq_type;
	u32		number;
	u32		trig_type;
};

static const
struct interrupt_desc timer_descs[] = {
	{.irq_type = 0x1, .number = 0xd, .trig_type = 0xff04, },
	{.irq_type = 0x1, .number = 0xe, .trig_type = 0xff04, },
	{.irq_type = 0x1, .number = 0xb, .trig_type = 0xff04, },
	{.irq_type = 0x1, .number = 0xa, .trig_type = 0xff04, },
	
};

struct timer_devices {
	const struct interrupt_desc *timer_desc;
	int 	always_on;
	char 	*compatible;
};

static const
struct timer_devices timer_dt __initconst = {
	.timer_desc = timer_descs,
	.always_on = 0,
	.compatible = "arm,armv8-timer arm,armv7-timer ",
}; 

#endif /* !__ASSEMBLY__ */
#endif /* !__GENERATED_GEN_DTB_H_ */
