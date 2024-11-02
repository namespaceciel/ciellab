#include <gtest/gtest.h>

#include <ciel/test/min_allocator.hpp>
#include <ciel/test/safe_allocator.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, empty) {
    {
        vector<int> c;
        ASSERT_TRUE(c.empty());
        c.push_back(1);
        ASSERT_TRUE(!c.empty());
        c.clear();
        ASSERT_TRUE(c.empty());
    }
    {
        vector<int, min_allocator<int>> c;
        ASSERT_TRUE(c.empty());
        c.push_back(1);
        ASSERT_TRUE(!c.empty());
        c.clear();
        ASSERT_TRUE(c.empty());
    }
    {
        vector<int, safe_allocator<int>> c;
        ASSERT_TRUE(c.empty());
        c.push_back(1);
        ASSERT_TRUE(!c.empty());
        c.clear();
        ASSERT_TRUE(c.empty());
    }
}
