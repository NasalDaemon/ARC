# Checks that `main` in BINARY has fully collapsed to `return 0`: it returns, and
# at most MAX_INSNS instructions (default 1, the one setting the result) come
# before its first return. A graph that did not fold leaves more work ahead of
# that return, or no return at all (a tail call out of main).
#
# Usage:
#   cmake -DCONFIG=<cfg> -DBINARY=<exe> -DOBJDUMP=<objdump> [-DMAX_INSNS=n]
#         -P check_collapsed_main.cmake
#
# CONFIG matters for multi-config generators only: the check is skipped (passes)
# for non-Release configurations, as only a Release optimiser constant-folds the graph.

if(NOT DEFINED BINARY)
    message(FATAL_ERROR "BINARY not set")
endif()
if(NOT DEFINED OBJDUMP)
    message(FATAL_ERROR "OBJDUMP not set")
endif()
if(NOT DEFINED MAX_INSNS)
    set(MAX_INSNS 1)
endif()

if(DEFINED CONFIG AND NOT CONFIG STREQUAL "" AND NOT CONFIG STREQUAL "Release")
    message(STATUS "Skipping collapsed-assembly check for configuration '${CONFIG}'")
    return()
endif()

execute_process(
    COMMAND "${OBJDUMP}" -d "${BINARY}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE asm
    ERROR_VARIABLE objdump_err
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "objdump failed (${result}) on ${BINARY}: ${objdump_err}")
endif()

string(REPLACE "\r" "" asm "${asm}")
string(REPLACE "\n" ";" lines "${asm}")

# The instruction lines of the <main>: block, up to its first return. Whatever
# objdump lists after that is never reached by a main that did not branch.
set(in_main FALSE)
set(returns FALSE)
set(insns "")
set(landing_pads "")
foreach(line IN LISTS lines)
    if(line MATCHES "^[0-9a-f]+ <main>:")
        set(in_main TRUE)
    elseif(in_main)
        if(line MATCHES "^[ \t]*$")
            break() # blank line: start of the next function
        elseif(line MATCHES "^[ \t]*[0-9a-f]+:[ \t]")
            # Control-flow landing pads (-fcf-protection, branch protection) do no work
            if(line MATCHES "[ \t](endbr64|endbr32|bti)([ \t]|$)")
                list(APPEND landing_pads "${line}")
                continue()
            endif()
            list(APPEND insns "${line}")
            if(line MATCHES "[ \t]ret[a-z]*([ \t]|$)")
                set(returns TRUE)
                break()
            endif()
        endif()
    endif()
endforeach()

if(NOT in_main)
    message(FATAL_ERROR "No <main>: symbol found in ${BINARY}")
endif()

message(STATUS "main() in ${BINARY}:")
foreach(insn IN LISTS landing_pads insns)
    message(STATUS "  ${insn}")
endforeach()

if(NOT returns)
    message(FATAL_ERROR "ARC graph did not collapse: main() never returns (a tail call out of main?)")
endif()

list(LENGTH insns num_insns)
math(EXPR before_ret "${num_insns} - 1")
if(before_ret GREATER MAX_INSNS)
    message(FATAL_ERROR
        "ARC graph did not fully collapse: main() has ${before_ret} instruction(s) before returning (expected <= ${MAX_INSNS})")
endif()

message(STATUS "OK: main() collapsed to ${before_ret} instruction(s) and a return")
