set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0 ")
    if(ARC_TESTS_DEBUG_SAN)
        string(APPEND CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -fsanitize=undefined ")
    endif()
    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "4.3.0")
        # Bug in CMake when compiling std
        string(APPEND CMAKE_CXX_FLAGS "-Wno-reserved-module-identifier ")
    endif()
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -lc++abi ")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0 ")
    if(ARC_TESTS_DEBUG_SAN)
        # -fsanitize=undefined doesn't build with modules
        string(APPEND CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract ")
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "15.1.0")
        # std::stacktrace works well with GCC 15.1+, but requires libstdc++exp to be linked in
        string(APPEND CMAKE_EXE_LINKER_FLAGS " "
            "-lstdc++exp "
        )
        set(ARC_STACKTRACE_ENABLED TRUE)
    endif()

    if(ARC_BUILD_LTO)
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.1.0")
            # Unfortunately, GCC 14 LTO partitioning and modules do not mix well (symbols often missing at link time)
            string(APPEND CMAKE_CXX_FLAGS " "
                "-flto-partition=none "
            )
        else()
            file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gcc-inc-lto")
            string(APPEND CMAKE_CXX_FLAGS " "
                "-flto-partition=cache "
                "-flto-incremental=${CMAKE_CURRENT_BINARY_DIR}/gcc-inc-lto "
            )
        endif()
    endif()
endif()
