#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/forward_iterator.hpp>
#include <ciel/test/input_iterator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/operator_hijacker.hpp>
#include <ciel/test/propagate_allocator.hpp>
#include <ciel/test/random_access_iterator.hpp>

#include <array>

template<class C>
inline void
test_operator_copy_impl(::testing::Test*, C& lhs, C& rhs) {
    rhs = lhs;
    ASSERT_EQ(lhs, rhs);
    ASSERT_EQ(lhs.get_allocator(), rhs.get_allocator());
}

template<class C>
inline void
test_operator_move_impl(::testing::Test*, C& lhs, C& rhs) {
    const auto temp = lhs;
    rhs             = std::move(lhs);
    ASSERT_EQ(temp, rhs);
    ASSERT_EQ(temp.get_allocator(), rhs.get_allocator());
}

template<class Iter, class C>
inline void
test_assign_iterator_range_impl(::testing::Test*, const C& c) {
    using T = typename C::value_type;

    {
        auto v = c;
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        v.assign(Iter{arr.data()}, Iter{arr.data() + arr.size()});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
    }
    {
        // empty range
        auto v = c;
        v.assign(Iter{nullptr}, Iter{nullptr});
        ASSERT_TRUE(v.empty());
    }
}

template<class C>
inline void
test_assign_size_value_impl(::testing::Test*, C& v) {
    using T = typename C::value_type;

    v.assign(5, 6);
    ASSERT_EQ(v, std::initializer_list<T>({6, 6, 6, 6, 6}));
}
