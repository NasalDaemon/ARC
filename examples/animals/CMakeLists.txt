add_executable(arc_example_animals main.cpp)

target_link_libraries(arc_example_animals PUBLIC arc::module)

# Add module interface units
target_sources(arc_example_animals
    PUBLIC FILE_SET CXX_MODULES FILES
        farmer.ixx
        cow.ixx
        sheep.ixx
)

# Generate animal_traits.ixx and farm.ixx from .arc files
target_generate_arc_modules(arc_example_animals)
