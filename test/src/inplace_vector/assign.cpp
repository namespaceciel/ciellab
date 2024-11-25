#include <gtest/gtest.h>

#include <ciel/inplace_vector.hpp>

#include <tuple>
#include <utility>

using namespace ciel;

TEST(inplace_vector, issue_5) {
    int x = 1;
    int y = 2;
    int z = 3;
    int w = 4;

    ciel::inplace_vector<std::tuple<int&>, 2> v1{std::tie(x), std::tie(y)};
    ciel::inplace_vector<std::tuple<int&>, 2> v2{std::tie(z), std::tie(w)};

    v1 = std::move(v2);

    ASSERT_EQ(x, 3);
    ASSERT_EQ(y, 4);
    ASSERT_EQ(z, 3);
    ASSERT_EQ(w, 4);
}
