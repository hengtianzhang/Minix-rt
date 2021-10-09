# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(minix_rt REQUIRED HINTS $ENV{MINIX_RT_BASE})

project(linux-test)

add_executable(linux-test)
target_sources(linux-test PRIVATE src/main.c)

find_package(libminix_rt REQUIRED)
libminix_rt_user_import_libraries()
target_link_libraries(linux-test PRIVATE libminix_rt)

DeclareRootservices(linux-test)
