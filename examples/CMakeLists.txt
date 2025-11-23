if(NOT CMAKE_CXX_MODULE_STD)
    message(FATAL_ERROR "CMake C++ Modules support is required to build the examples. Please enable CMAKE_CXX_MODULE_STD.")
endif()

add_subdirectory(greet)
add_subdirectory(animals)
add_subdirectory(animals_virtual)
