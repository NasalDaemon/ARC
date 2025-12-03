add_library(arc_example_filesystem_modules OBJECT
    nodes/impl/path_ops.cpp
    nodes/impl/memory_storage.cpp
    nodes/impl/disk_storage.cpp
)

target_link_libraries(arc_example_filesystem_modules PUBLIC arc::module)

target_sources(arc_example_filesystem_modules
    PUBLIC FILE_SET CXX_MODULES FILES
        entry.ixx
        graphs.ixx
        nodes/path_ops.ixx
        nodes/memory_storage.ixx
        nodes/disk_storage.ixx
        nodes/filesystem.ixx
        nodes/repl.ixx
        nodes/impl/memory_storage.load.ixx
        nodes/impl/filesystem.impl.ixx
        nodes/impl/repl.impl.ixx
)

target_generate_arc_modules(arc_example_filesystem_modules)

add_executable(arc_example_filesystem_in_memory_repl in_memory_repl.cpp)
add_executable(arc_example_filesystem_disk_repl disk_repl.cpp)

target_link_libraries(arc_example_filesystem_in_memory_repl PUBLIC
    arc_example_filesystem_modules
)
target_link_libraries(arc_example_filesystem_disk_repl PUBLIC
    arc_example_filesystem_modules
)

if(ARC_BUILD_TESTS)
    add_executable(arc_example_filesystem_tests
        tests/test_cluster.cpp
        tests/test_filesystem.cpp
        tests/test_memory_storage.cpp
        tests/test_disk_storage.cpp
        tests/test_path_ops.cpp
    )

    target_sources(arc_example_filesystem_tests
        PRIVATE FILE_SET CXX_MODULES FILES
            tests/mocks.ixx
    )

    target_generate_arc_src(arc_example_filesystem_tests
        GRAPH_MODULE
            examples.filesystem.tests.mocks
        GRAPH_TYPE
            "arc::test::Graph<examples::filesystem::Filesystem, arc::test::Mock<examples::filesystem::tests::MockStorageTypes>>"
        NODES
            node  examples.filesystem.filesystem
    )

    target_link_libraries(arc_example_filesystem_tests PUBLIC
        arc_example_filesystem_modules
        arc::tests
    )

    add_test(arc_example_filesystem_tests arc_example_filesystem_tests)
endif()

target_generate_arc_src(arc_example_filesystem_modules
    GRAPH_MODULE
        examples.filesystem.graphs
    GRAPH_TYPE
        examples::filesystem::graph::InMemory
    NODES
        fs examples.filesystem.filesystem
)

target_generate_arc_src(arc_example_filesystem_modules
    GRAPH_MODULE
        examples.filesystem.graphs
    GRAPH_TYPE
        examples::filesystem::graph::InMemoryRepl
    NODES
        repl  examples.filesystem.repl
        fs.fs examples.filesystem.filesystem
)

target_generate_arc_src(arc_example_filesystem_modules
    GRAPH_MODULE
        examples.filesystem.graphs
    GRAPH_TYPE
        examples::filesystem::graph::DiskRepl
    NODES
        repl  examples.filesystem.repl
        fs.fs examples.filesystem.filesystem
)
