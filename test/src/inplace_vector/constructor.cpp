#include <gtest/gtest.h>

#include <ciel/core/is_trivially_relocatable.hpp>
#include <ciel/inplace_vector.hpp>
#include <ciel/test/forward_iterator.hpp>
#include <ciel/test/input_iterator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/random_access_iterator.hpp>

#include <array>
#include <cstddef>
#include <initializer_list>
#include <type_traits>

using namespace ciel;

namespace {

template<class C>
void test_default_constructor_impl(::testing::Test*) {
    {
        C c;
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.capacity(), 8);
    }
    {
        C c = {};
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.capacity(), 8);
    }
}

template<class C>
void test_constructor_size_value_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v(3, T{1});
    ASSERT_EQ(v, std::initializer_list<T>({1, 1, 1}));
}

template<class C>
void test_constructor_size_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v(3);
    ASSERT_EQ(v, std::initializer_list<T>({0, 0, 0}));
}

template<class C, class Iter>
void test_constructor_iterator_range_impl(::testing::Test*) {
    using T = typename C::value_type;

    {
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        C v(Iter{arr.data()}, Iter{arr.data() + arr.size()});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
    }
    {
        // empty range
        C v(Iter{nullptr}, Iter{nullptr});
        ASSERT_TRUE(v.empty());
    }
}

template<class C>
void test_copy_constructor_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v1({0, 1, 2, 3, 4});
    C v2(v1); // NOLINT(performance-unnecessary-copy-initialization)
    ASSERT_EQ(v2, std::initializer_list<T>({0, 1, 2, 3, 4}));
}

template<class C>
void test_move_constructor_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v1({0, 1, 2, 3, 4});
    C v2(std::move(v1));
    ASSERT_EQ(v2, std::initializer_list<T>({0, 1, 2, 3, 4}));

    if (std::is_trivially_copyable<T>::value) {
        ASSERT_EQ(v1, v2);                                             // NOLINT(bugprone-use-after-move)
    } else if (is_trivially_relocatable<T>::value) {
        ASSERT_TRUE(v1.empty());                                       // NOLINT(bugprone-use-after-move)
    } else {
        ASSERT_EQ(v1, std::initializer_list<T>({-1, -1, -1, -1, -1})); // NOLINT(bugprone-use-after-move)
    }
}

template<class C>
void test_constructor_initializer_list_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v({0, 1, 2, 3, 4});
    ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
}

} // namespace

TEST(inplace_vector, default_constructor) {
    test_default_constructor_impl<inplace_vector<Int, 8>>(this);
}

TEST(inplace_vector, constructor_size_value) {
    test_constructor_size_value_impl<inplace_vector<Int, 8>>(this);
    {
        // distinguish from iterator range constructor
        const inplace_vector<size_t, 8> v(size_t{5}, size_t{5});
        ASSERT_EQ(v, std::initializer_list<size_t>({5, 5, 5, 5, 5}));
    }
}

TEST(inplace_vector, constructor_size) {
    test_constructor_size_impl<inplace_vector<Int, 8>>(this);
}

TEST(inplace_vector, constructor_iterator_range) {
    test_constructor_iterator_range_impl<inplace_vector<Int, 8>, InputIterator<Int>>(this);
    test_constructor_iterator_range_impl<inplace_vector<Int, 8>, ForwardIterator<Int>>(this);
    test_constructor_iterator_range_impl<inplace_vector<Int, 8>, RandomAccessIterator<Int>>(this);
    test_constructor_iterator_range_impl<inplace_vector<Int, 8>, Int*>(this);
}

TEST(inplace_vector, copy_constructor) {
    test_copy_constructor_impl<inplace_vector<Int, 8>>(this);
}

TEST(inplace_vector, move_constructor) {
    test_move_constructor_impl<inplace_vector<int, 8>>(this);
    test_move_constructor_impl<inplace_vector<Int, 8>>(this);
    test_move_constructor_impl<inplace_vector<TRInt, 8>>(this);
    test_move_constructor_impl<inplace_vector<TMInt, 8>>(this);
}

TEST(inplace_vector, constructor_initializer_list) {
    test_constructor_initializer_list_impl<inplace_vector<Int, 8>>(this);
}
