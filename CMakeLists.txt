cmake_minimum_required(VERSION 3.5)
project(ciellab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose build type: Debug/Release/RelWithDebInfo/MinSizeRel." FORCE)
endif (NOT CMAKE_BUILD_TYPE)

option(CIELLAB_BUILD_TESTS      "Build unit tests" ${PROJECT_IS_TOP_LEVEL})
option(CIELLAB_BUILD_BENCHMARKS "Build benchmarks" ${PROJECT_IS_TOP_LEVEL})

add_library(ciellab INTERFACE)

target_include_directories(ciellab INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# -Wno-unknown-warning-option: ignore unrecognized compiler options.
# -Wno-int-to-pointer-cast: current split_reference_count atomic_shared_ptr's implementation needs
#   48-bit uintptr_t and 16-bit local_ref_count to be packed in a std::atomic.
#   TODO: change to boost::atomic to support DWCAS.
# -Wno-class-memaccess: memcpy and memmove for trivially_relocatable objects.
# -Wno-assume: suppress false alarms.
target_compile_options(ciellab INTERFACE
    -Wno-unknown-warning-option
    -Wno-int-to-pointer-cast 
    -Wno-class-memaccess
    -Wno-assume
)

if (CIELLAB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif ()

if (CIELLAB_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif ()
