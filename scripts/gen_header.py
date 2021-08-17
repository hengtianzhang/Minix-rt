#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

"""
This script scans a specified object file and generates a header file
that defined macros for the offsets of various found structure members
(particularly symbols ending with ``_OFFSET`` or ``_SIZEOF``), primarily
intended for use in assembly code.
"""


import argparse
import sys
import re


def gen_offset_header(input_name, input_file, output_file, name):
    include_guard = name
    output_file.write("""/* THIS FILE IS AUTO GENERATED.  PLEASE DO NOT EDIT.
 *
 * This header file provides macros for the offsets of various structure
 * members.  These offset macros are primarily intended to be used in
 * assembly code.
 */

#ifndef %s
#define %s\n\n""" % (include_guard, include_guard))

    text_line = input_file.readline()
    while text_line:
        m = re.search('->(\w+)(\s+)(\w+).*', str(text_line))
        if m is not None:
            var_name = m.group(1)
            var_value = m.group(3)
            output_file.write(
                "#define %s    0x%x\n" % (var_name, int(var_value)))
        text_line = input_file.readline()

    output_file.write(
        "\n#endif /* !%s*/\n" % (include_guard))

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
        help="Output header file")

    parser.add_argument(
        "-n",
        "--name",
        required=True,
        help="Output define name")

    args = parser.parse_args()

    input_file = open(args.input, 'rb')
    output_file = open(args.output, 'w')
    name = args.name

    ret = gen_offset_header(args.input, input_file, output_file, name)
    sys.exit(ret)
