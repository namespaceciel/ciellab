#include <gtest/gtest.h>

#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/min_allocator.hpp>
#include <ciel/vector.hpp>

#include <memory>

using namespace ciel;

TEST(vector, equal) {
    {
        vector<Int> v1{0, 1, 2, 3, 4};
        vector<Int> v2{0, 1, 2, 3, 4};

        ASSERT_EQ(v1, v2);
        ASSERT_LE(v1, v2);
        ASSERT_GE(v1, v2);
    }
    {
        vector<Int, min_allocator<Int>> v1{0, 1, 2, 3, 4};
        vector<Int, min_allocator<Int>> v2{0, 1, 2, 3, 4};

        ASSERT_EQ(v1, v2);
        ASSERT_LE(v1, v2);
        ASSERT_GE(v1, v2);
    }
}

TEST(vector, not_equal) {
    {
        vector<Int> v1{0, 1, 2, 3, 4};
        vector<Int> v2{0, 1, 2, 3, 5};

        ASSERT_NE(v1, v2);
    }
    {
        vector<Int, min_allocator<Int>> v1{0, 1, 2, 3, 4};
        vector<Int, min_allocator<Int>> v2{0, 1, 2, 2, 4};

        ASSERT_NE(v1, v2);
    }
}

TEST(vector, less) {
    {
        vector<Int> v1{0, 1, 2, 3, 4};
        vector<Int> v2{0, 1, 3, 3, 4};

        ASSERT_LT(v1, v2);
        ASSERT_LE(v1, v2);
        ASSERT_GT(v2, v1);
        ASSERT_GE(v2, v1);
    }
    {
        vector<Int, min_allocator<Int>> v1{0, 1, 2, 3, 4};
        vector<Int, min_allocator<Int>> v2{0, 1, 2, 3, 5};

        ASSERT_LT(v1, v2);
        ASSERT_LE(v1, v2);
        ASSERT_GT(v2, v1);
        ASSERT_GE(v2, v1);
    }
}
