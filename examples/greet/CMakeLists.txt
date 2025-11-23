add_executable(arc_example_greet main.cpp)

target_link_libraries(arc_example_greet PUBLIC arc::module)

# Add module interface units
target_sources(arc_example_greet
    PUBLIC FILE_SET CXX_MODULES FILES
        alice.ixx
        bob.ixx
)

# Generate traits.ixx and forum.ixx from .arc files
target_generate_arc_modules(arc_example_greet)
