#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

namespace {

template<class C>
void
test_resize_impl(::testing::Test*) {
    using T = typename C::value_type;

    vector<T> v(100);

    v.resize(50);
    ASSERT_EQ(v.size(), 50);
    ASSERT_GE(v.capacity(), 100);
    ASSERT_EQ(v, vector<T>(50));

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

    vector<T> v(100);

    v.resize(50, 1);
    ASSERT_EQ(v.size(), 50);
    ASSERT_GE(v.capacity(), 100);
    ASSERT_EQ(v, vector<T>(50));

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
        vector<T> v(2, 42);

        v.resize(v.capacity() + 1, v[1]);
        ASSERT_TRUE(std::all_of(v.begin(), v.end(), [](int i) {
            return i == 42;
        }));
    }
    {
        vector<T> v(2, 42);
        v.reserve(10);

        v.resize(4, v[1]);
        ASSERT_EQ(v.size(), 4);
        ASSERT_TRUE(std::all_of(v.begin(), v.end(), [](int i) {
            return i == 42;
        }));
    }
}

} // namespace

TEST(vector, resize) {
    test_resize_impl<vector<int>>(this);
    test_resize_impl<vector<Int>>(this);
    test_resize_impl<vector<TRInt>>(this);
    test_resize_impl<vector<TMInt>>(this);

    test_resize_impl<vector<int, fancy_allocator<int>>>(this);
    test_resize_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_resize_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_resize_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, resize_value) {
    test_resize_value_impl<vector<int>>(this);
    test_resize_value_impl<vector<Int>>(this);
    test_resize_value_impl<vector<TRInt>>(this);
    test_resize_value_impl<vector<TMInt>>(this);

    test_resize_value_impl<vector<int, fancy_allocator<int>>>(this);
    test_resize_value_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_resize_value_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_resize_value_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, resize_self_value) {
    test_resize_self_value_impl<vector<int>>(this);
    test_resize_self_value_impl<vector<Int>>(this);
    test_resize_self_value_impl<vector<TRInt>>(this);
    test_resize_self_value_impl<vector<TMInt>>(this);

    test_resize_self_value_impl<vector<int, fancy_allocator<int>>>(this);
    test_resize_self_value_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_resize_self_value_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_resize_self_value_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}
