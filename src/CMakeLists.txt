cmake_minimum_required(VERSION 3.14)
project(sylves VERSION 0.1.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O2)
    endif()
elseif(MSVC)
    add_compile_options(/W4)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od)
    else()
        add_compile_options(/O2)
    endif()
endif()

# Collect source files
file(GLOB_RECURSE SYLVES_SOURCES src/*.c)

# Create static library
add_library(sylves STATIC ${SYLVES_SOURCES})

# Set include directories
target_include_directories(sylves
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/internal
)

# Link math library if needed
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    target_link_libraries(sylves PUBLIC ${MATH_LIBRARY})
endif()

# Installation rules
install(TARGETS sylves
    EXPORT sylvesTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install public headers
install(DIRECTORY src/include/sylves 
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Export targets
install(EXPORT sylvesTargets
    FILE sylvesTargets.cmake
    NAMESPACE sylves::
    DESTINATION lib/cmake/sylves
)

# Create and install package config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sylvesConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/sylvesConfig.cmake"
    INSTALL_DESTINATION lib/cmake/sylves
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/sylvesConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/sylvesConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/sylvesConfigVersion.cmake"
    DESTINATION lib/cmake/sylves
)

# Option to build tests
option(BUILD_TESTS "Build test programs" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
