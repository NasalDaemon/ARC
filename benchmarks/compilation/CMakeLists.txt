# Adds a Release-only test verifying that `main` in the given target fully collapsed
# to `return 0`: no calls and only a handful of instructions left by the optimiser.
# Multi-config generators add the test for all configs; the check script itself
# skips non-Release configurations at test time.
function(add_collapsed_asm_test target)
    if(NOT ARC_BUILD_TESTS)
        return()
    endif()
    get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT _isMultiConfig)
        return()
    endif()
    if(CMAKE_OBJDUMP)
        set(_objdump "${CMAKE_OBJDUMP}")
    else()
        find_program(_objdump NAMES objdump llvm-objdump)
    endif()
    if(NOT _objdump)
        message(STATUS "objdump not found: skipping ${target}_collapsed_asm test")
        return()
    endif()
    add_test(NAME ${target}_collapsed_asm
        COMMAND ${CMAKE_COMMAND}
            -DCONFIG=$<CONFIG>
            -DBINARY=$<TARGET_FILE:${target}>
            -DOBJDUMP=${_objdump}
            -P ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/check_collapsed_main.cmake)
endfunction()

add_subdirectory(9)
add_subdirectory(99)
add_subdirectory(99_seq)
