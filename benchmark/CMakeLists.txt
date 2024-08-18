cmake_minimum_required(VERSION 3.5)
project(ciellab_benchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE "Release")

add_executable(ciellab_benchmark
    src/all.cpp
)

target_link_libraries(ciellab_benchmark ciellab benchmark::benchmark)

add_subdirectory(benchmark)

target_compile_options(ciellab_benchmark PUBLIC -O3 -fexceptions -frtti)
