add_executable(arc_bench_compile99_seq main.cpp)
target_link_libraries(arc_bench_compile99_seq arc::module)
target_generate_arc_modules(arc_bench_compile99_seq)

if(ARC_BUILD_TESTS)
    add_test(arc_bench_compile99_seq_test arc_bench_compile99_seq)
endif()

if(ARC_BUILD_LTO)
    set_property(TARGET arc_bench_compile99_seq PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

file(GLOB NODE_IXX_FILES "nodes/*.ixx")
target_sources(arc_bench_compile99_seq
    PUBLIC FILE_SET CXX_MODULES FILES
        ${NODE_IXX_FILES}
)
