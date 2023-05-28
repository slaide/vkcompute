cmake_minimum_required(VERSION 3.0.0)
project(cppproject VERSION 0.1.0)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

include(CTest)
enable_testing()

find_library(
    vulkan
    NAMES vulkan libvulkan Vulkan
    REQUIRED
)
find_library(
    xcb
    NAMES xcb
    REQUIRED
)

add_executable(
    cppproject

    src/main.cpp
    src/application.cpp
    src/application/window.cpp
    src/application/vulkan_error.cpp
)

target_link_libraries(
    cppproject PRIVATE

    vulkan
    xcb
)

target_compile_options(
    cppproject PRIVATE

    -Wall                     # Enable all warnings
    -Wextra                   # Enable extra warnings
    -O2                       # Optimization level 2

    -DVK_USE_PLATFORM_XCB_KHR
)

target_include_directories(
    cppproject PRIVATE

    include
)
