#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#

# Error out on error
set -e

rm -f $1/.gen_depend
echo 1 > $1/.gen_depend
