#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

"""
This script gen dtb to c sources file.
"""


import argparse
import sys
from pyfdt.pyfdt import FDT_PROP, FdtBlobParse
import json


def fdt_check_address_cell(fdt_data):
	global address_cells
	for location in fdt_data:
		if location == "#address-cells":
			address_cells = int(fdt_data["#address-cells"][1], 16)


def fdt_check_size_cell(fdt_data):
	global size_cells
	for location in fdt_data:
		if location == "#size-cells":
			size_cells = int(fdt_data["#size-cells"][1], 16)


def fdt_foreach_property(fdt_property, fdt_data, fdt_key):
	for location in fdt_data:
		if location == fdt_key:
			fdt_property.append(fdt_data)

		if isinstance(fdt_data[location], dict):
			fdt_foreach_property(fdt_property, fdt_data[location], fdt_key)


def scan_memory_dump(dump_data, flags, name_base=[]):
	str_buffer = ""
	prefix = ""
	index = 0
	for location in dump_data:
		if len(name_base) != 0:
			prefix = name_base[index]
			prefix += " = "
			index += 1

		str_buffer += "%s{ .base = 0x%lx, .size = 0x%lx, .flags = %lx },"   \
			% (prefix, location[0], location[1], flags)

	return str_buffer


def foreach_reg(fdt_property, address_cells, size_cells, index, reg):
	if address_cells == 2:
		high = int(fdt_property["reg"][index], 16) << 32
		index += 1
		low = int(fdt_property["reg"][index], 16)
		address = high + low
	else:
		address = int(fdt_property["reg"][index], 16)

	index += 1
	if size_cells == 2:
		high = int(fdt_property["reg"][index], 16) << 32
		index += 1
		low = int(fdt_property["reg"][index], 16)
		size = high + low
	else:
		size = int(fdt_property["reg"][index], 16)

	index += 1
	reg.append(address)
	reg.append(size)
	if index < len(fdt_property["reg"]):
		foreach_reg(fdt_property, address_cells, size_cells, index, reg)


def dump_memory_reg(fdt_property, address_cells, size_cells):
	index = 1
	reg = []
	foreach_reg(fdt_property, address_cells, size_cells, index, reg)

	return reg


def fdt_scan_initrd(fdt_data, output_file):
	initrd_start = ""
	initrd_end = ""
	for location in fdt_data:
		if location == "chosen":
			for i in fdt_data["chosen"]:
				if i == "linux,initrd-start":
					initrd_start = fdt_data["chosen"][i][1]
				if i == "linux,initrd-end":
					initrd_end = fdt_data["chosen"][i][1]

	if len(initrd_start) == 0:
		initrd_start_value = 0
	else:
		initrd_start_value = int(initrd_start, 16)

	if len(initrd_end) == 0:
		initrd_end_value = 0
	else:
		initrd_end_value = int(initrd_end, 16)

	output_file.write("""
static const
phys_addr_t phys_initrd_start __initconst = 0x%lx;

static const
phys_addr_t phys_initrd_end __initconst = 0x%lx;
""" % (initrd_start_value, initrd_end_value))


def fdt_scan_memory(fdt_data, output_file):
	fdt_property = []
	fdt_foreach_property(fdt_property, fdt_data, "device_type")

	memory_reg = []
	for location in fdt_property:
		if location["device_type"][1] == "memory":
			reg = dump_memory_reg(location, address_cells, size_cells)
			memory_reg.append(reg)

	output_file.write("""
static const
struct memory_reg dtb_memory[] __initconst = {
	%s
};
""" % scan_memory_dump(memory_reg, 0))


def scan_stdout_dump(compatile):
	str_buffer = ""
	for i in compatile:
		str_buffer += "%s " % i
	return str_buffer


def fdt_scan_stdout_path(fdt_data, output_file):
	# get stdout path
	stdout_path = ""
	for location in fdt_data:
		if location == "chosen":
			for i in fdt_data["chosen"]:
				if i == "stdout-path" or i == "linux,stdout-path":
					stdout_path = fdt_data["chosen"][i][1].replace('/', '')

	stdout_path_list = []
	fdt_foreach_property(stdout_path_list, fdt_data, stdout_path)
	for i in stdout_path_list:
		for j in i[stdout_path]:
			if j == "compatible":
				stdout_node = i[stdout_path]
				compatile = stdout_node["compatible"]
				del compatile[0]
				memory_reg = []
				reg = dump_memory_reg(stdout_node, address_cells, size_cells)
				memory_reg.append(reg)
				output_file.write("""
static const
struct devcie_node dtb_stdout_path __initconst = {
	.compatile = "%s",
	.reg = %s
};
""" % (scan_stdout_dump(compatile), scan_memory_dump(memory_reg, 0)))


def dump_reserverd_memory(fdt_data, fdt):
	str_buffer = ""
	for i in fdt.reserve_entries:
		str_buffer += "{ .base = 0x%lx, .size = 0x%lx, .flags = 0 }," % \
			(i["address"], i["size"])

	for i in fdt_data:
		if i == "reserved-memory":
			for j in fdt_data[i]:
				status = 1
				no_map = 0
				if isinstance(fdt_data[i][j], dict):
					for k in fdt_data[i][j]:
						if k == "status":
							if fdt_data[i][j][k][1] != "ok" and fdt_data[i][j][k][1] != "okay":
								status = 0
						if k == "reg":
							reg = dump_memory_reg(fdt_data[i][j], address_cells, size_cells)
						if k == "no-map":
							no_map = 1
					if status == 0:
						continue
					str_buffer += "{ .base = 0x%lx, .size = 0x%lx, .flags = %d }," % \
						(reg[0], reg[1], no_map)

	str_buffer += "{ .base = 0x0, .size = 0x0, .flags = 0 },"

	return str_buffer


def fdt_scan_reserverd_memory(fdt_data, fdt, output_file):
	output_file.write("""
static const
struct memory_reg dtb_reserverd_memory[] __initconst = {
	%s
};
""" % dump_reserverd_memory(fdt_data, fdt))


def dump_psci_dt(fdt_data):
	str_buffer = ""
	valid = 0
	for i in fdt_data:
		if i == "psci":
			valid = 1
	if valid == 0:
		str_buffer += ".valid = 0,\n\t"
		return str_buffer
	
	for i in fdt_data["psci"]:
		if i == "migrate":
			str_buffer += ".migrate = %s,\n\t" % fdt_data["psci"][i][1]
		if i == "cpu_on":
			str_buffer += ".cpu_on = %s,\n\t" % fdt_data["psci"][i][1]
		if i == "cpu_off":
			str_buffer += ".cpu_off = %s,\n\t" % fdt_data["psci"][i][1]
		if i == "cpu_suspend":
			str_buffer += ".cpu_suspend = %s,\n\t" % fdt_data["psci"][i][1]
		if i == "method":
			if fdt_data["psci"]["method"][1] == "hvc":
				str_buffer += ".method = HVC_METHOD,\n\t"
			else:
				str_buffer += ".method = SMC_METHOD,\n\t"
		if i == "compatible":
			compatile = fdt_data["psci"]["compatible"]
			del compatile[0]
			str_buffer += ".compatible = \""
			for j in compatile:
				str_buffer += "%s " % j
			str_buffer += "\",\n\t"

	str_buffer += ".valid = 1,"

	return str_buffer


def fdt_scan_psci(fdt_data, output_file):
	output_file.write("""
static const
struct psci_devices psci_dt __initconst = {
	%s
};
""" % (dump_psci_dt(fdt_data)))


def dump_cpus_desc(fdt_property):
	str_buffer = ""
	for j in fdt_property:
		str_buffer += "{ "
		if j["device_type"][1] == "cpu":
			str_buffer += ".reg = %s," % (j["reg"][1])
			enable_method = j["enable-method"][1]
			if enable_method == "psci":
				str_buffer += ".enable_method = ENABLE_METHOD_PSCI,"
			else:
				str_buffer += ".enable_method = ENABLE_METHOD_SPIN_TABLE,"
			compatible = j["compatible"]
			del compatible[0]
			str_buffer += ".compatible = \""
			for k in compatible:
				str_buffer += "%s " % k
			str_buffer += "\""
		str_buffer += "},\n\t"
	return str_buffer


def fdt_scan_cpus(fdt_data, output_file):
	output_file.write("""
struct cpus_desc {
    u32 reg;
#define ENABLE_METHOD_PSCI 1
#define ENABLE_METHOD_SPIN_TABLE 2
    u32 enable_method;
    char *compatible;
};
""")
	for i in fdt_data:
		if i == "cpus":
			fdt_property = []
			fdt_foreach_property(fdt_property, fdt_data[i], "device_type")
			output_file.write("""
static const
struct cpus_desc cpus_dt[] __initconst = {
	%s
};
""" % (dump_cpus_desc(fdt_property)))


def fdt_scan_interrupt(fdt_data, output_file):
	global interrupt_cells
	interrupt_cells = 0
	for i in fdt_data:
		if i == "interrupt-parent":
			phandle = fdt_data[i][1]
			fdt_property = []
			fdt_foreach_property(fdt_property, fdt_data, "phandle")
			for j in fdt_property:
				if j["phandle"][1] == phandle:
					interrupt_node = j
					break
			break
	for i in interrupt_node:
		if i == "#interrupt-cells":
			interrupt_cells = int(interrupt_node["#interrupt-cells"][1], 16)
		if i == "interrupt-controller":
			devices_reg = []
			reg = dump_memory_reg(interrupt_node, address_cells, size_cells)
			for j in range(0, len(reg), 2):
				devices_reg.append(reg[j:j + 2])
			name_base = [".dist_base", ".cpu_base"]
			compatible = interrupt_node["compatible"]
			del compatible[0]
			str_compatible = ""
			for k in compatible:
				str_compatible += "%s" % k
			output_file.write("""
static const
struct interrupt_devices interrupt_dt __initconst = {
	%s
	.child = NULL,
	.compatible = "%s",
};
""" % (scan_memory_dump(devices_reg, 0, name_base),
	str_compatible))


def dump_timer_devices_desc(interrupts_desc):
	str_buffer = ""
	prefix = ["irq_type", "number", "trig_type"]
	for i  in interrupts_desc:
		str_buffer += "{"
		index = 0
		for j in i:
			str_buffer += ".%s = 0x%lx, " % (prefix[index], j)
			index += 1
		str_buffer += "},\n\t"
	return str_buffer


def fdt_scan_timer(fdt_data, output_file):
	if interrupt_cells == 0:
		return 0

	for i in fdt_data:
		if i == "timer":
			output_file.write("""
struct interrupt_desc {
	u32		irq_type;
	u32		number;
	u32		trig_type;
};
""")
			for j in fdt_data[i]:
				always_on = 0
				if j == "always-on":
					always_on = 1
				if j == "compatible":
					compatible = fdt_data[i]["compatible"]
					del compatible[0]
					str_compatible = ""
					for k in compatible:
						str_compatible += "%s " % k
				if j == "interrupts":
					interrupts = fdt_data[i][j]
					del interrupts[0]
					interrupts = [ int(x, 16) for x in interrupts ]
					interrupts_desc = []
					for j in range(0, len(interrupts), interrupt_cells):
						interrupts_desc.append(interrupts[j:j + interrupt_cells])
					output_file.write("""
static const
struct interrupt_desc timer_descs[] = {
	%s
};

struct timer_devices {
	const struct interrupt_desc *timer_desc;
	int 	always_on;
	char 	*compatible;
};
""" % (dump_timer_devices_desc(interrupts_desc)))
			output_file.write("""
static const
struct timer_devices timer_dt __initconst = {
	.timer_desc = timer_descs,
	.always_on = %d,
	.compatible = "%s",
}; 
""" % (always_on, str_compatible))


def gen_dtb_c_file(input_file, output_file):
	dtb = FdtBlobParse(input_file)
	fdt = dtb.to_fdt()
	fdt_json = fdt.to_json()
	fdt_data = json.loads(fdt_json)

	output_file.write("""
#ifndef __GENERATED_GEN_DTB_H_
#define __GENERATED_GEN_DTB_H_

#include <sel4m/types.h>
#include <sel4m/init.h>

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
""")

	machine = ""
	for location in fdt_data:
		if location == "model":
			machine = fdt_data["model"][1]
			break
		elif location == "compatible":
			machine = fdt_data[location][1]
			break
		else:
			machine = "Unknow"

	output_file.write("""
static const char machine_name[] __initconst = "%s";
""" % machine)

	fdt_check_address_cell(fdt_data)
	fdt_check_size_cell(fdt_data)

	fdt_scan_initrd(fdt_data, output_file)

	fdt_scan_memory(fdt_data, output_file)

	fdt_scan_reserverd_memory(fdt_data, fdt, output_file)

	fdt_scan_stdout_path(fdt_data, output_file)

	fdt_scan_psci(fdt_data, output_file)

	fdt_scan_cpus(fdt_data, output_file)

	fdt_scan_interrupt(fdt_data, output_file)

	fdt_scan_timer(fdt_data, output_file)

	output_file.write("""
#endif /* !__GENERATED_GEN_DTB_H_ */
""")

	return 0


if __name__ == '__main__':
	parser = argparse.ArgumentParser(
		description=__doc__,
		formatter_class=argparse.RawDescriptionHelpFormatter)

	parser.add_argument(
		"-i",
		"--input",
		required=True,
		help="Input object file")
	parser.add_argument(
		"-o",
		"--output",
		required=True,
		help="Output object file")

	args = parser.parse_args()

	input_file = open(args.input, 'rb')
	output_file = open(args.output, 'w')

	ret = gen_dtb_c_file(input_file, output_file)

	sys.exit(ret)
