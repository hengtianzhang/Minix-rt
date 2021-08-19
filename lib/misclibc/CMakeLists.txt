# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(libmisclibc VERSION 0.0.1)

add_library(misclibc INTERFACE)

target_include_directories(misclibc INTERFACE include)
