cmake_minimum_required(VERSION 3.5)
project(ciellab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CIELLAB_BUILD_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})
option(CIELLAB_BUILD_BENCHMARKS "Build benchmarks" ${PROJECT_IS_TOP_LEVEL})

add_library(ciellab INTERFACE)

target_include_directories(ciellab INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Current split_reference_count atomic_shared_ptr's implementation needs 48-bit uintptr_t and 16-bit local_ref_count
# to be packed in a std::atomic.
# TODO: change to boost::atomic to support DWCAS.
target_compile_options(ciellab INTERFACE -Wno-int-to-pointer-cast)

if (CIELLAB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif ()

if (CIELLAB_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif ()