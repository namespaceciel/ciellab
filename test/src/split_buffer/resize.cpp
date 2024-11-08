#include <gtest/gtest.h>

#include <ciel/split_buffer.hpp>
#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/int_wrapper.hpp>

using namespace ciel;

namespace {

template<class C>
void
test_resize_impl(::testing::Test*) {
    using T = typename C::value_type;

    split_buffer<T> v(100);

    v.resize(50);
    ASSERT_EQ(v.size(), 50);
    ASSERT_GE(v.capacity(), 100);
    ASSERT_EQ(v, split_buffer<T>(50));

    v.resize(200);
    ASSERT_EQ(v.size(), 200);
    ASSERT_GE(v.capacity(), 200);
    ASSERT_TRUE(std::all_of(v.begin(), v.end(), [](int i) {
        return i == 0;
    }));
}

template<class C>
void
test_resize_value_impl(::testing::Test*) {
    using T = typename C::value_type;

    split_buffer<T> v(100);

    v.resize(50, 1);
    ASSERT_EQ(v.size(), 50);
    ASSERT_GE(v.capacity(), 100);
    ASSERT_EQ(v, split_buffer<T>(50));

    v.resize(200, 1);
    ASSERT_EQ(v.size(), 200);
    ASSERT_GE(v.capacity(), 200);
    ASSERT_TRUE(std::all_of(v.begin(), v.begin() + 50, [](int i) {
        return i == 0;
    }));
    ASSERT_TRUE(std::all_of(v.begin() + 50, v.end(), [](int i) {
        return i == 1;
    }));
}

template<class C>
void
test_resize_self_value_impl(::testing::Test*) {
    using T = typename C::value_type;

    {
        // expansion
        split_buffer<T> v(2, 42);

        v.resize(v.capacity() + 1, v[1]);
        ASSERT_TRUE(std::all_of(v.begin(), v.end(), [](int i) {
            return i == 42;
        }));
    }
    {
        split_buffer<T> v(2, 42);
        v.reserve_back_spare(8);

        v.resize(4, v[1]);
        ASSERT_EQ(v.size(), 4);
        ASSERT_TRUE(std::all_of(v.begin(), v.end(), [](int i) {
            return i == 42;
        }));
    }
}

} // namespace

TEST(split_buffer, resize) {
    test_resize_impl<split_buffer<int>>(this);
    test_resize_impl<split_buffer<Int>>(this);
    test_resize_impl<split_buffer<TRInt>>(this);
    test_resize_impl<split_buffer<TMInt>>(this);

    test_resize_impl<split_buffer<int, fancy_allocator<int>>>(this);
    test_resize_impl<split_buffer<Int, fancy_allocator<Int>>>(this);
    test_resize_impl<split_buffer<TRInt, fancy_allocator<TRInt>>>(this);
    test_resize_impl<split_buffer<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(split_buffer, resize_value) {
    test_resize_value_impl<split_buffer<int>>(this);
    test_resize_value_impl<split_buffer<Int>>(this);
    test_resize_value_impl<split_buffer<TRInt>>(this);
    test_resize_value_impl<split_buffer<TMInt>>(this);

    test_resize_value_impl<split_buffer<int, fancy_allocator<int>>>(this);
    test_resize_value_impl<split_buffer<Int, fancy_allocator<Int>>>(this);
    test_resize_value_impl<split_buffer<TRInt, fancy_allocator<TRInt>>>(this);
    test_resize_value_impl<split_buffer<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(split_buffer, resize_self_value) {
    test_resize_self_value_impl<split_buffer<int>>(this);
    test_resize_self_value_impl<split_buffer<Int>>(this);
    test_resize_self_value_impl<split_buffer<TRInt>>(this);
    test_resize_self_value_impl<split_buffer<TMInt>>(this);

    test_resize_self_value_impl<split_buffer<int, fancy_allocator<int>>>(this);
    test_resize_self_value_impl<split_buffer<Int, fancy_allocator<Int>>>(this);
    test_resize_self_value_impl<split_buffer<TRInt, fancy_allocator<TRInt>>>(this);
    test_resize_self_value_impl<split_buffer<TMInt, fancy_allocator<TMInt>>>(this);
}
