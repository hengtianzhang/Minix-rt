# SPDX-License-Identifier: Apache-2.0

kernel_ld_options(${LINKERFLAGPREFIX},--no-undefined ${LINKERFLAGPREFIX},-X)

kernel_compile_options(-mgeneral-regs-only)
kernel_compile_options(-fno-asynchronous-unwind-tables)

kernel_cc_option(-mabi=lp64)

if(CONFIG_CPU_BIG_ENDIAN)
kernel_compile_options(-mbig-endian)
target_link_libraries(kernel_interface
	INTERFACE
	${LINKERFLAGPREFIX},-EB
	${LINKERFLAGPREFIX},-maarch64elfb
)
set(UTS_MACHINE "aarch64_be" CACHE PATH "")
else()
kernel_compile_options(-mlittle-endian)
kernel_ld_options(${LINKERFLAGPREFIX},-EL ${LINKERFLAGPREFIX},-maarch64elf)
set(UTS_MACHINE "aarch64" CACHE PATH "")
endif()

add_subdirectory(boot)
add_subdirectory(core)
add_subdirectory(mm)
