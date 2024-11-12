#include <gtest/gtest.h>

#include <ciel/test/forward_iterator.hpp>
#include <ciel/test/input_iterator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/random_access_iterator.hpp>

#include <array>

template<class C>
inline void test_default_constructor_impl(::testing::Test*) {
    using Alloc = typename C::allocator_type;
    {
        C c;
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.capacity(), 0);
        ASSERT_EQ(c.get_allocator(), Alloc());
    }
    {
        C c = {};
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.capacity(), 0);
        ASSERT_EQ(c.get_allocator(), Alloc());
    }
}

template<class C>
inline void test_default_constructor_with_allocator_impl(::testing::Test*) {
    using Alloc = typename C::allocator_type;

    C v(Alloc{});
    ASSERT_TRUE(v.empty());
    ASSERT_EQ(v.capacity(), 0);
}

template<class C>
inline void test_constructor_size_value_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v(3, T{1});
    ASSERT_EQ(v, std::initializer_list<T>({1, 1, 1}));
}

template<class C>
inline void test_constructor_size_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v(3);
    ASSERT_EQ(v, std::initializer_list<T>({0, 0, 0}));
}

template<class C, class Iter>
inline void test_constructor_iterator_range_impl(::testing::Test*) {
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
inline void test_copy_constructor_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v1({0, 1, 2, 3, 4});
    C v2(v1); // NOLINT(performance-unnecessary-copy-initialization)
    ASSERT_EQ(v2, std::initializer_list<T>({0, 1, 2, 3, 4}));
}

template<class C>
inline void test_copy_constructor_with_allocator_impl(::testing::Test*) {
    using T     = typename C::value_type;
    using Alloc = typename C::allocator_type;

    C v1({0, 1, 2, 3, 4});
    C v2(v1, Alloc{});
    ASSERT_EQ(v2, std::initializer_list<T>({0, 1, 2, 3, 4}));
}

template<class C>
inline void test_move_constructor_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v1({0, 1, 2, 3, 4});
    C v2(std::move(v1));
    ASSERT_EQ(v2, std::initializer_list<T>({0, 1, 2, 3, 4}));
}

template<class C>
inline void test_move_constructor_with_allocator_impl(::testing::Test*) {
    using T     = typename C::value_type;
    using Alloc = typename C::allocator_type;

    C v1({0, 1, 2, 3, 4});
    C v2(std::move(v1), Alloc{});
    ASSERT_EQ(v2, std::initializer_list<T>({0, 1, 2, 3, 4}));
}

template<class C>
inline void test_constructor_initializer_list_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v({0, 1, 2, 3, 4});
    ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
}
