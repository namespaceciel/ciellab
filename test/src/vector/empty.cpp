#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, empty) {
    {
        vector<int> c;
        ASSERT_TRUE(c.empty());

        c.push_back(1);
        ASSERT_FALSE(c.empty());

        c.clear();
        ASSERT_TRUE(c.empty());
    }
    {
        vector<int, fancy_allocator<int>> c;
        ASSERT_TRUE(c.empty());

        c.push_back(1);
        ASSERT_FALSE(c.empty());

        c.clear();
        ASSERT_TRUE(c.empty());
    }
}
