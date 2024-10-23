#include <gtest/gtest.h>

#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/min_allocator.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, emplace_back) {
    {
        vector<Int, min_allocator<Int>> v;

        for (int i = 0; i < 64; ++i) {
            v.emplace_back(i);
        }

        for (int i = 0; i < 64; ++i) {
            ASSERT_EQ(v[i], Int(i));
        }
    }
}

TEST(vector, emplace_back_self_reference) {
    {
        vector<Int, min_allocator<Int>> v{0, 1, 2, 3, 4};

        while (v.size() < v.capacity()) {
            v.emplace_back(123);
        }
        v.emplace_back(v[0]);

        ASSERT_EQ(v.back(), v[0]);

        while (v.size() < v.capacity()) {
            v.emplace_back(123);
        }
        v.emplace_back(v[1]);

        ASSERT_EQ(v.back(), v[1]);
    }
}

// non-standard extension
TEST(vector, emplace_back_initializer_list) {
    {
        vector<vector<Int, min_allocator<Int>>, min_allocator<vector<Int, min_allocator<Int>>>> v1;
        v1.emplace_back({Int{0}, Int{1}, Int{2}, Int{3}, Int{4}});

        ASSERT_EQ(v1, std::initializer_list<std::initializer_list<Int>>({
                          {0, 1, 2, 3, 4}
        }));
    }
}
