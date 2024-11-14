#ifndef CIELLAB_INCLUDE_CIEL_TEST_SBV_ASSIGN_TESTS_HPP_
#define CIELLAB_INCLUDE_CIEL_TEST_SBV_ASSIGN_TESTS_HPP_

#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/forward_iterator.hpp>
#include <ciel/test/input_iterator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/operator_hijacker.hpp>
#include <ciel/test/propagate_allocator.hpp>
#include <ciel/test/random_access_iterator.hpp>
#include <ciel/test/range.hpp>

#include <array>

template<class C>
inline void test_operator_copy_impl(::testing::Test*, C& lhs, C& rhs) {
    rhs = lhs;
    ASSERT_EQ(lhs, rhs);
    if (std::allocator_traits<typename C::allocator_type>::propagate_on_container_copy_assignment::value) {
        ASSERT_EQ(lhs.get_allocator(), rhs.get_allocator());
    }
}

template<class C>
inline void test_operator_move_impl(::testing::Test*, C& lhs, C& rhs) {
    const auto temp = lhs;
    rhs             = std::move(lhs);
    ASSERT_EQ(temp, rhs);
}

template<class Iter, class C>
inline void test_assign_iterator_range_impl(::testing::Test*, const C& c) {
    using T = typename C::value_type;

    {
        auto v = c;
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        v.assign(Iter{arr.data()}, Iter{arr.data() + arr.size()});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
    }
    // range without size
    {
        auto v = c;
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        auto r = ciel::make_range(Iter{arr.data()}, Iter{arr.data() + arr.size()});
        v.assign_range(r);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
    }
    {
        auto v = c;
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        auto r = ciel::make_range(Iter{arr.data()}, Iter{arr.data() + arr.size()});
        v.assign_range(std::move(r));
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
        if (!std::is_trivial<T>::value) {
            ASSERT_EQ(arr, std::initializer_list<T>({-1, -1, -1, -1, -1}));
        }
    }
    // range with size
    {
        auto v = c;
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        auto r = ciel::make_range(Iter{arr.data()}, Iter{arr.data() + arr.size()}, arr.size());
        v.assign_range(r);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
    }
    {
        auto v = c;
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        auto r = ciel::make_range(Iter{arr.data()}, Iter{arr.data() + arr.size()}, arr.size());
        v.assign_range(std::move(r));
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4}));
        if (!std::is_trivial<T>::value) {
            ASSERT_EQ(arr, std::initializer_list<T>({-1, -1, -1, -1, -1}));
        }
    }
    // empty range
    {
        auto v = c;
        v.assign(Iter{nullptr}, Iter{nullptr});
        ASSERT_TRUE(v.empty());
    }
}

template<class C>
inline void test_assign_size_value_impl(::testing::Test*, C& v) {
    using T = typename C::value_type;

    v.assign(5, 6);
    ASSERT_EQ(v, std::initializer_list<T>({6, 6, 6, 6, 6}));
}

#endif // CIELLAB_INCLUDE_CIEL_TEST_SBV_ASSIGN_TESTS_HPP_
