#include <gtest/gtest.h>

#include <ciel/test/int_wrapper.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, erase) {
    {
        vector<Int> v{0, 1, 2, 3, 4};

        {
            auto it = v.erase(v.begin());
            ASSERT_EQ(it, v.begin());
            ASSERT_EQ(v, std::initializer_list<Int>({1, 2, 3, 4}));
        }
        {
            auto it = v.erase(v.end() - 1);
            ASSERT_EQ(it, v.end());
            ASSERT_EQ(v, std::initializer_list<Int>({1, 2, 3}));
        }
    }
    {
        vector<Int> v{0, 1, 2, 3, 4};
        auto it = v.erase(v.begin(), v.begin() + 2);
        ASSERT_EQ(it, v.begin());
        ASSERT_EQ(v, std::initializer_list<Int>({2, 3, 4}));
    }
    {
        vector<Int> v{0, 1, 2, 3, 4};
        auto it = v.erase(v.begin(), v.end());
        ASSERT_EQ(it, v.end());
        ASSERT_TRUE(v.empty());
    }
}

TEST(vector, erase_tr) {
    {
        vector<TRInt> v{0, 1, 2, 3, 4};

        {
            auto it = v.erase(v.begin());
            ASSERT_EQ(it, v.begin());
            ASSERT_EQ(v, std::initializer_list<TRInt>({1, 2, 3, 4}));
        }
        {
            auto it = v.erase(v.end() - 1);
            ASSERT_EQ(it, v.end());
            ASSERT_EQ(v, std::initializer_list<TRInt>({1, 2, 3}));
        }
    }
    {
        vector<TRInt> v{0, 1, 2, 3, 4};
        auto it = v.erase(v.begin(), v.begin() + 2);
        ASSERT_EQ(it, v.begin());
        ASSERT_EQ(v, std::initializer_list<TRInt>({2, 3, 4}));
    }
    {
        vector<TRInt> v{0, 1, 2, 3, 4};
        auto it = v.erase(v.begin(), v.end());
        ASSERT_EQ(it, v.end());
        ASSERT_TRUE(v.empty());
    }
}
