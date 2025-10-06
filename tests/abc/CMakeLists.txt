add_library(arc_tests_abc_lib)
add_library(arc::tests::abc::lib ALIAS arc_tests_abc_lib)

add_library(arc_tests_abc_module_lib)
add_library(arc::tests::abc::module ALIAS arc_tests_abc_module_lib)

if(ARC_BUILD_LTO)
    set_property(TARGET arc_tests_abc_lib arc_tests_abc_module_lib
        PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_libraries(arc_tests_abc_lib PUBLIC arc::headers)
target_link_libraries(arc_tests_abc_module_lib PUBLIC arc::module)

target_include_directories(arc_tests_abc_lib PUBLIC headers/include)
target_generate_arc_headers(arc_tests_abc_lib INCLUDE_DIR headers/include
    # GLOB
    #     abc
    # FILES
    #     abc/graph.hxx.arc
)

target_generate_arc_src(arc_tests_abc_lib
    # PRECOMPILE_HEADERS
    # UNITY
    # COMMON_HEADERS
    #     abc/alice.hpp
    GRAPH_HEADER abc/graph.hxx
    GRAPH_TYPE   arc::Graph<abc::AliceBob>
    NODES
        # alice         , bob         , charlie.charlie
        # abc/alice.tpp , abc/bob.tpp , abc/charlie.tpp
        alice           abc/alice.tpp
        bob             abc/bob.tpp
        charlie.charlie abc/charlie.tpp
)

target_generate_arc_modules(arc_tests_abc_module_lib)

target_sources(arc_tests_abc_lib
    PRIVATE
        headers/src/alice.cpp
        headers/src/ellie.cpp
)

target_sources(arc_tests_abc_module_lib
    PUBLIC FILE_SET CXX_MODULES FILES
        modules/abc/alice.ixx
        modules/abc/alice.impl.ixx
        modules/abc/bob.ixx
        modules/abc/charlie.ixx
        modules/abc/ellie.ixx
        modules/abc/graph.ixx
    PRIVATE
        modules/abc/alice.cpp
        modules/abc/ellie.cpp
)
