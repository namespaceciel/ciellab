#include <gtest/gtest.h>

#include <ciel/split_buffer.hpp>
#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/operator_hijacker.hpp>

using namespace ciel;

TEST(split_buffer, data) {
    {
        split_buffer<int> v;
        ASSERT_EQ(v.data(), nullptr);
    }
    {
        split_buffer<int> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
    {
        split_buffer<operator_hijacker> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
    {
        split_buffer<int, fancy_allocator<int>> v;
        ASSERT_EQ(v.data(), nullptr);
    }
    {
        split_buffer<int, fancy_allocator<int>> v(100);
        ASSERT_EQ(v.data(), &v.front());
    }
    {
        split_buffer<operator_hijacker, fancy_allocator<operator_hijacker>> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
}

TEST(split_buffer, data_const) {
    {
        const split_buffer<int> v;
        ASSERT_EQ(v.data(), nullptr);
    }
    {
        const split_buffer<int> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
    {
        const split_buffer<operator_hijacker> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
    {
        const split_buffer<int, fancy_allocator<int>> v;
        ASSERT_EQ(v.data(), nullptr);
    }
    {
        const split_buffer<int, fancy_allocator<int>> v(100);
        ASSERT_EQ(v.data(), &v.front());
    }
    {
        const split_buffer<operator_hijacker, fancy_allocator<operator_hijacker>> v(100);
        ASSERT_EQ(v.data(), std::addressof(v.front()));
    }
}
