#include <gtest/gtest.h>

#include <ciel/test/propagate_allocator.hpp>
#include <ciel/vector.hpp>

#include <memory>

using namespace ciel;

namespace {

template<class C>
void test_swap_impl(::testing::Test*, C& lhs, C& rhs) {
    const auto lhs_copy = lhs;
    const auto rhs_copy = rhs;

    lhs.swap(rhs);

    ASSERT_EQ(lhs.size(), 200);
    ASSERT_EQ(rhs.size(), 100);

    if (std::allocator_traits<typename C::allocator_type>::propagate_on_container_swap::value) {
        ASSERT_EQ(lhs.get_allocator(), rhs_copy.get_allocator());
        ASSERT_EQ(rhs.get_allocator(), lhs_copy.get_allocator());
    }
}

} // namespace

TEST(vector, swap) {
    {
        // propagate_on_container_swap: false_type, equal
        vector<int, non_pocs_allocator<int>> l(100, non_pocs_allocator<int>(5));
        vector<int, non_pocs_allocator<int>> l2(200, non_pocs_allocator<int>(5));
        test_swap_impl(this, l, l2);
    }
    {
        // propagate_on_container_swap: false_type, unequal
        vector<int, non_pocs_allocator<int>> l(100, non_pocs_allocator<int>(5));
        vector<int, non_pocs_allocator<int>> l2(200, non_pocs_allocator<int>(3));
        test_swap_impl(this, l, l2);
    }
    {
        // propagate_on_container_swap: true_type, equal
        vector<int, pocs_allocator<int>> l(100, pocs_allocator<int>(5));
        vector<int, pocs_allocator<int>> l2(200, pocs_allocator<int>(5));
        test_swap_impl(this, l, l2);
    }
    {
        // propagate_on_container_swap: true_type, unequal
        vector<int, pocs_allocator<int>> l(100, pocs_allocator<int>(5));
        vector<int, pocs_allocator<int>> l2(200, pocs_allocator<int>(3));
        test_swap_impl(this, l, l2);
    }
}
