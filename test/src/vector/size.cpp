#include <gtest/gtest.h>

#include <ciel/test/min_allocator.hpp>
#include <ciel/test/safe_allocator.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, size) {
    {
        vector<int> c;
        ASSERT_EQ(c.size(), 0);

        c.push_back(2);
        ASSERT_EQ(c.size(), 1);

        c.push_back(1);
        ASSERT_EQ(c.size(), 2);

        c.push_back(3);
        ASSERT_EQ(c.size(), 3);

        c.erase(c.begin());
        ASSERT_EQ(c.size(), 2);

        c.erase(c.begin());
        ASSERT_EQ(c.size(), 1);

        c.erase(c.begin());
        ASSERT_EQ(c.size(), 0);
    }
    {
        vector<int, min_allocator<int>> c;
        ASSERT_EQ(c.size(), 0);

        c.push_back(2);
        ASSERT_EQ(c.size(), 1);

        c.push_back(1);
        ASSERT_EQ(c.size(), 2);

        c.push_back(3);
        ASSERT_EQ(c.size(), 3);

        c.erase(c.begin());
        ASSERT_EQ(c.size(), 2);

        c.erase(c.begin());
        ASSERT_EQ(c.size(), 1);

        c.erase(c.begin());
        ASSERT_EQ(c.size(), 0);
    }
    {
        vector<int, safe_allocator<int>> c;
        ASSERT_EQ(c.size(), 0);

        c.push_back(2);
        ASSERT_EQ(c.size(), 1);

        c.push_back(1);
        ASSERT_EQ(c.size(), 2);

        c.push_back(3);
        ASSERT_EQ(c.size(), 3);

        c.erase(c.begin());
        ASSERT_EQ(c.size(), 2);

        c.erase(c.begin());
        ASSERT_EQ(c.size(), 1);

        c.erase(c.begin());
        ASSERT_EQ(c.size(), 0);
    }
}
