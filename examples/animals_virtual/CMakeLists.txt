add_executable(arc_example_animals_virtual main.cpp)

target_link_libraries(arc_example_animals_virtual PUBLIC arc::module)

# Add module interface units
target_sources(arc_example_animals_virtual
    PUBLIC FILE_SET CXX_MODULES FILES
        farmer.ixx
        cow.ixx
        sheep.ixx
        goat.ixx
        fox.ixx
        animal_facade.ixx
        animal.ixx
)

# Generate traits.ixx and farm.ixx from .arc files
target_generate_arc_modules(arc_example_animals_virtual)
