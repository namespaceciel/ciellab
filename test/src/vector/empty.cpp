#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

namespace {

template<class C>
void
test_empty_impl(::testing::Test*) {
    C c;
    ASSERT_TRUE(c.empty());

    c.push_back(1);
    ASSERT_FALSE(c.empty());

    c.clear();
    ASSERT_TRUE(c.empty());
}

} // namespace

TEST(vector, empty) {
    test_empty_impl<vector<int>>(this);
    test_empty_impl<vector<int, fancy_allocator<int>>>(this);
}
