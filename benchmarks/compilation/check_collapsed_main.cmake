# Checks that `main` in BINARY has fully collapsed to `return 0`: no function calls,
# no external tail-calls, and at most MAX_INSNS instructions (default 6).
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
    set(MAX_INSNS 6)
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

# Collect the instruction lines of the <main>: function block
set(in_main FALSE)
set(insns "")
foreach(line IN LISTS lines)
    if(line MATCHES "^[0-9a-f]+ <main>:")
        set(in_main TRUE)
    elseif(in_main)
        if(line MATCHES "^[ \t]*$")
            break() # blank line: start of the next function
        elseif(line MATCHES "^[ \t]*[0-9a-f]+:[ \t]")
            list(APPEND insns "${line}")
        endif()
    endif()
endforeach()

if(NOT in_main)
    message(FATAL_ERROR "No <main>: symbol found in ${BINARY}")
endif()

message(STATUS "main() in ${BINARY}:")
foreach(insn IN LISTS insns)
    message(STATUS "  ${insn}")
endforeach()

# Any remaining call means a node boundary survived the optimiser
set(bad "")
foreach(insn IN LISTS insns)
    if(insn MATCHES "[ \t](callq?|blr?)[ \t]") # x86 call/callq, ARM bl/blr
        list(APPEND bad "${insn}")
    elseif(insn MATCHES "[ \t](jmp|b)[ \t].*<([^>]+)>")
        # Unconditional jump to another function is a tail call (graph not folded into main).
        # Jumps into main's own partitions (main.cold, main.isra, ...) are internal.
        if(NOT CMAKE_MATCH_2 MATCHES "^main[.+]")
            list(APPEND bad "${insn}")
        endif()
    endif()
endforeach()

list(LENGTH bad num_bad)
list(LENGTH insns num_insns)

if(num_bad GREATER 0)
    string(REPLACE ";" "\n  " bad_str "${bad}")
    message(FATAL_ERROR
        "ARC graph did not collapse: main() still contains ${num_bad} call(s)/tail-call(s):\n  ${bad_str}")
endif()

if(num_insns GREATER MAX_INSNS)
    message(FATAL_ERROR
        "ARC graph did not fully collapse: main() has ${num_insns} instructions (expected <= ${MAX_INSNS})")
endif()

message(STATUS "OK: main() collapsed to ${num_insns} instruction(s) with no calls")
