add_subdirectory(abc)

find_package(doctest REQUIRED)

add_library(arc_tests test_main.cpp)
add_library(arc::tests ALIAS arc_tests)
target_compile_definitions(arc_tests PUBLIC DOCTEST_CONFIG_SUPER_FAST_ASSERTS=1)

target_link_libraries(arc_tests PUBLIC doctest::doctest)

add_executable(arc_tests_headers
    test_abc_headers.cpp
    test_embedded_headers.cpp
    test_mem_ptr.cpp
)
add_executable(arc_tests_module
    test_abc_module.cpp
    test_circular_buffer.cpp
    test_collection.cpp
    test_embedded_modules.cpp
    test_global.cpp
    test_node_count.cpp
    test_null_trait.cpp
    test_union.cpp
    test_virtual.cpp
    test_repeater/main.cpp
)
add_executable(arc_tests_mock
    test_combine.cpp
    test_lazy.cpp
    test_mock.cpp
)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "15.1.0")
    add_executable(arc_tests_thread
        test_thread/test_collection.cpp
        test_thread/test_thread.cpp
        test_thread/test_union.cpp
    )
    target_generate_arc_modules(arc_tests_thread
        GLOB test_thread
        EMBED
            test_thread/test_collection.cpp
            test_thread/test_union.cpp
    )
    target_sources(arc_tests_thread
        PUBLIC FILE_SET CXX_MODULES FILES
            test_thread/global_scheduler.ixx
            test_thread/poster.ixx
            test_thread/test_node.ixx
        PRIVATE
            test_thread/poster.cpp
    )
    target_link_libraries(arc_tests_thread PUBLIC
        arc::module
        arc::tests
    )
    add_test(arc_tests_thread arc_tests_thread)
endif()

if(ARC_BUILD_LTO)
    set_property(TARGET arc_tests_headers arc_tests_module
        PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

    # Some things can't link with GCC-14+LTO
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "15.1.0")
        set_property(TARGET arc_tests_mock
            PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        set_property(TARGET arc_tests_thread
            PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

target_generate_arc_headers(arc_tests_headers
    EMBED
        test_embedded_headers.cpp
        arc/tests/test_embedded_headers.hxx
)

target_generate_arc_modules(arc_tests_module
    GLOB
        test_domain
        test_repeater
    EMBED
        test_embedded_modules.cpp
        test_global.cpp
        test_collection.cpp
        test_node_count.cpp
        test_null_trait.cpp
        test_virtual.cpp
)

target_generate_arc_modules(arc_tests_mock
    EMBED
        test_mock.cpp
)

target_sources(arc_tests_module
    PUBLIC FILE_SET CXX_MODULES FILES
        test_repeater/test_node.ixx
)

target_generate_arc_src(arc_tests_module
    GRAPH_MODULE abc.graph
    GRAPH_TYPE   arc::Graph<abc::AliceBob>
    NODES
        alice    abc.alice
)

target_link_libraries(arc_tests_headers PUBLIC
    arc::tests::abc::lib
    arc::headers
    arc::tests
)
target_link_libraries(arc_tests_module PUBLIC
    arc::tests::abc::module
    arc::module
    arc::tests
)
target_link_libraries(arc_tests_mock PUBLIC
    arc::module
    arc::tests
)

add_test(arc_tests_headers arc_tests_headers)
add_test(arc_tests_module arc_tests_module)
add_test(arc_tests_mock arc_tests_mock)
