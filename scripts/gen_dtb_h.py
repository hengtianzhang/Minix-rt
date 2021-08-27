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
from pyfdt.pyfdt import FdtBlobParse
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


def scan_memory_dump(dump_data):
    str_buffer = ""
    for location in dump_data:
        str_buffer += "{ .base = 0x%lx, .size = 0x%lx },"   \
            % (location[0], location[1])

    return str_buffer


def dump_memory_reg(fdt_property, address_cells, size_cells):
    index = 1
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
    reg = [address, size]

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
""" % scan_memory_dump(memory_reg))


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
""" % (scan_stdout_dump(compatile), scan_memory_dump(memory_reg)))


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
};

struct devcie_node {
    char *compatile;
    struct memory_reg reg;
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

    fdt_scan_stdout_path(fdt_data, output_file)

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
