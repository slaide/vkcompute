cmake_minimum_required(VERSION 3.0.0)
project(cppproject VERSION 0.1.0)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

include(CTest)
enable_testing()

add_executable(cppproject_main main.cpp)

target_compile_options(cppproject_main PRIVATE
    -Wall                     # Enable all warnings
    -Wextra                   # Enable extra warnings
    #-O2                       # Optimization level 2
    -std=c++2b                # Use C++23 standard
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
