#include <gtest/gtest.h>

#include <ciel/split_buffer.hpp>

#include <iostream>

using namespace ciel;

TEST(split_buffer, print) {
    {
        split_buffer<int> v;
        v.reserve_back_spare(10);
        v.reserve_front_spare(4);
        std::cout << v << '\n';
    }
    {
        split_buffer<int> v{0, 1, 2, 3, 4};
        v.reserve_back_spare(10);
        v.reserve_front_spare(6);
        std::cout << v << '\n';
    }
}
