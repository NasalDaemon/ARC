find_package(doctest REQUIRED)

add_library(arc_doctest test_main.cpp)
target_include_directories(arc_doctest PUBLIC include)

add_library(arc::doctest ALIAS arc_doctest)
target_compile_definitions(arc_doctest PUBLIC DOCTEST_CONFIG_SUPER_FAST_ASSERTS=1)

target_link_libraries(arc_doctest PUBLIC doctest::doctest)
