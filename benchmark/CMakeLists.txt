cmake_minimum_required(VERSION 3.12)
project(ciellab_benchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE "Release")

find_package(absl REQUIRED)
find_package(Folly REQUIRED)
find_package(gflags REQUIRED)

include(FetchContent)
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        b04cec1bf90c3d8e47739bb3271607a18d8b5106
)
FetchContent_MakeAvailable(benchmark)

add_executable(ciellab_benchmark
    src/main.cpp
    src/hashtable.cpp
)

target_link_libraries(ciellab_benchmark
    ciellab
    benchmark::benchmark
    absl::flat_hash_set
    absl::flat_hash_map
    absl::node_hash_set
    absl::node_hash_map
    Folly::folly
)

target_compile_options(ciellab_benchmark PUBLIC -O3 -Wall -Wextra -fexceptions -frtti)
