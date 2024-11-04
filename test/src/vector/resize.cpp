#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, resize_value) {
    {
        vector<Int> v(100);

        v.resize(50, 1);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);
        ASSERT_EQ(v, vector<Int>(50));

        v.resize(200, 1);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);
        ASSERT_TRUE(std::all_of(v.begin(), v.begin() + 50, [](int i) {
            return i == 0;
        }));
        ASSERT_TRUE(std::all_of(v.begin() + 50, v.end(), [](int i) {
            return i == 1;
        }));
    }
    {
        vector<Int, fancy_allocator<Int>> v(100);

        v.resize(50, 1);
        ASSERT_EQ(v.size(), 50);
        ASSERT_GE(v.capacity(), 100);
        const vector<Int, fancy_allocator<Int>> temp(50);
        ASSERT_EQ(v, temp);

        v.resize(200, 1);
        ASSERT_EQ(v.size(), 200);
        ASSERT_GE(v.capacity(), 200);
        ASSERT_TRUE(std::all_of(v.begin(), v.begin() + 50, [](int i) {
            return i == 0;
        }));
        ASSERT_TRUE(std::all_of(v.begin() + 50, v.end(), [](int i) {
            return i == 1;
        }));
    }
}

TEST(vector, resize_self_value) {
    {
        vector<Int> v(2, 42);

        v.resize(v.capacity() + 1, v[1]);
        ASSERT_TRUE(std::all_of(v.begin(), v.end(), [](int i) {
            return i == 42;
        }));
    }
}
