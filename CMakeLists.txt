cmake_minimum_required(VERSION 3.0)
project(test)

set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)

#include(CMakePrintHelpers)

add_definitions(-DPROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions(-DDEBUG=1)
endif()

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 REQUIRED FATAL_ERROR)

add_subdirectory(common)
add_subdirectory(00_clear_screen)
add_subdirectory(01_gui)
add_subdirectory(__vsync)