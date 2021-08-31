# SPDX-License-Identifier: Apache-2.0

list(APPEND crt_files arch/${ARCH}/crt0.S)

add_library(sel4mruntime_crt_obj OBJECT ${crt_files})

set_property(GLOBAL PROPERTY SERVICES_LINKER_SCRIPT_S "${CMAKE_CURRENT_LIST_DIR}/arch/${ARCH}/linker.lds.S")

services_ld_options(-Wl,--no-undefined -Wl,-X)
services_cc_option(-mabi=lp64)

if(CONFIG_CPU_BIG_ENDIAN)
services_compile_definitions(__AARCH64EB__)
services_compile_options(-mbig-endian)
target_link_libraries(services_interface
	INTERFACE
	${LINKERFLAGPREFIX},-EB
	${LINKERFLAGPREFIX},-maarch64elfb
)
else()
services_compile_definitions(__AARCH64EL__)
services_compile_options(-mlittle-endian)
services_ld_options(${LINKERFLAGPREFIX},-EL ${LINKERFLAGPREFIX},-maarch64elf)
endif()

target_link_libraries(sel4mruntime_crt_obj PUBLIC services_interface)
