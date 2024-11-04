#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, shrink_to_fit) {
    {
        vector<int> v(100);
        v.push_back(1);

        const size_t capacity = v.capacity();
        v.shrink_to_fit();
        ASSERT_LE(v.capacity(), capacity);
        ASSERT_EQ(v.size(), 101);
    }
    {
        vector<int, fancy_allocator<int>> v(100);
        v.push_back(1);

        const size_t capacity = v.capacity();
        v.shrink_to_fit();
        ASSERT_LE(v.capacity(), capacity);
        ASSERT_EQ(v.size(), 101);
    }
}
