#include <gtest/gtest.h>

#include <ciel/test/limited_allocator.hpp>
#include <ciel/test/min_allocator.hpp>
#include <ciel/test/safe_allocator.hpp>
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
        vector<int, limited_allocator<int, 401>> v(100);
        v.push_back(1);

        const size_t capacity = v.capacity();
        v.shrink_to_fit();
        ASSERT_LE(v.capacity(), capacity);
        ASSERT_EQ(v.size(), 101);
    }
    {
        vector<int, min_allocator<int>> v(100);
        v.push_back(1);

        const size_t capacity = v.capacity();
        v.shrink_to_fit();
        ASSERT_LE(v.capacity(), capacity);
        ASSERT_EQ(v.size(), 101);
    }
    {
        vector<int, safe_allocator<int>> v(100);
        v.push_back(1);

        const size_t capacity = v.capacity();
        v.shrink_to_fit();
        ASSERT_LE(v.capacity(), capacity);
        ASSERT_EQ(v.size(), 101);
    }
#ifdef CIEL_HAS_EXCEPTIONS
    {
        vector<int, limited_allocator<int, 400>> v(100);
        v.push_back(1);

        const size_t capacity = v.capacity();
        v.shrink_to_fit();
        ASSERT_LE(v.capacity(), capacity);
        ASSERT_EQ(v.size(), 101);
    }
#endif
}
