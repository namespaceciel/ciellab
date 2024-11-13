#include <gtest/gtest.h>

#include <ciel/inplace_vector.hpp>
#include <ciel/test/forward_iterator.hpp>
#include <ciel/test/input_iterator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/random_access_iterator.hpp>

#include <array>
#include <initializer_list>
#include <type_traits>

using namespace ciel;

namespace {

template<class C>
void test_insert_size_value_impl(::testing::Test*) {
    using T = typename C::value_type;

    const T value(5);

    // emplace, count > pos_end_dis
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.end() - 1, 4, value);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 5, 5, 5, 5, 4}));
    }
    // emplace, count < pos_end_dis
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, 2, value);
        ASSERT_EQ(v, std::initializer_list<T>({0, 5, 5, 1, 2, 3, 4}));
    }
    // emplace at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.end(), 4, value);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 5, 5, 5, 5}));
    }
}

template<class C>
void test_insert_size_self_value_impl(::testing::Test*) {
    using T = typename C::value_type;

    // emplace, count > pos_end_dis
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.end() - 1, 4, v[1]);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 1, 1, 1, 1, 4}));
    }
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.end() - 1, 4, v.back());
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 4, 4, 4, 4}));
    }
    // emplace, count < pos_end_dis
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, 2, v.front());
        ASSERT_EQ(v, std::initializer_list<T>({0, 0, 0, 1, 2, 3, 4}));
    }
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, 2, v.back());
        ASSERT_EQ(v, std::initializer_list<T>({0, 4, 4, 1, 2, 3, 4}));
    }
    // emplace at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.end(), 4, v[1]);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 1, 1, 1, 1}));
    }
}

template<class C>
void test_insert_lvalue_impl(::testing::Test*) {
    using T = typename C::value_type;

    const T value(5);

    // emplace not at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, value);
        ASSERT_EQ(v, std::initializer_list<T>({0, 5, 1, 2, 3, 4}));
    }
    // emplace at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.end(), value);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 5}));
    }
}

template<class C>
void test_insert_self_lvalue_impl(::testing::Test*) {
    using T = typename C::value_type;

    // emplace not at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, v.front());
        ASSERT_EQ(v, std::initializer_list<T>({0, 0, 1, 2, 3, 4}));
    }
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, v.back());
        ASSERT_EQ(v, std::initializer_list<T>({0, 4, 1, 2, 3, 4}));
    }
    // emplace at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.end(), v[1]);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 1}));
    }
}

template<class C>
void test_insert_rvalue_impl(::testing::Test*) {
    using T = typename C::value_type;

    // emplace not at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, T{5});
        ASSERT_EQ(v, std::initializer_list<T>({0, 5, 1, 2, 3, 4}));
    }
    // emplace at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.end(), T{5});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 5}));
    }
}

template<class C>
void test_insert_self_rvalue_impl(::testing::Test*) {
    using T = typename C::value_type;

    // emplace not at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, std::move(v.front()));
        if (!std::is_trivial<T>::value) {
            ASSERT_EQ(v, std::initializer_list<T>({-1, 0, 1, 2, 3, 4}));
        }
    }
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, std::move(v.back()));
        if (!std::is_trivial<T>::value) {
            ASSERT_EQ(v, std::initializer_list<T>({0, 4, 1, 2, 3, -1}));
        }
    }
    // emplace at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.end(), std::move(v[1]));
        if (!std::is_trivial<T>::value) {
            ASSERT_EQ(v, std::initializer_list<T>({0, -1, 2, 3, 4, 1}));
        }
    }
}

template<class C, class Iter>
void test_insert_iterator_range_impl(::testing::Test*) {
    using T = typename C::value_type;

    {
        C v{0, 1, 2, 3, 4, 5, 6};
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        v.insert(v.begin(), Iter{arr.data()}, Iter{arr.data() + arr.size()});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 5, 6}));

        v.insert(v.begin(), Iter{nullptr}, Iter{nullptr});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 5, 6}));
    }
    {
        C v{0, 1, 2, 3, 4, 5, 6};
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        v.insert(v.end() - 1, Iter{arr.data()}, Iter{arr.data() + arr.size()});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 6}));

        v.insert(v.begin(), Iter{nullptr}, Iter{nullptr});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 6}));
    }
    {
        C v{0, 1, 2, 3, 4, 5, 6};
        std::array<T, 5> arr{0, 1, 2, 3, 4};
        v.insert(v.end(), Iter{arr.data()}, Iter{arr.data() + arr.size()});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4}));

        v.insert(v.begin(), Iter{nullptr}, Iter{nullptr});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4}));
    }
}

} // namespace

TEST(inplace_vector, insert_size_value) {
    test_insert_size_value_impl<inplace_vector<int, 16>>(this);
    test_insert_size_value_impl<inplace_vector<Int, 16>>(this);
    test_insert_size_value_impl<inplace_vector<TRInt, 16>>(this);
    test_insert_size_value_impl<inplace_vector<TMInt, 16>>(this);
}

TEST(inplace_vector, insert_size_self_value) {
    test_insert_size_self_value_impl<inplace_vector<int, 16>>(this);
    test_insert_size_self_value_impl<inplace_vector<Int, 16>>(this);
    test_insert_size_self_value_impl<inplace_vector<TRInt, 16>>(this);
    test_insert_size_self_value_impl<inplace_vector<TMInt, 16>>(this);
}

TEST(inplace_vector, insert_lvalue) {
    test_insert_lvalue_impl<inplace_vector<int, 16>>(this);
    test_insert_lvalue_impl<inplace_vector<Int, 16>>(this);
    test_insert_lvalue_impl<inplace_vector<TRInt, 16>>(this);
    test_insert_lvalue_impl<inplace_vector<TMInt, 16>>(this);
}

TEST(inplace_vector, insert_self_lvalue) {
    test_insert_self_lvalue_impl<inplace_vector<int, 16>>(this);
    test_insert_self_lvalue_impl<inplace_vector<Int, 16>>(this);
    test_insert_self_lvalue_impl<inplace_vector<TRInt, 16>>(this);
    test_insert_self_lvalue_impl<inplace_vector<TMInt, 16>>(this);
}

TEST(inplace_vector, insert_rvalue) {
    test_insert_rvalue_impl<inplace_vector<int, 16>>(this);
    test_insert_rvalue_impl<inplace_vector<Int, 16>>(this);
    test_insert_rvalue_impl<inplace_vector<TRInt, 16>>(this);
    test_insert_rvalue_impl<inplace_vector<TMInt, 16>>(this);
}

TEST(inplace_vector, insert_self_rvalue) {
    test_insert_self_rvalue_impl<inplace_vector<int, 16>>(this);
    test_insert_self_rvalue_impl<inplace_vector<Int, 16>>(this);
    test_insert_self_rvalue_impl<inplace_vector<TRInt, 16>>(this);
    test_insert_self_rvalue_impl<inplace_vector<TMInt, 16>>(this);
}

TEST(inplace_vector, insert_iterator_range) {
    test_insert_iterator_range_impl<inplace_vector<int, 16>, InputIterator<int>>(this);
    test_insert_iterator_range_impl<inplace_vector<Int, 16>, InputIterator<Int>>(this);
    test_insert_iterator_range_impl<inplace_vector<TRInt, 16>, InputIterator<TRInt>>(this);
    test_insert_iterator_range_impl<inplace_vector<TMInt, 16>, InputIterator<TMInt>>(this);

    test_insert_iterator_range_impl<inplace_vector<int, 16>, ForwardIterator<int>>(this);
    test_insert_iterator_range_impl<inplace_vector<Int, 16>, ForwardIterator<Int>>(this);
    test_insert_iterator_range_impl<inplace_vector<TRInt, 16>, ForwardIterator<TRInt>>(this);
    test_insert_iterator_range_impl<inplace_vector<TMInt, 16>, ForwardIterator<TMInt>>(this);

    test_insert_iterator_range_impl<inplace_vector<int, 16>, RandomAccessIterator<int>>(this);
    test_insert_iterator_range_impl<inplace_vector<Int, 16>, RandomAccessIterator<Int>>(this);
    test_insert_iterator_range_impl<inplace_vector<TRInt, 16>, RandomAccessIterator<TRInt>>(this);
    test_insert_iterator_range_impl<inplace_vector<TMInt, 16>, RandomAccessIterator<TMInt>>(this);

    test_insert_iterator_range_impl<inplace_vector<int, 16>, int*>(this);
    test_insert_iterator_range_impl<inplace_vector<Int, 16>, Int*>(this);
    test_insert_iterator_range_impl<inplace_vector<TRInt, 16>, TRInt*>(this);
    test_insert_iterator_range_impl<inplace_vector<TMInt, 16>, TMInt*>(this);
}
