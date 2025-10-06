set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0 ")
    if(ARC_TESTS_DEBUG_SAN)
        string(APPEND CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -fsanitize=undefined ")
    endif()
    string(APPEND CMAKE_CXX_FLAGS " -Werror -Wall -Wextra -Wpedantic -ferror-limit=1 ")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -lc++abi ")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0 ")
    if(ARC_TESTS_DEBUG_SAN)
        # -fsanitize=undefined doesn't build with modules
        string(APPEND CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract ")
    endif()
    string(APPEND CMAKE_CXX_FLAGS " "
        "-Werror -Wall -Wextra -Wpedantic "
        "-fdiagnostics-all-candidates -fconcepts-diagnostics-depth=5 "
    )

    if(ARC_BUILD_LTO)
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.1.0")
            # Unfortunately, GCC 14 LTO partitioning and modules do not mix well (symbols often missing at link time)
            string(APPEND CMAKE_CXX_FLAGS
                "-flto-partition=none "
            )
        else()
            file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gcc-inc-lto")
            string(APPEND CMAKE_CXX_FLAGS
                "-flto-partition=cache "
                "-flto-incremental=${CMAKE_CURRENT_BINARY_DIR}/gcc-inc-lto "
            )
        endif()
    endif()
endif()
