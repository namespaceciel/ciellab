#include <gtest/gtest.h>

#include <ciel/test/min_allocator.hpp>
#include <ciel/test/operator_hijacker.hpp>
#include <ciel/test/safe_allocator.hpp>
#include <ciel/vector.hpp>

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
        vector<int, min_allocator<int>> v;
        ASSERT_EQ(v.data(), nullptr);
    }
    {
        vector<int, min_allocator<int>> v(100);
        ASSERT_EQ(v.data(), &v.front());
    }
    {
        vector<operator_hijacker, min_allocator<operator_hijacker>> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
    {
        vector<int, safe_allocator<int>> v;
        ASSERT_EQ(v.data(), nullptr);
    }
    {
        vector<int, safe_allocator<int>> v(100);
        ASSERT_EQ(v.data(), &v.front());
    }
    {
        vector<operator_hijacker, safe_allocator<operator_hijacker>> v(100);
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
        const vector<int, min_allocator<int>> v;
        ASSERT_EQ(v.data(), nullptr);
    }
    {
        const vector<int, min_allocator<int>> v(100);
        ASSERT_EQ(v.data(), &v.front());
    }
    {
        const vector<operator_hijacker, min_allocator<operator_hijacker>> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
    {
        const vector<int, safe_allocator<int>> v;
        ASSERT_EQ(v.data(), nullptr);
    }
    {
        const vector<int, safe_allocator<int>> v(100);
        ASSERT_EQ(v.data(), &v.front());
    }
    {
        const vector<operator_hijacker, safe_allocator<operator_hijacker>> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
}
