# SPDX-License-Identifier: Apache-2.0

kernel_ld_options(-Wl,--no-undefined -Wl,-X)

kernel_compile_options(-mgeneral-regs-only)
kernel_compile_options(-fno-asynchronous-unwind-tables)

kernel_cc_option(-mabi=lp64)

if(CONFIG_CPU_BIG_ENDIAN)
kernel_compile_options(-mbig-endian)
kernel_ld_options(-EB -maarch64elfb)
set(UTS_MACHINE "aarch64_be" CACHE PATH "")
else()
kernel_compile_options(-mlittle-endian)
kernel_ld_options(-EL -maarch64elf)
set(UTS_MACHINE "aarch64" CACHE PATH "")
endif()

kernel_ld_option_ifdef(CONFIG_LD_IS_LLD -z norelro)

add_subdirectory(core)
add_subdirectory(boot)
