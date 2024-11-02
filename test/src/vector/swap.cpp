#include <gtest/gtest.h>

#include <ciel/test/min_allocator.hpp>
#include <ciel/test/safe_allocator.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, swap) {
    {
        vector<int> v1(100);
        vector<int> v2(200);

        v1.swap(v2);
        ASSERT_EQ(v1.size(), 200);
        ASSERT_EQ(v1.capacity(), 200);
        ASSERT_EQ(v2.size(), 100);
        ASSERT_EQ(v2.capacity(), 100);
    }
    {
        vector<int, min_allocator<int>> v1(100);
        vector<int, min_allocator<int>> v2(200);

        v1.swap(v2);
        ASSERT_EQ(v1.size(), 200);
        ASSERT_EQ(v1.capacity(), 200);
        ASSERT_EQ(v2.size(), 100);
        ASSERT_EQ(v2.capacity(), 100);
    }
    {
        vector<int, safe_allocator<int>> v1(100);
        vector<int, safe_allocator<int>> v2(200);

        v1.swap(v2);
        ASSERT_EQ(v1.size(), 200);
        ASSERT_EQ(v1.capacity(), 200);
        ASSERT_EQ(v2.size(), 100);
        ASSERT_EQ(v2.capacity(), 100);
    }
}
