cmake_minimum_required(VERSION 3.12)
project(ciellab_test LANGUAGES CXX)

set(CMAKE_BUILD_TYPE "Debug")

set(SOURCES
    src/main.cpp
    src/allocator_traits.cpp
    src/atomic_shared_ptr.cpp
    src/can_be_destroyed_from_base.cpp
    src/compressed_pair.cpp
    src/do_if_noexcept.cpp
    src/exception_safety.cpp
    src/finally.cpp
    src/function.cpp
    src/inplace_vector.cpp
    src/is_nullable.cpp
    src/list.cpp
    src/observer_ptr.cpp
    src/shared_ptr.cpp
    src/split_buffer.cpp
    src/vector.cpp
    src/vector.constructor.cpp
    src/vector.compare.cpp
    src/vector.emplace_back.cpp
    src/vector.exception_safety.cpp
    src/worth_move.cpp
    src/vector/capacity.cpp
    src/vector/empty.cpp
    src/vector/max_size.cpp
)

set(CMAKE_CXX_STANDARD_OPTIONS
    11
    14
    17
    20
    23
)

add_subdirectory(googletest)

function (add_ciellab_test CXX_STANDARD ENABLE_EXCEPTIONS ENABLE_RTTI)

    set(SUB_PROJECT_NAME "${PROJECT_NAME}_${CXX_STANDARD}_exceptions_${ENABLE_EXCEPTIONS}_rtti_${ENABLE_RTTI}")

    add_executable(${SUB_PROJECT_NAME} ${SOURCES})

    if (CXX_STANDARD EQUAL 11)
        target_compile_options(${SUB_PROJECT_NAME} PRIVATE -std=c++11)
    elseif (CXX_STANDARD EQUAL 14)
        target_compile_options(${SUB_PROJECT_NAME} PRIVATE -std=c++14)
    elseif (CXX_STANDARD EQUAL 17)
        target_compile_options(${SUB_PROJECT_NAME} PRIVATE -std=c++17)
    elseif (CXX_STANDARD EQUAL 20)
        target_compile_options(${SUB_PROJECT_NAME} PRIVATE -std=c++20)
    elseif (CXX_STANDARD EQUAL 23)
        target_compile_options(${SUB_PROJECT_NAME} PRIVATE -std=c++23)
    else ()
        message(FATAL_ERROR "CXX_STANDARD ${CXX_STANDARD} is not available.")
    endif ()

    target_include_directories(${SUB_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(${SUB_PROJECT_NAME} PRIVATE ciellab GTest::gtest)

    include(GoogleTest)
    gtest_discover_tests(${SUB_PROJECT_NAME})

    target_compile_options(${SUB_PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)

    if (ENABLE_EXCEPTIONS STREQUAL "on")
        target_compile_options(${SUB_PROJECT_NAME} PRIVATE -fexceptions)
    else ()
        target_compile_options(${SUB_PROJECT_NAME} PRIVATE -fno-exceptions)
    endif ()

    if (ENABLE_RTTI STREQUAL "on")
        target_compile_options(${SUB_PROJECT_NAME} PRIVATE -frtti)
    else ()
        target_compile_options(${SUB_PROJECT_NAME} PRIVATE -fno-rtti)
    endif ()

endfunction ()

foreach (CXX_STANDARD ${CMAKE_CXX_STANDARD_OPTIONS})

    add_ciellab_test(${CXX_STANDARD} on on)
    add_ciellab_test(${CXX_STANDARD} on off)
    add_ciellab_test(${CXX_STANDARD} off on)
    add_ciellab_test(${CXX_STANDARD} off off)

endforeach ()
