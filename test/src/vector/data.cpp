#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/operator_hijacker.hpp>
#include <ciel/vector.hpp>

#include <memory>

using namespace ciel;

TEST(vector, data) {
    {
        vector<int> v;
        ASSERT_EQ(v.data(), nullptr);
    }
    {
        vector<int> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
    {
        vector<operator_hijacker> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
    {
        vector<int, fancy_allocator<int>> v;
        ASSERT_EQ(v.data(), nullptr);
    }
    {
        vector<int, fancy_allocator<int>> v(100);
        ASSERT_EQ(v.data(), &v.front());
    }
    {
        vector<operator_hijacker, fancy_allocator<operator_hijacker>> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
}

TEST(vector, data_const) {
    {
        const vector<int> v;
        ASSERT_EQ(v.data(), nullptr);
    }
    {
        const vector<int> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
    {
        const vector<operator_hijacker> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
    {
        const vector<int, fancy_allocator<int>> v;
        ASSERT_EQ(v.data(), nullptr);
    }
    {
        const vector<int, fancy_allocator<int>> v(100);
        ASSERT_EQ(v.data(), &v.front());
    }
    {
        const vector<operator_hijacker, fancy_allocator<operator_hijacker>> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
}
