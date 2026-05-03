add_library(arc_example_filesystem_modules OBJECT
    nodes/impl/path_ops.cpp
    nodes/impl/memory_storage.cpp
    nodes/impl/disk_storage.cpp
    nodes/impl/console_output.cpp
)

target_link_libraries(arc_example_filesystem_modules PUBLIC arc::module)

target_sources(arc_example_filesystem_modules
    PUBLIC FILE_SET CXX_MODULES FILES
        graphs.ixx
        types.ixx
        nodes/path_ops.ixx
        nodes/memory_storage.ixx
        nodes/disk_storage.ixx
        nodes/filesystem.ixx
        nodes/command_handler.ixx
        nodes/console_output.ixx
        nodes/terminal_line_reader.ixx
        nodes/repl.ixx
        nodes/impl/memory_storage.load.ixx
        nodes/impl/filesystem.impl.ixx
        nodes/impl/command_handler.impl.ixx
        nodes/impl/terminal_line_reader.impl.ixx
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

if(ARC_BUILD_LTO)
    set_property(TARGET arc_example_filesystem_modules PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET arc_example_filesystem_in_memory_repl PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET arc_example_filesystem_disk_repl PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(ARC_BUILD_TESTS)
    add_executable(arc_example_filesystem_tests
        tests/test_cluster.cpp
        tests/test_filesystem.cpp
        tests/test_memory_storage.cpp
        tests/test_disk_storage.cpp
        tests/test_path_ops.cpp
        tests/test_commands.cpp
        tests/test_repl.cpp
        tests/test_tab_completion.cpp
    )

    target_sources(arc_example_filesystem_tests
        PRIVATE FILE_SET CXX_MODULES FILES
            tests/mocks.ixx
    )

    target_generate_arc_src(arc_example_filesystem_tests
        GRAPH_MODULE
            examples.filesystem.tests.mocks
        GRAPH_TYPE
            "arc::test::Graph<examples::filesystem::node::Filesystem, arc::test::Mock<examples::filesystem::tests::MockStorageTypes>>"
        NODES
            node  examples.filesystem.filesystem
    )

    target_generate_arc_src(arc_example_filesystem_tests
        GRAPH_MODULE
            examples.filesystem.tests.mocks
        GRAPH_TYPE
            "arc::test::Graph<examples::filesystem::node::CommandHandler, arc::test::Mock<examples::filesystem::tests::MockFilesystemTypes>>"
        NODES
            node  examples.filesystem.command_handler
    )

    target_generate_arc_src(arc_example_filesystem_tests
        GRAPH_MODULE
            examples.filesystem.tests.mocks
        GRAPH_TYPE
            "arc::test::Graph<examples::filesystem::node::TerminalLineReader, arc::test::Mock<examples::filesystem::tests::MockFilesystemTypes>>"
        NODES
            node  examples.filesystem.terminal_line_reader
    )

    target_generate_arc_src(arc_example_filesystem_tests
        GRAPH_MODULE
            examples.filesystem.repl
        GRAPH_TYPE
            "arc::test::Graph<examples::filesystem::node::Repl>"
        NODES
            node  examples.filesystem.repl
    )

    target_link_libraries(arc_example_filesystem_tests PUBLIC
        arc_example_filesystem_modules
        arc::tests
    )

    add_test(arc_example_filesystem_tests arc_example_filesystem_tests)

    if(ARC_BUILD_LTO)
        set_property(TARGET arc_example_filesystem_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
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
        repl        examples.filesystem.repl
        lineReader  examples.filesystem.terminal_line_reader
        commands    examples.filesystem.command_handler
        fs.fs       examples.filesystem.filesystem
)

target_generate_arc_src(arc_example_filesystem_modules
    GRAPH_MODULE
        examples.filesystem.graphs
    GRAPH_TYPE
        examples::filesystem::graph::DiskRepl
    NODES
        repl        examples.filesystem.repl
        lineReader  examples.filesystem.terminal_line_reader
        commands    examples.filesystem.command_handler
        fs.fs       examples.filesystem.filesystem
)
