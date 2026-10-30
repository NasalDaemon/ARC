add_subdirectory(abc)

add_executable(arc_tests_headers
    test_abc_headers.cpp
    test_embedded_headers.cpp
    test_mem_ptr.cpp
    test_nodiscard.cpp
)
add_executable(arc_tests_module
    test_abc_module.cpp
    test_circular_buffer.cpp
    test_collection.cpp
    test_contracts.cpp
    test_const_trait.cpp
    test_default_impl.cpp
    test_embedded_modules.cpp
    test_global.cpp
    test_group.cpp
    test_invoke_method.cpp
    test_joined_trait.cpp
    test_node_count.cpp
    test_null_trait.cpp
    test_datastore/test_datastore.cpp
    test_datastore/test_layout.cpp
    test_datastore/test_lifetime.cpp
    test_datastore/test_refs.cpp
    test_datastore/test_single_listener.cpp
    test_datastore/test_topology.cpp
    test_protocol.cpp
    test_spy.cpp
    test_tracer_spy.cpp
    test_trait_requires.cpp
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
        arc::doctest
    )
    add_test(arc_tests_thread arc_tests_thread)
endif()

if(ARC_BUILD_LTO)
    # Some things can't link with GCC-14+LTO
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "15.1.0")
        set_property(TARGET arc_tests_headers arc_tests_module arc_tests_mock arc_tests_thread
            PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        set_property(TARGET arc_tests_headers
            PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

target_generate_arc_headers(arc_tests_headers
    EMBED
        test_embedded_headers.cpp    arc/tests/test_embedded_headers.hxx
        test_nodiscard.cpp           arc/tests/test_nodiscard.hxx
)

target_generate_arc_modules(arc_tests_module
    GLOB
        test_domain
        test_repeater
        test_trait_requires
    EMBED
        test_datastore/test_datastore.cpp
        test_datastore/test_layout.cpp
        test_datastore/test_lifetime.cpp
        test_datastore/test_refs.cpp
        test_datastore/test_single_listener.cpp
        test_datastore/test_topology.cpp
        test_collection.cpp
        test_contracts.cpp
        test_const_trait.cpp
        test_default_impl.cpp
        test_embedded_modules.cpp
        test_global.cpp
        test_group.cpp
        test_invoke_method.cpp
        test_joined_trait.cpp
        test_node_count.cpp
        test_null_trait.cpp
        test_protocol.cpp
        test_spy.cpp
        test_tracer_spy.cpp
        test_trait_requires.cpp
        test_virtual.cpp
)

target_generate_arc_modules(arc_tests_mock
    EMBED
        test_mock.cpp
)

target_sources(arc_tests_module
    PUBLIC FILE_SET CXX_MODULES FILES
        test_repeater/test_node.ixx
        test_datastore/types.ixx
)

target_generate_arc_src(arc_tests_module
    GRAPH_MODULE abc.graph
    GRAPH_TYPE   arc::Graph<abc::cluster::AliceBob>
    NODES
        alice    abc.alice
)

target_link_libraries(arc_tests_headers PUBLIC
    arc::tests::abc::lib
    arc::headers
    arc::doctest
)
target_link_libraries(arc_tests_module PUBLIC
    arc::tests::abc::module
    arc::module
    arc::doctest
)
target_link_libraries(arc_tests_mock PUBLIC
    arc::module
    arc::doctest
)

add_test(arc_tests_headers arc_tests_headers)
add_test(arc_tests_module arc_tests_module)
add_test(arc_tests_mock arc_tests_mock)

# Discarding a [[nodiscard]] trait method's result fails the compile on each call surface: arc_tests_headers compiles
# test_nodiscard.cpp as it is, and each case below compiles it with one discarded call, building on its own.
set_source_files_properties(test_nodiscard.cpp TARGET_DIRECTORY arc_tests_headers
    PROPERTIES COMPILE_OPTIONS -Werror=unused-result)
arc_import_properties()
foreach(DISCARD_CASE NAMED OVERLOAD EXPLICIT DEFAULT TAG FUNCTOR KEYED DUCK FANOUT REPEATER SELF)
    set(DISCARD_TARGET arc_nodiscard_discard_${DISCARD_CASE})
    add_library(${DISCARD_TARGET} OBJECT EXCLUDE_FROM_ALL test_nodiscard.cpp)
    target_compile_definitions(${DISCARD_TARGET} PRIVATE ARC_DISCARD_${DISCARD_CASE})
    target_compile_options(${DISCARD_TARGET} PRIVATE -Werror=unused-result)
    target_include_directories(${DISCARD_TARGET} PRIVATE ${ARC_GENERATOR_INCLUDE_DIR})
    target_link_libraries(${DISCARD_TARGET} PRIVATE arc::headers arc::doctest)
    add_dependencies(${DISCARD_TARGET} arc_tests_headers_hxx_arc)
    add_test(NAME ${DISCARD_TARGET}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${DISCARD_TARGET} --config $<CONFIG>)
    set_tests_properties(${DISCARD_TARGET} PROPERTIES
        PASS_REGULAR_EXPRESSION "ignoring return value|call answers nothing here")
endforeach()

add_test(NAME arc_generator_test
    COMMAND ${ARC_GENERATOR_PYTHON} ${ARC_GENERATOR_DIR}/generator_test.py
        ${ARC_GENERATOR_PYTHON} ${ARC_GENERATOR_DIR}/generate.py)
