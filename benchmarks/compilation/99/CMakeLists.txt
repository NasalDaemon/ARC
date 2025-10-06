add_executable(arc_bench_compile99 main.cpp)
target_link_libraries(arc_bench_compile99 arc::module)
target_generate_arc_modules(arc_bench_compile99)

if(ARC_BUILD_TESTS)
    add_test(arc_bench_compile99_test arc_bench_compile99)
endif()

if(ARC_BUILD_LTO)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # flto=thin doesn't collapse benchmark99 into return 0
        target_compile_options(arc_bench_compile99 PRIVATE "-flto=full")
        target_link_options(arc_bench_compile99 PRIVATE "-flto=full")
    else()
        set_property(TARGET arc_bench_compile99 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

file(GLOB NODE_IXX_FILES "nodes/*.ixx")
target_sources(arc_bench_compile99
    PUBLIC FILE_SET CXX_MODULES FILES
        ${NODE_IXX_FILES}
)

target_generate_arc_src(arc_bench_compile99
    GRAPH_MODULE arc.bench.compile99.graph
    GRAPH_TYPE   arc::Graph<arc::bench::compile99::Graph>
    NODES
        node1    arc.bench.compile99.node1
        node2    arc.bench.compile99.node2
        node3    arc.bench.compile99.node3
        node4    arc.bench.compile99.node4
        node5    arc.bench.compile99.node5
        node6    arc.bench.compile99.node6
        node7    arc.bench.compile99.node7
        node8    arc.bench.compile99.node8
        node9    arc.bench.compile99.node9
        node10   arc.bench.compile99.node10
        node11   arc.bench.compile99.node11
        node12   arc.bench.compile99.node12
        node13   arc.bench.compile99.node13
        node14   arc.bench.compile99.node14
        node15   arc.bench.compile99.node15
        node16   arc.bench.compile99.node16
        node17   arc.bench.compile99.node17
        node18   arc.bench.compile99.node18
        node19   arc.bench.compile99.node19
        node20   arc.bench.compile99.node20
        node21   arc.bench.compile99.node21
        node22   arc.bench.compile99.node22
        node23   arc.bench.compile99.node23
        node24   arc.bench.compile99.node24
        node25   arc.bench.compile99.node25
        node26   arc.bench.compile99.node26
        node27   arc.bench.compile99.node27
        node28   arc.bench.compile99.node28
        node29   arc.bench.compile99.node29
        node30   arc.bench.compile99.node30
        node31   arc.bench.compile99.node31
        node32   arc.bench.compile99.node32
        node33   arc.bench.compile99.node33
        node34   arc.bench.compile99.node34
        node35   arc.bench.compile99.node35
        node36   arc.bench.compile99.node36
        node37   arc.bench.compile99.node37
        node38   arc.bench.compile99.node38
        node39   arc.bench.compile99.node39
        node40   arc.bench.compile99.node40
        node41   arc.bench.compile99.node41
        node42   arc.bench.compile99.node42
        node43   arc.bench.compile99.node43
        node44   arc.bench.compile99.node44
        node45   arc.bench.compile99.node45
        node46   arc.bench.compile99.node46
        node47   arc.bench.compile99.node47
        node48   arc.bench.compile99.node48
        node49   arc.bench.compile99.node49
        node50   arc.bench.compile99.node50
        node51   arc.bench.compile99.node51
        node52   arc.bench.compile99.node52
        node53   arc.bench.compile99.node53
        node54   arc.bench.compile99.node54
        node55   arc.bench.compile99.node55
        node56   arc.bench.compile99.node56
        node57   arc.bench.compile99.node57
        node58   arc.bench.compile99.node58
        node59   arc.bench.compile99.node59
        node60   arc.bench.compile99.node60
        node61   arc.bench.compile99.node61
        node62   arc.bench.compile99.node62
        node63   arc.bench.compile99.node63
        node64   arc.bench.compile99.node64
        node65   arc.bench.compile99.node65
        node66   arc.bench.compile99.node66
        node67   arc.bench.compile99.node67
        node68   arc.bench.compile99.node68
        node69   arc.bench.compile99.node69
        node70   arc.bench.compile99.node70
        node71   arc.bench.compile99.node71
        node72   arc.bench.compile99.node72
        node73   arc.bench.compile99.node73
        node74   arc.bench.compile99.node74
        node75   arc.bench.compile99.node75
        node76   arc.bench.compile99.node76
        node77   arc.bench.compile99.node77
        node78   arc.bench.compile99.node78
        node79   arc.bench.compile99.node79
        node80   arc.bench.compile99.node80
        node81   arc.bench.compile99.node81
        node82   arc.bench.compile99.node82
        node83   arc.bench.compile99.node83
        node84   arc.bench.compile99.node84
        node85   arc.bench.compile99.node85
        node86   arc.bench.compile99.node86
        node87   arc.bench.compile99.node87
        node88   arc.bench.compile99.node88
        node89   arc.bench.compile99.node89
        node90   arc.bench.compile99.node90
        node91   arc.bench.compile99.node91
        node92   arc.bench.compile99.node92
        node93   arc.bench.compile99.node93
        node94   arc.bench.compile99.node94
        node95   arc.bench.compile99.node95
        node96   arc.bench.compile99.node96
        node97   arc.bench.compile99.node97
        node98   arc.bench.compile99.node98
        node99   arc.bench.compile99.node99
)
