#include <gtest/gtest.h>

#include <ciel/split_buffer.hpp>
#include <ciel/test/int_wrapper.hpp>

using namespace ciel;

TEST(split_buffer, equal) {
    {
        split_buffer<Int> v1{0, 1, 2, 3, 4};
        split_buffer<Int> v2{0, 1, 2, 3, 4};

        ASSERT_EQ(v1, v2);
        ASSERT_LE(v1, v2);
        ASSERT_GE(v1, v2);
    }
}

TEST(split_buffer, not_equal) {
    {
        split_buffer<Int> v1{0, 1, 2, 3, 4};
        split_buffer<Int> v2{0, 1, 2, 3, 5};

        ASSERT_NE(v1, v2);
    }
}

TEST(split_buffer, less) {
    {
        split_buffer<Int> v1{0, 1, 2, 3, 4};
        split_buffer<Int> v2{0, 1, 3, 3, 4};

        ASSERT_LT(v1, v2);
        ASSERT_LE(v1, v2);
        ASSERT_GT(v2, v1);
        ASSERT_GE(v2, v1);
    }
}
