#!/bin/sh -x

echo $1 | $2 -q $3 > $4
