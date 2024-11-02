#include <gtest/gtest.h>

#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/limited_allocator.hpp>
#include <ciel/test/min_allocator.hpp>
#include <ciel/test/move_only.hpp>
#include <ciel/test/safe_allocator.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, resize_value) {
    {
        vector<int> v(100);

        v.resize(50, 1);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);
        ASSERT_EQ(v, vector<int>(50));

        v.resize(200, 1);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);

        for (int i = 0; i < 50; ++i) {
            ASSERT_EQ(v[i], 0);
        }
        for (int i = 50; i < 200; ++i) {
            ASSERT_EQ(v[i], 1);
        }
    }
    {
        // Add 1 for implementations that dynamically allocate a container proxy.
        vector<int, limited_allocator<int, 300 + 1>> v(100);

        v.resize(50, 1);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);

        v.resize(200, 1);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);
    }
    {
        vector<int, min_allocator<int>> v(100);

        v.resize(50, 1);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);
        const vector<int, min_allocator<int>> temp(50);
        ASSERT_EQ(v, temp);

        v.resize(200, 1);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);

        for (int i = 0; i < 50; ++i) {
            ASSERT_EQ(v[i], 0);
        }
        for (int i = 50; i < 200; ++i) {
            ASSERT_EQ(v[i], 1);
        }
    }
    {
        vector<int, min_allocator<int>> v(100);

        v.resize(50, 1);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);

        v.resize(200, 1);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);
    }
    {
        vector<int, safe_allocator<int>> v(100);

        v.resize(50, 1);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);
        const vector<int, safe_allocator<int>> temp(50);
        ASSERT_EQ(v, temp);

        v.resize(200, 1);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);

        for (int i = 0; i < 50; ++i) {
            ASSERT_EQ(v[i], 0);
        }
        for (int i = 50; i < 200; ++i) {
            ASSERT_EQ(v[i], 1);
        }
    }
    {
        vector<int, safe_allocator<int>> v(100);

        v.resize(50, 1);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);

        v.resize(200, 1);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);
    }
}

TEST(vector, resize_self_value) {
    {
        vector<Int> v(2, 42);

        v.resize(v.capacity() + 1, v[1]);

        for (const auto& i : v) {
            ASSERT_EQ(i, 42);
        }
    }
}

TEST(vector, resize) {
    {
        vector<int> v(100);

        v.resize(50);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);

        v.resize(200);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);
    }
    {
        // Add 1 for implementations that dynamically allocate a container proxy.
        vector<int, limited_allocator<int, 300 + 1>> v(100);

        v.resize(50);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);

        v.resize(200);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);
    }
    {
        vector<MoveOnly> v(100);

        v.resize(50);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);

        v.resize(200);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);
    }
    {
        // Add 1 for implementations that dynamically allocate a container proxy.
        vector<MoveOnly, limited_allocator<MoveOnly, 300 + 1>> v(100);

        v.resize(50);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);

        v.resize(200);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);
    }
    {
        vector<MoveOnly, min_allocator<MoveOnly>> v(100);

        v.resize(50);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);

        v.resize(200);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);
    }
    {
        vector<int, safe_allocator<int>> v(100);
        v.resize(50);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);

        v.resize(200);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);
    }
}
