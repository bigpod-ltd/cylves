cmake_minimum_required(VERSION 3.14)

# Test executable
add_executable(test_sylves test_sylves.c)
add_executable(test_core test_core.c)

# Link with the sylves library
target_link_libraries(test_sylves PRIVATE sylves)
target_link_libraries(test_core PRIVATE sylves)

# Add tests
add_test(NAME test_sylves COMMAND test_sylves)
add_test(NAME test_core COMMAND test_core)

# Set working directory for tests
set_tests_properties(test_sylves PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(test_core PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
