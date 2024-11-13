#include <gtest/gtest.h>

#include <ciel/test/different_allocator.hpp>
#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/forward_iterator.hpp>
#include <ciel/test/input_iterator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/random_access_iterator.hpp>
#include <ciel/test/sbv_constructor_tests.hpp>
#include <ciel/vector.hpp>

#include <cstddef>
#include <initializer_list>
#include <utility>

using namespace ciel;

TEST(vector, default_constructor) {
    test_default_constructor_impl<vector<Int>>(this);
    test_default_constructor_impl<vector<Int, fancy_allocator<Int>>>(this);
}

TEST(vector, default_constructor_with_allocator) {
    test_default_constructor_with_allocator_impl<vector<Int>>(this);
    test_default_constructor_with_allocator_impl<vector<Int, fancy_allocator<Int>>>(this);
}

TEST(vector, constructor_size_value) {
    test_constructor_size_value_impl<vector<Int>>(this);
    test_constructor_size_value_impl<vector<Int, fancy_allocator<Int>>>(this);
    {
        // distinguish from iterator range constructor
        const vector<size_t> v(size_t{5}, size_t{5});
        ASSERT_EQ(v, std::initializer_list<size_t>({5, 5, 5, 5, 5}));
    }
}

TEST(vector, constructor_size) {
    test_constructor_size_impl<vector<Int>>(this);
    test_constructor_size_impl<vector<Int, fancy_allocator<Int>>>(this);
}

TEST(vector, constructor_iterator_range) {
    test_constructor_iterator_range_impl<vector<Int>, InputIterator<Int>>(this);
    test_constructor_iterator_range_impl<vector<Int>, ForwardIterator<Int>>(this);
    test_constructor_iterator_range_impl<vector<Int>, RandomAccessIterator<Int>>(this);
    test_constructor_iterator_range_impl<vector<Int>, Int*>(this);

    test_constructor_iterator_range_impl<vector<Int, fancy_allocator<Int>>, InputIterator<Int>>(this);
    test_constructor_iterator_range_impl<vector<Int, fancy_allocator<Int>>, ForwardIterator<Int>>(this);
    test_constructor_iterator_range_impl<vector<Int, fancy_allocator<Int>>, RandomAccessIterator<Int>>(this);
    test_constructor_iterator_range_impl<vector<Int, fancy_allocator<Int>>, Int*>(this);
}

TEST(vector, copy_constructor) {
    test_copy_constructor_impl<vector<Int>>(this);
    test_copy_constructor_impl<vector<Int, fancy_allocator<Int>>>(this);
}

TEST(vector, copy_constructor_with_allocator) {
    test_copy_constructor_with_allocator_impl<vector<Int>>(this);
    test_copy_constructor_with_allocator_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_copy_constructor_with_allocator_impl<vector<Int, different_allocator<Int>>>(this);
}

TEST(vector, move_constructor) {
    test_move_constructor_impl<vector<Int>>(this);
    test_move_constructor_impl<vector<Int, fancy_allocator<Int>>>(this);
}

TEST(vector, move_constructor_with_allocator) {
    test_move_constructor_with_allocator_impl<vector<Int>>(this);
    test_move_constructor_with_allocator_impl<vector<Int, fancy_allocator<Int>>>(this);
    {
        // Check if source objects are properly moved.
        using C     = vector<Int, different_allocator<Int>>;
        using T     = typename C::value_type;
        using Alloc = typename C::allocator_type;

        C v1({0, 1, 2, 3, 4});
        const C v2(std::move(v1), Alloc{});
        ASSERT_EQ(v2, std::initializer_list<T>({0, 1, 2, 3, 4}));
        ASSERT_EQ(v1, std::initializer_list<T>({-1, -1, -1, -1, -1})); // NOLINT(bugprone-use-after-move)
    }
}

TEST(vector, constructor_initializer_list) {
    test_constructor_initializer_list_impl<vector<Int>>(this);
    test_constructor_initializer_list_impl<vector<Int, fancy_allocator<Int>>>(this);
}
