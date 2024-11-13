#include <gtest/gtest.h>

#include <ciel/split_buffer.hpp>
#include <ciel/test/int_wrapper.hpp>

#include <initializer_list>

using namespace ciel;

namespace {

template<class T>
void test_erase_impl(::testing::Test*) {
    // erase single
    {
        split_buffer<T> v{0, 1, 2, 3, 4};
        {
            auto it = v.erase(v.begin());
            ASSERT_EQ(it, v.begin());
            ASSERT_EQ(v, std::initializer_list<T>({1, 2, 3, 4}));
        }
        {
            auto it = v.erase(v.begin() + 1);
            ASSERT_EQ(it, v.begin() + 1);
            ASSERT_EQ(v, std::initializer_list<T>({1, 3, 4}));
        }
        {
            auto it = v.erase(v.end() - 2);
            ASSERT_EQ(it, v.end() - 1);
            ASSERT_EQ(v, std::initializer_list<T>({1, 4}));
        }
        {
            auto it = v.erase(v.end() - 1);
            ASSERT_EQ(it, v.end());
            ASSERT_EQ(v, std::initializer_list<T>({1}));
        }
    }
    // erase at begin
    {
        split_buffer<T> v{0, 1, 2, 3, 4};
        auto it = v.erase(v.begin(), v.begin() + 2);
        ASSERT_EQ(it, v.begin());
        ASSERT_EQ(v, std::initializer_list<T>({2, 3, 4}));
    }
    // erase at first half
    {
        split_buffer<T> v{0, 1, 2, 3, 4};
        auto it = v.erase(v.begin() + 1, v.begin() + 3);
        ASSERT_EQ(it, v.begin() + 1);
        ASSERT_EQ(v, std::initializer_list<T>({0, 3, 4}));
    }
    // erase at second half
    {
        split_buffer<T> v{0, 1, 2, 3, 4};
        auto it = v.erase(v.begin() + 2, v.begin() + 4);
        ASSERT_EQ(it, v.begin() + 2);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 4}));
    }
    // erase at end
    {
        split_buffer<T> v{0, 1, 2, 3, 4};
        auto it = v.erase(v.begin() + 2, v.end());
        ASSERT_EQ(it, v.end());
        ASSERT_EQ(v, std::initializer_list<T>({0, 1}));
    }
}

} // namespace

TEST(split_buffer, erase) {
    test_erase_impl<int>(this);
    test_erase_impl<Int>(this);
    test_erase_impl<TRInt>(this);
    test_erase_impl<TMInt>(this);
}
