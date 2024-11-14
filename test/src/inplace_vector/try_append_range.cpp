#include <gtest/gtest.h>

#include <ciel/inplace_vector.hpp>
#include <ciel/test/forward_iterator.hpp>
#include <ciel/test/input_iterator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/random_access_iterator.hpp>
#include <ciel/test/range.hpp>

#include <array>
#include <initializer_list>
#include <type_traits>

using namespace ciel;

namespace {

template<class C, class Iter>
void test_try_append_range_impl(::testing::Test*) {
    using T = typename C::value_type;

    const std::initializer_list<T> il{-1, -1, -1, -1, -1};

    // range without size
    {
        C v{0, 1};
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        auto r = ciel::make_range(Iter{arr.data()}, Iter{arr.data() + arr.size()});
        ASSERT_EQ(v.try_append_range(r), r.end());
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 0, 1, 2, 3, 4}));
    }
    {
        C v{0, 1};
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        auto r = ciel::make_range(Iter{arr.data()}, Iter{arr.data() + arr.size()});
        ASSERT_EQ(v.try_append_range(std::move(r)), r.end());
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 0, 1, 2, 3, 4}));
        if (!std::is_trivial<T>::value) {
            ASSERT_TRUE(std::equal(arr.begin(), arr.end(), il.begin()));
        }
    }
    {
        C v{0, 1, 2, 3, 4};
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        auto r = ciel::make_range(Iter{arr.data()}, Iter{arr.data() + arr.size()});
        ASSERT_EQ(*v.try_append_range(r), 3);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 0, 1, 2}));
    }
    {
        C v{0, 1, 2, 3, 4};
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        auto r = ciel::make_range(Iter{arr.data()}, Iter{arr.data() + arr.size()});
        ASSERT_EQ(*v.try_append_range(std::move(r)), 3);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 0, 1, 2}));
        if (!std::is_trivial<T>::value) {
            ASSERT_TRUE(std::equal(arr.begin(), arr.begin() + 3, il.begin()));
        }
    }
    // range with size
    {
        C v{0, 1};
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        auto r = ciel::make_range(Iter{arr.data()}, Iter{arr.data() + arr.size()}, arr.size());
        ASSERT_EQ(v.try_append_range(r), r.end());
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 0, 1, 2, 3, 4}));
    }
    {
        C v{0, 1};
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        auto r = ciel::make_range(Iter{arr.data()}, Iter{arr.data() + arr.size()}, arr.size());
        ASSERT_EQ(v.try_append_range(std::move(r)), r.end());
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 0, 1, 2, 3, 4}));
        if (!std::is_trivial<T>::value) {
            ASSERT_TRUE(std::equal(arr.begin(), arr.end(), il.begin()));
        }
    }
    {
        C v{0, 1, 2, 3, 4};
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        auto r = ciel::make_range(Iter{arr.data()}, Iter{arr.data() + arr.size()}, arr.size());
        ASSERT_EQ(*v.try_append_range(r), 3);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 0, 1, 2}));
    }
    {
        C v{0, 1, 2, 3, 4};
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        auto r = ciel::make_range(Iter{arr.data()}, Iter{arr.data() + arr.size()}, arr.size());
        ASSERT_EQ(*v.try_append_range(std::move(r)), 3);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 0, 1, 2}));
        if (!std::is_trivial<T>::value) {
            ASSERT_TRUE(std::equal(arr.begin(), arr.begin() + 3, il.begin()));
        }
    }
}

} // namespace

TEST(inplace_vector, try_append_range) {
    test_try_append_range_impl<inplace_vector<int, 8>, InputIterator<int>>(this);
    test_try_append_range_impl<inplace_vector<Int, 8>, InputIterator<Int>>(this);
    test_try_append_range_impl<inplace_vector<TRInt, 8>, InputIterator<TRInt>>(this);
    test_try_append_range_impl<inplace_vector<TMInt, 8>, InputIterator<TMInt>>(this);

    test_try_append_range_impl<inplace_vector<int, 8>, ForwardIterator<int>>(this);
    test_try_append_range_impl<inplace_vector<Int, 8>, ForwardIterator<Int>>(this);
    test_try_append_range_impl<inplace_vector<TRInt, 8>, ForwardIterator<TRInt>>(this);
    test_try_append_range_impl<inplace_vector<TMInt, 8>, ForwardIterator<TMInt>>(this);

    test_try_append_range_impl<inplace_vector<int, 8>, RandomAccessIterator<int>>(this);
    test_try_append_range_impl<inplace_vector<Int, 8>, RandomAccessIterator<Int>>(this);
    test_try_append_range_impl<inplace_vector<TRInt, 8>, RandomAccessIterator<TRInt>>(this);
    test_try_append_range_impl<inplace_vector<TMInt, 8>, RandomAccessIterator<TMInt>>(this);

    test_try_append_range_impl<inplace_vector<int, 8>, int*>(this);
    test_try_append_range_impl<inplace_vector<Int, 8>, Int*>(this);
    test_try_append_range_impl<inplace_vector<TRInt, 8>, TRInt*>(this);
    test_try_append_range_impl<inplace_vector<TMInt, 8>, TMInt*>(this);
}
