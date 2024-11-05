#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

namespace {

template<class C>
void
test_size_impl(::testing::Test*) {
    C c;
    ASSERT_EQ(c.size(), 0);

    c.push_back(2);
    ASSERT_EQ(c.size(), 1);

    c.push_back(1);
    ASSERT_EQ(c.size(), 2);

    c.push_back(3);
    ASSERT_EQ(c.size(), 3);

    c.erase(c.begin());
    ASSERT_EQ(c.size(), 2);

    c.erase(c.begin());
    ASSERT_EQ(c.size(), 1);

    c.erase(c.begin());
    ASSERT_EQ(c.size(), 0);
}

} // namespace

TEST(vector, size) {
    test_size_impl<vector<int>>(this);
    test_size_impl<vector<int, fancy_allocator<int>>>(this);
}
