#include <gtest/gtest.h>

#include <ciel/test/min_allocator.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, capacity) {
    {
        vector<int> v;
        ASSERT_EQ(v.capacity(), 0);
    }
    {
        vector<int> v(100);
        ASSERT_EQ(v.capacity(), 100);

        v.push_back(0);
        ASSERT_GT(v.capacity(), 101);
    }
    {
        vector<int, min_allocator<int>> v;
        ASSERT_EQ(v.capacity(), 0);
    }
    {
        vector<int, min_allocator<int>> v(100);
        ASSERT_EQ(v.capacity(), 100);

        v.push_back(0);
        ASSERT_GT(v.capacity(), 101);
    }
}
