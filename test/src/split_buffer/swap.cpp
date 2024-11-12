#include <gtest/gtest.h>

#include <ciel/split_buffer.hpp>
#include <ciel/test/propagate_allocator.hpp>

using namespace ciel;

namespace {

template<class C>
void test_swap_impl(::testing::Test*, C& lhs, C& rhs) {
    const auto lhs_copy = lhs;
    const auto rhs_copy = rhs;

    lhs.swap(rhs);

    ASSERT_EQ(lhs.size(), 200);
    ASSERT_EQ(rhs.size(), 100);

    ASSERT_EQ(lhs.get_allocator(), rhs_copy.get_allocator());
    ASSERT_EQ(rhs.get_allocator(), lhs_copy.get_allocator());
}

} // namespace

TEST(split_buffer, swap) {
    {
        // propagate_on_container_swap: false_type, equal
        split_buffer<int, non_pocs_allocator<int>> l(100, non_pocs_allocator<int>(5));
        split_buffer<int, non_pocs_allocator<int>> l2(200, non_pocs_allocator<int>(5));
        test_swap_impl(this, l, l2);
    }
    {
        // propagate_on_container_swap: false_type, unequal
        split_buffer<int, non_pocs_allocator<int>> l(100, non_pocs_allocator<int>(5));
        split_buffer<int, non_pocs_allocator<int>> l2(200, non_pocs_allocator<int>(3));
        test_swap_impl(this, l, l2);
    }
    {
        // propagate_on_container_swap: true_type, equal
        split_buffer<int, pocs_allocator<int>> l(100, pocs_allocator<int>(5));
        split_buffer<int, pocs_allocator<int>> l2(200, pocs_allocator<int>(5));
        test_swap_impl(this, l, l2);
    }
    {
        // propagate_on_container_swap: true_type, unequal
        split_buffer<int, pocs_allocator<int>> l(100, pocs_allocator<int>(5));
        split_buffer<int, pocs_allocator<int>> l2(200, pocs_allocator<int>(3));
        test_swap_impl(this, l, l2);
    }
}
