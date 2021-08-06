# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(sel4m REQUIRED HINTS $ENV{SEL4M_BASE})

project(hello_world)
