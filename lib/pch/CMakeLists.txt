add_library(arc_pch_lib STATIC blank.cpp)
add_library(arc::pch ALIAS arc_pch_lib)

target_link_libraries(arc_pch_lib PUBLIC arc_lib)

target_precompile_headers(arc_pch_lib PUBLIC
    [["arc/arc.hpp"]]
)
