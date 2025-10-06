add_library(arc_lib INTERFACE)
add_library(arc::headers ALIAS arc_lib)

add_library(arc_module_lib)
add_library(arc::module ALIAS arc_module_lib)

target_compile_features(arc_lib INTERFACE cxx_std_23)
target_compile_features(arc_module_lib PUBLIC cxx_std_23)

target_include_directories(arc_lib INTERFACE include macros/include)

# Do not expose the underlying arc header library from the module lib (apart from macros)
target_link_libraries(arc_module_lib PRIVATE arc_lib)
target_include_directories(arc_module_lib PUBLIC macros/include)

target_generate_arc_headers(arc_lib INCLUDE_DIR include)

target_sources(arc_module_lib
    PUBLIC FILE_SET CXX_MODULES FILES
        modules/arc.ixx
)

if(ARC_COMPRESS_TYPES)
    target_compile_definitions(arc_module_lib PRIVATE   ARC_COMPRESS_TYPES=1)
    target_compile_definitions(arc_lib        INTERFACE ARC_COMPRESS_TYPES=1)
endif()

if(CMAKE_CXX_MODULE_STD)
    target_compile_definitions(arc_module_lib PUBLIC ARC_IMPORT_STD=1)
endif()
