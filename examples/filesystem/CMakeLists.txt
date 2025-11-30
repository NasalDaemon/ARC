add_library(arc_example_filesystem_modules OBJECT
    nodes/impl/path_ops.cpp
    nodes/impl/memory_storage.cpp
)

target_link_libraries(arc_example_filesystem_modules PUBLIC arc::module)

target_sources(arc_example_filesystem_modules
    PUBLIC FILE_SET CXX_MODULES FILES
        entry.ixx
        graphs.ixx
        nodes/path_ops.ixx
        nodes/memory_storage.ixx
        nodes/filesystem.ixx
        nodes/impl/memory_storage.load.ixx
        nodes/impl/filesystem.impl.ixx
)

target_generate_arc_modules(arc_example_filesystem_modules)

target_generate_arc_src(arc_example_filesystem_modules
    GRAPH_MODULE
        examples.filesystem.graphs
    GRAPH_TYPE
        examples::filesystem::InMemoryGraph
    NODES
        fs  examples.filesystem.filesystem
)

add_executable(arc_example_filesystem main.cpp)

target_link_libraries(arc_example_filesystem PUBLIC
    arc_example_filesystem_modules
)

if(ARC_BUILD_TESTS)
    add_executable(arc_example_filesystem_tests
        tests/test_domain.cpp
        tests/test_filesystem.cpp
        tests/test_memory_storage.cpp
        tests/test_path_ops.cpp
    )

    target_link_libraries(arc_example_filesystem_tests PUBLIC
        arc_example_filesystem_modules
        arc::tests
    )

    add_test(arc_example_filesystem_tests arc_example_filesystem_tests)
endif()
