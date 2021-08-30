# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(sel4m REQUIRED HINTS $ENV{SEL4M_BASE})

project(hello_world)

add_executable(hello_world)
target_sources(hello_world PRIVATE src/main.c)

find_package(memblock)
memblock_user_import_libraries()

target_link_libraries(hello_world PRIVATE memblock)

DeclareRootservices(hello_world)
