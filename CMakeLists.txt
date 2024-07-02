cmake_minimum_required(VERSION 3.0)
project(test)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} main_00.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE "$ENV{VULKAN_SDK}/include")