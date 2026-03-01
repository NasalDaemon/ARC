add_executable(arc_example_animals_virtual main.cpp)

target_link_libraries(arc_example_animals_virtual PUBLIC arc::module)

# Add module interface units
target_sources(arc_example_animals_virtual
    PUBLIC FILE_SET CXX_MODULES FILES
        nodes/animal_facade.ixx
        nodes/cow.ixx
        nodes/farmer.ixx
        nodes/fox.ixx
        nodes/goat.ixx
        nodes/ianimal.ixx
        nodes/sheep.ixx
)

# Generate traits.ixx and farm.ixx from .arc files
target_generate_arc_modules(arc_example_animals_virtual)
