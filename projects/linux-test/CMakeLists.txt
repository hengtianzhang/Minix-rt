# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(sel4m REQUIRED HINTS $ENV{SEL4M_BASE})

project(linux-test)

add_executable(linux-test)
target_sources(linux-test PRIVATE src/main.c)

find_package(libsel4m REQUIRED)
libsel4m_user_import_libraries()
target_link_libraries(linux-test PRIVATE libsel4m)

DeclareRootservices(linux-test)
