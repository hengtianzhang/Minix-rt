#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#

# Error out on error
set -e

if [ -r $1/.version ]; then
	VERSION=$(expr 0$(cat $1/.version) + 1)
	echo $VERSION > $1/.version
else
	rm -f $1/.version
	echo 1 > $1/.version
fi;
