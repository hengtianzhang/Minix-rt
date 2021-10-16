#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

import argparse
import os
import stat
import sys


def gen_file(output_file, name, input_args):
    if input_args == "none":
        str_arg = ""
    else:
        str_arg = "-initrd " + input_args + " "
    output_file.write("""
#!/bin/sh

qemu-system-aarch64 -cpu cortex-a57 -machine type=virt,gic-version=2    \
        -append "rdinit=/linuxrc console=ttyAMA0 earlycon=115200 rodata=nofull" \
        -kernel %s \
        -device virtio-scsi-device  \
        -smp 8  \
        %s  \
        -m 4G   \
        -nographic  \
""" % (name, str_arg))

    return 0


if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument(
        "-o",
        "--output",
        required=True,
        help="Output header file")

    parser.add_argument(
        "-n",
        "--name",
        required=True,
        help="Output define name")

    parser.add_argument(
        "-i",
        "--input",
        required=True,
        help="Input define args")

    args = parser.parse_args()

    output_file = open(args.output, 'w')
    name = args.name
    input_args = args.input
 
    ret = gen_file(output_file, name, input_args)

    sys.exit(ret)
