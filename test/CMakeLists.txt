cmake_minimum_required(VERSION 3.12)
project(ciellab_test LANGUAGES CXX)

set(CMAKE_BUILD_TYPE "Debug")

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        58d77fa8070e8cec2dc1ed015d66b454c8d78850 # release-1.12.1, last version supporting C++11
)
FetchContent_MakeAvailable(googletest)

set(SOURCES
    src/main.cpp
    src/allocator_traits.cpp
    src/atomic_shared_ptr.cpp
    src/avl_raw.cpp
    src/can_be_destroyed_from_base.cpp
    src/compressed_pair.cpp
    src/do_if_noexcept.cpp
    src/finally.cpp
    src/function.cpp
    src/function/constructor.cpp
    src/function/overload_resolution.cpp
    src/hazard_pointer.cpp
    src/is_nullable.cpp
    src/list.cpp
    src/message.cpp
    src/mpsc_queue.cpp
    src/observer_ptr.cpp
    src/reference_counter.cpp
    src/shared_ptr.cpp
    src/spinlock_ptr.cpp
    src/swap.cpp
    src/treiber_stack.cpp
    src/with_lock.cpp
    src/worth_move.cpp
    src/inplace_vector/assign.cpp
    src/inplace_vector/constructor.cpp
    src/inplace_vector/emplace.cpp
    src/inplace_vector/erase.cpp
    src/inplace_vector/insert.cpp
    src/inplace_vector/trait.cpp
    src/inplace_vector/try_append_range.cpp
    src/vector/assign.cpp
    src/vector/capacity.cpp
    src/vector/compare.cpp
    src/vector/constructor.cpp
    src/vector/data.cpp
    src/vector/emplace_back.cpp
    src/vector/emplace.cpp
    src/vector/empty.cpp
    src/vector/erase.cpp
    src/vector/exception_safety.cpp
    src/vector/insert.cpp
    src/vector/max_size.cpp
    src/vector/print.cpp
    src/vector/reserve.cpp
    src/vector/resize.cpp
    src/vector/shrink_to_fit.cpp
    src/vector/size.cpp
    src/vector/swap.cpp
)

set(CMAKE_CXX_STANDARD_OPTIONS
    11
    14
    17
    20
)

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
    else ()
        message(FATAL_ERROR "CXX_STANDARD ${CXX_STANDARD} is not available.")
    endif ()

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
