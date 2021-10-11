#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

# Checks the existence of an argument to cpio -o.
# flag refers to a variable in the parent scope that contains the argument, if
# the argument isn't supported then the flag is set to the empty string in the parent scope.
function(CheckCPIOArgument var flag)
    if(NOT (DEFINED ${var}))
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cpio-testfile "Testfile contents")
        execute_process(
            COMMAND bash -c "echo cpio-testfile | cpio ${flag} -o"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            OUTPUT_QUIET ERROR_QUIET
            RESULT_VARIABLE result
        )
        if(result)
            set(${var} "" CACHE INTERNAL "")
            message(STATUS "CPIO test ${var} FAILED")
        else()
            set(${var} "${flag}" CACHE INTERNAL "")
            message(STATUS "CPIO test ${var} PASSED")
        endif()
        file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/cpio-testfile)
    endif()
endfunction()

# Function for declaring rules to build a cpio archive that can be linked
# into another target
function(MakeCPIO commands output_name)
    string(REGEX REPLACE ".+/(.+)\\..*" "\\1" basename ${output_name})
    # Check that the reproducible flag is available. Don't use it if it isn't.
    CheckCPIOArgument(cpio_reproducible_flag "--reproducible")
    set(append "")
    set(
        commands_l
        "bash;-c;cpio ${cpio_reproducible_flag} --quiet --create -H newc --file=${output_name}.cpio &;&&"
    )
    foreach(file ${ARGN})
        # Try and generate reproducible cpio meta-data as we do this:
        # - touch -d @0 file sets the modified time to 0
        # - --owner=root:root sets user and group values to 0:0
        # - --reproducible creates reproducible archives with consistent inodes and device numbering
        list(
            APPEND
            commands_l
                "bash;-c;cd `dirname ${file}` && mkdir -p temp_${basename} && cd temp_${basename} && cp -a ${file} . && touch -d @0 `basename ${file}` && echo `basename ${file}` | cpio --append ${cpio_reproducible_flag} --owner=root:root --quiet -o -H newc --file=${output_name}.cpio && rm `basename ${file}` && cd ../ && rmdir temp_${basename};&&"
        )
    endforeach()
    list(APPEND commands_l "true")
    set(${commands} ${commands_l} PARENT_SCOPE)
endfunction()
