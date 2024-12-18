cmake_minimum_required(VERSION 3.12)
project(ciellab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose build type: Debug/Release/RelWithDebInfo/MinSizeRel." FORCE)
endif (NOT CMAKE_BUILD_TYPE)

set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

option(CIELLAB_BUILD_TESTS      "Build unit tests" ${PROJECT_IS_TOP_LEVEL})
option(CIELLAB_BUILD_BENCHMARKS "Build benchmarks" ${PROJECT_IS_TOP_LEVEL})

add_library(ciellab INTERFACE)

target_include_directories(ciellab INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (CIELLAB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif ()

if (CIELLAB_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif ()

add_executable(ciellab_clang_tidy clang_tidy.cpp)
target_link_libraries(ciellab_clang_tidy ciellab)
