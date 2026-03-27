add_library(arc_example_calculator_modules OBJECT
    src/nodes/impl/tokeniser.cpp
    src/nodes/impl/parser.cpp
    src/nodes/impl/variables.cpp
    src/nodes/impl/functions.cpp
    src/nodes/impl/formatter.cpp
)

target_link_libraries(arc_example_calculator_modules PUBLIC arc::module)

target_sources(arc_example_calculator_modules
    PUBLIC FILE_SET CXX_MODULES FILES
        src/types.ixx
        src/nodes/tokeniser.ixx
        src/nodes/parser.ixx
        src/nodes/variables.ixx
        src/nodes/functions.ixx
        src/nodes/formatter.ixx
        src/nodes/evaluator.ixx
        src/nodes/impl/evaluator.impl.ixx
        src/nodes/repl.ixx
        src/nodes/impl/repl.impl.ixx
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
        examples.calculator.clusters
    GRAPH_TYPE
        "arc::Graph<examples::calculator::domain::Calculator>"
    NODES
        evaluator examples.calculator.evaluator
        repl      examples.calculator.repl
)

if(ARC_BUILD_TESTS)
    add_executable(arc_example_calculator_tests
        tests/test_tokeniser.cpp
        tests/test_parser.cpp
        tests/test_evaluator.cpp
        tests/test_variables.cpp
        tests/test_functions.cpp
        tests/test_formatter.cpp
        tests/test_cluster.cpp
    )

    target_link_libraries(arc_example_calculator_tests PUBLIC
        arc_example_calculator_modules
        arc::tests
    )

    target_generate_arc_src(arc_example_calculator_tests
        GRAPH_MODULE
            examples.calculator.clusters
        GRAPH_TYPE
            "arc::test::Graph<examples::calculator::node::Evaluator>"
        NODES
            node examples.calculator.evaluator
    )

    add_test(arc_example_calculator_tests arc_example_calculator_tests)

    if(ARC_BUILD_LTO)
        set_property(TARGET arc_example_calculator_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()
