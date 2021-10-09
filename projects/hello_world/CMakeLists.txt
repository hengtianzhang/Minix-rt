# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(minix_rt REQUIRED HINTS $ENV{MINIX_RT_BASE})

project(hello_world)

add_executable(hello_world)
target_sources(hello_world PRIVATE src/main.c)

find_package(libminix_rt REQUIRED)
libminix_rt_user_import_libraries()

target_link_libraries(hello_world PRIVATE libminix_rt)

DeclareRootservices(hello_world)
