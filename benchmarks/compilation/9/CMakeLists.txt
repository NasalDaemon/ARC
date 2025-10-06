add_executable(arc_bench_compile9 main.cpp)
target_link_libraries(arc_bench_compile9 arc::module)
target_generate_arc_modules(arc_bench_compile9)

if(ARC_BUILD_TESTS)
    add_test(arc_bench_compile9_test arc_bench_compile9)
endif()

if(ARC_BUILD_LTO)
    set_property(TARGET arc_bench_compile9 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

file(GLOB NODE_IXX_FILES "nodes/*.ixx")
target_sources(arc_bench_compile9
    PUBLIC FILE_SET CXX_MODULES FILES
        ${NODE_IXX_FILES}
)

target_generate_arc_src(arc_bench_compile9
    GRAPH_MODULE arc.bench.compile9.graph
    GRAPH_TYPE   arc::Graph<arc::bench::compile9::Graph>
    NODES
        node1    arc.bench.compile9.node1
        node2    arc.bench.compile9.node2
        node3    arc.bench.compile9.node3
        node4    arc.bench.compile9.node4
        node5    arc.bench.compile9.node5
        node6    arc.bench.compile9.node6
        node7    arc.bench.compile9.node7
        node8    arc.bench.compile9.node8
        node9    arc.bench.compile9.node9
)
