#include <gtest/gtest.h>

#include <ciel/vector.hpp>

#include <iostream>

using namespace ciel;

TEST(vector, print) {
    {
        vector<int> v;
        v.reserve(10);
        std::cout << v;
    }
    {
        vector<int> v{0, 1, 2, 3, 4};
        v.reserve(10);
        std::cout << v;
    }
}
