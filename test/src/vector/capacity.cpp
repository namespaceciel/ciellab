#include <gtest/gtest.h>

#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, capacity) {
    {
        vector<int> v;
        ASSERT_EQ(v.capacity(), 0);
    }
    {
        vector<int> v(100);
        ASSERT_GE(v.capacity(), 100);

        const auto old_cap = v.capacity();
        v.resize(old_cap);
        v.push_back(0);
        ASSERT_GT(v.capacity(), old_cap);
    }
}
