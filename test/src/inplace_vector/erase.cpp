#include <gtest/gtest.h>

#include <ciel/inplace_vector.hpp>
#include <ciel/test/int_wrapper.hpp>

using namespace ciel;

namespace {

template<class T>
void test_erase_impl(::testing::Test*) {
    // erase single
    {
        inplace_vector<T, 16> v{0, 1, 2, 3, 4};
        {
            auto it = v.erase(v.begin());
            ASSERT_EQ(it, v.begin());
            ASSERT_EQ(v, std::initializer_list<T>({1, 2, 3, 4}));
        }
        {
            auto it = v.erase(v.end() - 2);
            ASSERT_EQ(it, v.end() - 1);
            ASSERT_EQ(v, std::initializer_list<T>({1, 2, 4}));
        }
        {
            auto it = v.erase(v.end() - 1);
            ASSERT_EQ(it, v.end());
            ASSERT_EQ(v, std::initializer_list<T>({1, 2}));
        }
    }
    // erase count < pos_end_dis
    {
        inplace_vector<T, 16> v{0, 1, 2, 3, 4};
        auto it = v.erase(v.begin(), v.begin() + 2);
        ASSERT_EQ(it, v.begin());
        ASSERT_EQ(v, std::initializer_list<T>({2, 3, 4}));
    }
    // erase count > pos_end_dis
    {
        inplace_vector<T, 16> v{0, 1, 2, 3, 4};
        auto it = v.erase(v.begin(), v.begin() + 3);
        ASSERT_EQ(it, v.begin());
        ASSERT_EQ(v, std::initializer_list<T>({3, 4}));
    }
    // erase at end
    {
        inplace_vector<T, 16> v{0, 1, 2, 3, 4};
        auto it = v.erase(v.begin(), v.end());
        ASSERT_EQ(it, v.end());
        ASSERT_TRUE(v.empty());
    }
}

} // namespace

TEST(inplace_vector, erase) {
    test_erase_impl<int>(this);
    test_erase_impl<Int>(this);
    test_erase_impl<TRInt>(this);
    test_erase_impl<TMInt>(this);
}
