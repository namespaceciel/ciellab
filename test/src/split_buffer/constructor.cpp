#include <gtest/gtest.h>

#include <ciel/split_buffer.hpp>
#include <ciel/test/different_allocator.hpp>
#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/sbv_constructor_tests.hpp>

using namespace ciel;

TEST(split_buffer, default_constructor) {
    test_default_constructor_impl<split_buffer<Int>>(this);
    test_default_constructor_impl<split_buffer<Int, fancy_allocator<Int>>>(this);
}

TEST(split_buffer, default_constructor_with_allocator) {
    test_default_constructor_with_allocator_impl<split_buffer<Int>>(this);
    test_default_constructor_with_allocator_impl<split_buffer<Int, fancy_allocator<Int>>>(this);
}

TEST(split_buffer, constructor_size_value) {
    test_constructor_size_value_impl<split_buffer<Int>>(this);
    test_constructor_size_value_impl<split_buffer<Int, fancy_allocator<Int>>>(this);
    {
        // distinguish from iterator range constructor
        split_buffer<size_t> v(size_t{5}, size_t{5});
        ASSERT_EQ(v, std::initializer_list<size_t>({5, 5, 5, 5, 5}));
    }
}

TEST(split_buffer, constructor_size) {
    test_constructor_size_impl<split_buffer<Int>>(this);
    test_constructor_size_impl<split_buffer<Int, fancy_allocator<Int>>>(this);
}

TEST(split_buffer, constructor_iterator_range) {
    test_constructor_iterator_range_impl<split_buffer<Int>, InputIterator<Int>>(this);
    test_constructor_iterator_range_impl<split_buffer<Int>, ForwardIterator<Int>>(this);
    test_constructor_iterator_range_impl<split_buffer<Int>, RandomAccessIterator<Int>>(this);
    test_constructor_iterator_range_impl<split_buffer<Int>, Int*>(this);

    test_constructor_iterator_range_impl<split_buffer<Int, fancy_allocator<Int>>, InputIterator<Int>>(this);
    test_constructor_iterator_range_impl<split_buffer<Int, fancy_allocator<Int>>, ForwardIterator<Int>>(this);
    test_constructor_iterator_range_impl<split_buffer<Int, fancy_allocator<Int>>, RandomAccessIterator<Int>>(this);
    test_constructor_iterator_range_impl<split_buffer<Int, fancy_allocator<Int>>, Int*>(this);
}

TEST(split_buffer, copy_constructor) {
    test_copy_constructor_impl<split_buffer<Int>>(this);
    test_copy_constructor_impl<split_buffer<Int, fancy_allocator<Int>>>(this);
}

TEST(split_buffer, copy_constructor_with_allocator) {
    test_copy_constructor_with_allocator_impl<split_buffer<Int>>(this);
    test_copy_constructor_with_allocator_impl<split_buffer<Int, fancy_allocator<Int>>>(this);
    test_copy_constructor_with_allocator_impl<split_buffer<Int, different_allocator<Int>>>(this);
}

TEST(split_buffer, move_constructor) {
    test_move_constructor_impl<split_buffer<Int>>(this);
    test_move_constructor_impl<split_buffer<Int, fancy_allocator<Int>>>(this);
}

TEST(split_buffer, move_constructor_with_allocator) {
    test_move_constructor_with_allocator_impl<split_buffer<Int>>(this);
    test_move_constructor_with_allocator_impl<split_buffer<Int, fancy_allocator<Int>>>(this);
    {
        // Check if source objects are properly moved.
        using C     = split_buffer<Int, different_allocator<Int>>;
        using T     = typename C::value_type;
        using Alloc = typename C::allocator_type;

        C v1({0, 1, 2, 3, 4});
        C v2(std::move(v1), Alloc{});
        ASSERT_EQ(v2, std::initializer_list<T>({0, 1, 2, 3, 4}));
        ASSERT_EQ(v1, std::initializer_list<T>({-1, -1, -1, -1, -1}));
    }
}

TEST(split_buffer, constructor_initializer_list) {
    test_constructor_initializer_list_impl<split_buffer<Int>>(this);
    test_constructor_initializer_list_impl<split_buffer<Int, fancy_allocator<Int>>>(this);
}
