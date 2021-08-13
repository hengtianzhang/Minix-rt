# SPDX-License-Identifier: Apache-2.0

# Configures binary toos as llvm binary tool set

find_program(CMAKE_AR      llvm-ar      ${find_program_clang_args})
find_program(CMAKE_NM      llvm-nm      ${find_program_clang_args})
find_program(CMAKE_OBJDUMP llvm-objdump ${find_program_clang_args})
find_program(CMAKE_RANLIB  llvm-ranlib  ${find_program_clang_args})
find_program(CMAKE_STRIP   llvm-strip   ${find_program_clang_args})
find_program(CMAKE_OBJCOPY llvm-objcopy ${find_program_clang_args})
find_program(CMAKE_READELF llvm-readelf ${find_program_clang_args})

# Use the gnu binutil abstraction
include(${SEL4M_BASE}/cmake/bintools/llvm/target_bintools.cmake)
