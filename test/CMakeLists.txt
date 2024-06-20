cmake_minimum_required(VERSION 3.5)
project(ciellab_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE "Debug")

add_executable(ciellab_test
    src/finally_tests.cpp
    src/list_tests.cpp
    src/small_vector_tests.cpp
    src/split_buffer_tests.cpp
    src/vector_tests.cpp
    src/exception_safety_tests.cpp
    src/shared_ptr_tests.cpp
    src/atomic_shared_ptr_tests.cpp
)

target_link_libraries(ciellab_test ciellab)

add_subdirectory(googletest)

target_link_libraries(ciellab_test GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(ciellab_test)

target_compile_options(ciellab_test PUBLIC -fexceptions -frtti -Wall -Wextra -Werror)
