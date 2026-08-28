cmake_minimum_required(VERSION 3.10)
project(cpuload VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(src/common)
add_subdirectory(src/server)
add_subdirectory(src/client)
