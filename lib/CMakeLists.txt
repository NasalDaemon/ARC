add_library(arc_lib INTERFACE)

add_library(arc_compiler_flags INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(arc_compiler_flags INTERFACE "-Werror" "-Wall" "-Wextra" "-Wpedantic" "-ferror-limit=1")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(arc_compiler_flags INTERFACE
        "-Werror" "-Wall" "-Wextra" "-Wpedantic"
        "-fdiagnostics-all-candidates" "-fconcepts-diagnostics-depth=5"
    )
endif()

# arc_lib_compiled adds non-template definitions for arc::headers consumers only;
# arc::module consumers link arc_lib (INTERFACE) and get the module partition instead.
add_library(arc_lib_compiled)
add_library(arc::headers ALIAS arc_lib_compiled)

add_library(arc_module_lib)
add_library(arc::module ALIAS arc_module_lib)

target_compile_features(arc_lib INTERFACE cxx_std_23)
target_compile_features(arc_module_lib PUBLIC cxx_std_23)

target_include_directories(arc_lib INTERFACE include macros/include)
target_link_libraries(arc_lib INTERFACE arc_compiler_flags)

target_link_libraries(arc_lib_compiled PUBLIC arc_lib)
target_include_directories(arc_lib_compiled PRIVATE src/include)
target_sources(arc_lib_compiled PRIVATE src/tracer_spy.cpp)

# Do not expose the underlying arc header library from the module lib (apart from macros)
target_link_libraries(arc_module_lib PRIVATE arc_lib)
target_link_libraries(arc_module_lib PUBLIC arc_compiler_flags)
target_include_directories(arc_module_lib PUBLIC  macros/include)
target_include_directories(arc_module_lib PRIVATE src/include)

target_generate_arc_headers(arc_lib INCLUDE_DIR include)

target_sources(arc_module_lib
    PUBLIC FILE_SET CXX_MODULES FILES
        modules/arc.ixx
)

if(ARC_COMPRESS_TYPES)
    target_compile_definitions(arc_module_lib PRIVATE   ARC_COMPRESS_TYPES=1)
    target_compile_definitions(arc_lib        INTERFACE ARC_COMPRESS_TYPES=1)
endif()

if(ARC_STACKTRACE_ENABLED)
    target_compile_definitions(arc_module_lib PRIVATE   ARC_STACKTRACE_ENABLED=1)
    target_compile_definitions(arc_lib        INTERFACE ARC_STACKTRACE_ENABLED=1)
endif()

if(CMAKE_CXX_MODULE_STD)
    target_sources(arc_module_lib PRIVATE modules/tracer_spy.cpp)
    target_compile_definitions(arc_module_lib PUBLIC ARC_IMPORT_STD=1)
endif()
