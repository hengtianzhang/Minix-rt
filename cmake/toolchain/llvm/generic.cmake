# SPDX-License-Identifier: Apache-2.0

set_ifndef(LLVM_TOOLCHAIN_PATH "$ENV{CLANG_ROOT_DIR}")
set_ifndef(LLVM_TOOLCHAIN_PATH "$ENV{LLVM_TOOLCHAIN_PATH}")
if(LLVM_TOOLCHAIN_PATH)
  set(TOOLCHAIN_HOME ${LLVM_TOOLCHAIN_PATH}/bin/)
endif()

set(LLVM_TOOLCHAIN_PATH ${CLANG_ROOT_DIR} CACHE PATH "clang install directory")

set(COMPILER clang)
set(LINKER lld)
set(BINTOOLS llvm)

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")

message(STATUS "Found toolchain: host (clang/ld)")
