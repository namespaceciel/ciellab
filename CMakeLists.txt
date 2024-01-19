cmake_minimum_required(VERSION 3.5)
project(ciellab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ciellab INTERFACE)

target_include_directories(ciellab INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

string(COMPARE EQUAL ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR} PROJECT_IS_TOP_LEVEL)

option(CIELLAB_BUILD_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})
option(CIELLAB_BUILD_BENCHMARKS "Build benchmarks" ${PROJECT_IS_TOP_LEVEL})

if (CIELLAB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif ()

if (CIELLAB_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif ()