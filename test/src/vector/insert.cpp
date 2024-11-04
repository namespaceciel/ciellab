#include <gtest/gtest.h>

#include <ciel/test/int_wrapper.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, insert_self_reference) {
    {
        vector<Int> v{0, 1, 2, 3, 4};
        ASSERT_EQ(v.size(), v.capacity());

        v.insert(v.begin() + 1, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({0, 2, 1, 2, 3, 4}));
    }
    {
        vector<Int> v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({0, 2, 1, 2, 3, 4}));
    }
    {
        vector<Int> v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, std::move(v[2]));
        ASSERT_EQ(v, std::initializer_list<Int>({0, 2, 1, -1, 3, 4}));
    }
    {
        vector<Int> v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, 5, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({0, 2, 2, 2, 2, 2, 1, 2, 3, 4}));
    }
}
