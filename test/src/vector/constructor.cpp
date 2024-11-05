#include <gtest/gtest.h>

#include <ciel/test/different_allocator.hpp>
#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/forward_iterator.hpp>
#include <ciel/test/input_iterator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/random_access_iterator.hpp>
#include <ciel/vector.hpp>

#include <array>

using namespace ciel;

namespace {

template<class C>
void
test_default_constructor_impl(::testing::Test*) {
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
void
test_default_constructor_with_allocator_impl(::testing::Test*) {
    using Alloc = typename C::allocator_type;

    C v(Alloc{});
    ASSERT_TRUE(v.empty());
    ASSERT_EQ(v.capacity(), 0);
}

template<class C>
void
test_constructor_size_value_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v(3, T{1});
    ASSERT_EQ(v, std::initializer_list<T>({1, 1, 1}));
}

template<class C>
void
test_constructor_size_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v(3);
    ASSERT_EQ(v, std::initializer_list<T>({0, 0, 0}));
}

template<class C>
void
test_constructor_iterator_range_impl(::testing::Test*) {
    using T = typename C::value_type;

    // InputIterator
    {
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        C v(InputIterator<T>{arr.data()}, InputIterator<T>{arr.data() + arr.size()});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
    }
    {
        // empty range
        C v(InputIterator<T>{nullptr}, InputIterator<T>{nullptr});
        ASSERT_TRUE(v.empty());
    }
    // ForwardIterator
    {
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        C v(ForwardIterator<T>{arr.data()}, ForwardIterator<T>{arr.data() + arr.size()});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
    }
    {
        // empty range
        C v(ForwardIterator<T>{nullptr}, ForwardIterator<T>{nullptr});
        ASSERT_TRUE(v.empty());
    }
    // random_access_iterator
    {
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        C v(RandomAccessIterator<T>{arr.data()}, RandomAccessIterator<T>{arr.data() + arr.size()});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
    }
    {
        // empty range
        C v(RandomAccessIterator<T>{nullptr}, RandomAccessIterator<T>{nullptr});
        ASSERT_TRUE(v.empty());
    }
    // contiguous_iterator
    {
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        C v(arr.data(), arr.data() + arr.size());
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
    }
    {
        // empty range
        C v(static_cast<T*>(nullptr), static_cast<T*>(nullptr));
        ASSERT_TRUE(v.empty());
    }
}

template<class C>
void
test_copy_constructor_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v1({0, 1, 2, 3, 4});
    C v2(v1);
    ASSERT_EQ(v2, std::initializer_list<T>({0, 1, 2, 3, 4}));
}

template<class C>
void
test_copy_constructor_with_allocator_impl(::testing::Test*) {
    using T     = typename C::value_type;
    using Alloc = typename C::allocator_type;

    C v1({0, 1, 2, 3, 4});
    C v2(v1, Alloc{});
    ASSERT_EQ(v2, std::initializer_list<T>({0, 1, 2, 3, 4}));
}

template<class C>
void
test_move_constructor_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v1({0, 1, 2, 3, 4});
    C v2(std::move(v1));
    ASSERT_EQ(v2, std::initializer_list<T>({0, 1, 2, 3, 4}));
}

template<class C>
void
test_move_constructor_with_allocator_impl(::testing::Test*) {
    using T     = typename C::value_type;
    using Alloc = typename C::allocator_type;

    C v1({0, 1, 2, 3, 4});
    C v2(std::move(v1), Alloc{});
    ASSERT_EQ(v2, std::initializer_list<T>({0, 1, 2, 3, 4}));
}

template<class C>
void
test_constructor_initializer_list_impl(::testing::Test*) {
    using T = typename C::value_type;

    C v({0, 1, 2, 3, 4});
    ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
}

} // namespace

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
        vector<size_t> v(size_t{5}, size_t{5});
        ASSERT_EQ(v, std::initializer_list<size_t>({5, 5, 5, 5, 5}));
    }
}

TEST(vector, constructor_size) {
    test_constructor_size_impl<vector<Int>>(this);
    test_constructor_size_impl<vector<Int, fancy_allocator<Int>>>(this);
}

TEST(vector, constructor_iterator_range) {
    test_constructor_iterator_range_impl<vector<Int>>(this);
    test_constructor_iterator_range_impl<vector<Int, fancy_allocator<Int>>>(this);
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
        C v2(std::move(v1), Alloc{});
        ASSERT_EQ(v2, std::initializer_list<T>({0, 1, 2, 3, 4}));
        ASSERT_EQ(v1, std::initializer_list<T>({-1, -1, -1, -1, -1}));
    }
}

TEST(vector, constructor_initializer_list) {
    test_constructor_initializer_list_impl<vector<Int>>(this);
    test_constructor_initializer_list_impl<vector<Int, fancy_allocator<Int>>>(this);
}
