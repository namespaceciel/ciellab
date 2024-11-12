#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

namespace {

template<class C>
void test_reserve_impl(::testing::Test*) {
    {
        C v;
        v.reserve(10);
        ASSERT_GE(v.capacity(), 10);
    }
    {
        C v(100);
        ASSERT_GE(v.capacity(), 100);

        v.reserve(50);
        ASSERT_EQ(v.size(), 100);
        ASSERT_GE(v.capacity(), 100);

        v.reserve(150);
        ASSERT_EQ(v.size(), 100);
        ASSERT_GE(v.capacity(), 150);
    }
}

template<class C>
void test_reserve_data_validity_impl(::testing::Test*) {
    using T = typename C::value_type;

    vector<T> v{0, 1, 2, 3, 4};
    v.reserve(v.capacity() + 1);
    ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
}

} // namespace

TEST(vector, reserve) {
    test_reserve_impl<vector<int>>(this);
    test_reserve_impl<vector<int, fancy_allocator<int>>>(this);
}

TEST(vector, reserve_data_validity) {
    test_reserve_data_validity_impl<vector<int>>(this);
    test_reserve_data_validity_impl<vector<Int>>(this);
    test_reserve_data_validity_impl<vector<TRInt>>(this);
    test_reserve_data_validity_impl<vector<TMInt>>(this);

    test_reserve_data_validity_impl<vector<int, fancy_allocator<int>>>(this);
    test_reserve_data_validity_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_reserve_data_validity_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_reserve_data_validity_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

#ifdef CIEL_HAS_EXCEPTIONS
TEST(vector, reserve_beyond_max_size) {
    {
        vector<int> v;
        const size_t sz = v.max_size() + 1;

        try {
            v.reserve(sz);
            ASSERT_TRUE(false);

        } catch (const std::length_error&) {
            ASSERT_EQ(v.size(), 0);
            ASSERT_EQ(v.capacity(), 0);
        }
    }
    {
        vector<int> v(10, 42);
        const int* previous_data       = v.data();
        const size_t previous_capacity = v.capacity();
        const size_t sz                = v.max_size() + 1;

        try {
            v.reserve(sz);
            ASSERT_TRUE(false);

        } catch (std::length_error&) {
            ASSERT_EQ(v.size(), 10);
            ASSERT_EQ(v.capacity(), previous_capacity);
            ASSERT_EQ(v.data(), previous_data);
            ASSERT_TRUE(std::all_of(v.begin(), v.end(), [](int i) {
                return i == 42;
            }));
        }
    }
}
#endif
