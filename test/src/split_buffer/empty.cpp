#include <gtest/gtest.h>

#include <ciel/split_buffer.hpp>
#include <ciel/test/fancy_allocator.hpp>

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

TEST(split_buffer, empty) {
    test_empty_impl<split_buffer<int>>(this);
    test_empty_impl<split_buffer<int, fancy_allocator<int>>>(this);
}
