add_library(arc_example_calculator_modules OBJECT
    src/types.cpp
    src/nodes/impl/tokeniser.cpp
    src/nodes/impl/parser.cpp
    src/nodes/impl/variables.cpp
    src/nodes/impl/formatter.cpp
    src/nodes/impl/history_store.cpp
    src/nodes/impl/batch_line_reader.cpp
    src/nodes/impl/console_output.cpp
)

target_link_libraries(arc_example_calculator_modules PUBLIC arc::module)

target_sources(arc_example_calculator_modules
    PUBLIC FILE_SET CXX_MODULES FILES
        src/types.ixx
        src/nodes/tokeniser.ixx
        src/nodes/parser.ixx
        src/nodes/variables.ixx
        src/nodes/functions.ixx
        src/nodes/impl/functions.impl.ixx
        src/nodes/formatter.ixx
        src/nodes/evaluator.ixx
        src/nodes/impl/evaluator.impl.ixx
        src/nodes/history_store.ixx
        src/nodes/batch_line_reader.ixx
        src/nodes/console_output.ixx
        src/nodes/terminal_line_reader.ixx
        src/nodes/impl/terminal_line_reader.impl.ixx
        src/nodes/command_handler.ixx
        src/nodes/impl/command_handler.impl.ixx
        src/nodes/file_persistence.ixx
        src/nodes/impl/file_persistence.impl.ixx
        src/nodes/repl.ixx
        src/nodes/impl/repl.impl.ixx
        src/graphs.ixx
)

target_generate_arc_modules(arc_example_calculator_modules GLOB src)

add_executable(arc_example_calculator src/main.cpp)

target_link_libraries(arc_example_calculator PUBLIC
    arc_example_calculator_modules
)

if(ARC_BUILD_LTO)
    set_property(TARGET arc_example_calculator_modules PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET arc_example_calculator PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_generate_arc_src(arc_example_calculator_modules
    GRAPH_MODULE
        examples.calculator.graphs
    GRAPH_TYPE
        "examples::calculator::graph::Batch"
    NODES
        compute.evaluator examples.calculator.node.evaluator
        repl              examples.calculator.node.repl
        compute.functions examples.calculator.node.functions
        data.commands     examples.calculator.node.command_handler
        data.persistence  examples.calculator.node.file_persistence
)

target_generate_arc_src(arc_example_calculator_modules
    GRAPH_MODULE
        examples.calculator.graphs
    GRAPH_TYPE
        "examples::calculator::graph::Interactive"
    NODES
        compute.evaluator examples.calculator.node.evaluator
        repl              examples.calculator.node.repl
        compute.functions examples.calculator.node.functions
        lineReader        examples.calculator.node.terminal_line_reader
        data.commands     examples.calculator.node.command_handler
        data.persistence  examples.calculator.node.file_persistence
)

if(ARC_BUILD_TESTS)
    add_executable(arc_example_calculator_tests
        tests/test_tokeniser.cpp
        tests/test_parser.cpp
        tests/test_evaluator.cpp
        tests/test_variables.cpp
        tests/test_functions.cpp
        tests/test_formatter.cpp
        tests/test_history.cpp
        tests/test_batch_line_reader.cpp
        tests/test_persistence.cpp
        tests/test_commands.cpp
        tests/test_cluster.cpp
        tests/test_repl.cpp
    )

    target_sources(arc_example_calculator_tests
        PRIVATE FILE_SET CXX_MODULES FILES
            tests/graphs.ixx
    )

    target_link_libraries(arc_example_calculator_tests PUBLIC
        arc_example_calculator_modules
        arc::doctest
    )

    target_generate_arc_src(arc_example_calculator_tests
        GRAPH_MODULE
            examples.calculator.node.functions
        GRAPH_TYPE
            "arc::test::Graph<examples::calculator::node::Functions>"
        NODES
            node examples.calculator.node.functions
    )

    target_generate_arc_src(arc_example_calculator_tests
        GRAPH_MODULE
            examples.calculator.node.evaluator
        GRAPH_TYPE
            "arc::test::Graph<examples::calculator::node::Evaluator>"
        NODES
            node examples.calculator.node.evaluator
    )

    target_generate_arc_src(arc_example_calculator_tests
        GRAPH_MODULE
            examples.calculator.node.file_persistence
        GRAPH_TYPE
            "arc::test::Graph<examples::calculator::node::FilePersistence>"
        NODES
            node examples.calculator.node.file_persistence
    )

    target_generate_arc_src(arc_example_calculator_tests
        GRAPH_MODULE
            examples.calculator.node.command_handler
        GRAPH_TYPE
            "arc::test::Graph<examples::calculator::node::CommandHandler>"
        NODES
            node examples.calculator.node.command_handler
    )

    target_generate_arc_src(arc_example_calculator_tests
        GRAPH_MODULE
            examples.calculator.tests.graphs
        GRAPH_TYPE
            "examples::calculator::tests::IntegrationGraph"
        NODES
            repl              examples.calculator.node.repl
            compute.evaluator examples.calculator.node.evaluator
            compute.functions examples.calculator.node.functions
            data.commands     examples.calculator.node.command_handler
            data.persistence  examples.calculator.node.file_persistence
    )

    target_generate_arc_src(arc_example_calculator_tests
        GRAPH_MODULE
            examples.calculator.node.repl
        GRAPH_TYPE
            "arc::test::Graph<examples::calculator::node::Repl>"
        NODES
            node examples.calculator.node.repl
    )

    add_test(arc_example_calculator_tests arc_example_calculator_tests)

    if(ARC_BUILD_LTO)
        set_property(TARGET arc_example_calculator_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()
