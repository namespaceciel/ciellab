#include <gtest/gtest.h>

#include <ciel/test/limited_allocator.hpp>
#include <ciel/test/min_allocator.hpp>
#include <ciel/test/safe_allocator.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, reserve) {
    {
        vector<int> v;
        v.reserve(10);
        ASSERT_GE(v.capacity(), 10);
    }
    {
        vector<int> v(100);
        ASSERT_GE(v.capacity(), 100);

        v.reserve(50);
        ASSERT_EQ(v.size(), 100);
        ASSERT_GE(v.capacity(), 100);

        v.reserve(150);
        ASSERT_EQ(v.size(), 100);
        ASSERT_GE(v.capacity(), 150);
    }
    {
        // Add 1 for implementations that dynamically allocate a container proxy.
        vector<int, limited_allocator<int, 250 + 1>> v(100);
        ASSERT_GE(v.capacity(), 100);

        v.reserve(50);
        ASSERT_EQ(v.size(), 100);
        ASSERT_GE(v.capacity(), 100);

        v.reserve(150);
        ASSERT_EQ(v.size(), 100);
        ASSERT_GE(v.capacity(), 150);
    }
    {
        vector<int, min_allocator<int>> v;
        v.reserve(10);
        ASSERT_GE(v.capacity(), 10);
    }
    {
        vector<int, min_allocator<int>> v(100);
        ASSERT_GE(v.capacity(), 100);

        v.reserve(50);
        ASSERT_EQ(v.size(), 100);
        ASSERT_GE(v.capacity(), 100);

        v.reserve(150);
        ASSERT_EQ(v.size(), 100);
        ASSERT_GE(v.capacity(), 150);
    }
    {
        vector<int, safe_allocator<int>> v;
        v.reserve(10);
        ASSERT_GE(v.capacity(), 10);
    }
    {
        vector<int, safe_allocator<int>> v(100);
        ASSERT_GE(v.capacity(), 100);

        v.reserve(50);
        ASSERT_EQ(v.size(), 100);
        ASSERT_GE(v.capacity(), 100);

        v.reserve(150);
        ASSERT_EQ(v.size(), 100);
        ASSERT_GE(v.capacity(), 150);
    }
#ifdef CIEL_HAS_EXCEPTIONS
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

            for (int i = 0; i < 10; ++i) {
                ASSERT_EQ(v[i], 42);
            }
        }
    }
    {
        vector<int, limited_allocator<int, 100>> v;
        v.reserve(50);
        ASSERT_GE(v.capacity(), 50);

        try {
            v.reserve(101);
            ASSERT_TRUE(false);

        } catch (const std::length_error&) {}

        ASSERT_GE(v.capacity(), 50);
    }
#endif
}
