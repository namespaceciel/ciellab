#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/forward_iterator.hpp>
#include <ciel/test/input_iterator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/random_access_iterator.hpp>
#include <ciel/vector.hpp>

#include <array>

using namespace ciel;

namespace {

template<class C>
void test_insert_size_value_impl(::testing::Test*) {
    using T = typename C::value_type;

    const T value(5);

    // expansion
    {
        C v{0, 1, 2, 3, 4};
        v.resize(v.capacity());

        v.insert(v.begin() + 2, 4, value);
        v.resize(9);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 5, 5, 5, 5, 2, 3, 4}));
    }
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

    // expansion
    {
        C v{0, 1, 2, 3, 4};
        v.resize(v.capacity());

        v.insert(v.begin() + 2, 4, v[1]);
        v.resize(9);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 1, 1, 1, 1, 2, 3, 4}));
    }
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

    // expansion
    {
        C v{0, 1, 2, 3, 4};
        v.resize(v.capacity());

        v.insert(v.begin() + 2, value);
        v.resize(6);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 5, 2, 3, 4}));
    }
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

    // expansion
    {
        C v{0, 1, 2, 3, 4};
        v.resize(v.capacity());

        v.insert(v.begin() + 2, v[1]);
        v.resize(6);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 1, 2, 3, 4}));
    }
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

    // expansion
    {
        C v{0, 1, 2, 3, 4};
        v.resize(v.capacity());

        v.insert(v.begin() + 2, T{5});
        v.resize(6);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 5, 2, 3, 4}));
    }
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

    // expansion
    {
        C v{0, 1, 2, 3, 4};
        v.resize(v.capacity());

        v.insert(v.begin() + 2, std::move(v[1]));
        v.resize(6);
        if (!std::is_trivial<T>::value) {
            ASSERT_EQ(v, std::initializer_list<T>({0, -1, 1, 2, 3, 4}));
        }
    }
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

TEST(vector, insert_size_value) {
    test_insert_size_value_impl<vector<int>>(this);
    test_insert_size_value_impl<vector<Int>>(this);
    test_insert_size_value_impl<vector<TRInt>>(this);
    test_insert_size_value_impl<vector<TMInt>>(this);

    test_insert_size_value_impl<vector<int, fancy_allocator<int>>>(this);
    test_insert_size_value_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_insert_size_value_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_insert_size_value_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, insert_size_self_value) {
    test_insert_size_self_value_impl<vector<int>>(this);
    test_insert_size_self_value_impl<vector<Int>>(this);
    test_insert_size_self_value_impl<vector<TRInt>>(this);
    test_insert_size_self_value_impl<vector<TMInt>>(this);

    test_insert_size_self_value_impl<vector<int, fancy_allocator<int>>>(this);
    test_insert_size_self_value_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_insert_size_self_value_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_insert_size_self_value_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, insert_lvalue) {
    test_insert_lvalue_impl<vector<int>>(this);
    test_insert_lvalue_impl<vector<Int>>(this);
    test_insert_lvalue_impl<vector<TRInt>>(this);
    test_insert_lvalue_impl<vector<TMInt>>(this);

    test_insert_lvalue_impl<vector<int, fancy_allocator<int>>>(this);
    test_insert_lvalue_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_insert_lvalue_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_insert_lvalue_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, insert_self_lvalue) {
    test_insert_self_lvalue_impl<vector<int>>(this);
    test_insert_self_lvalue_impl<vector<Int>>(this);
    test_insert_self_lvalue_impl<vector<TRInt>>(this);
    test_insert_self_lvalue_impl<vector<TMInt>>(this);

    test_insert_self_lvalue_impl<vector<int, fancy_allocator<int>>>(this);
    test_insert_self_lvalue_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_insert_self_lvalue_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_insert_self_lvalue_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, insert_rvalue) {
    test_insert_rvalue_impl<vector<int>>(this);
    test_insert_rvalue_impl<vector<Int>>(this);
    test_insert_rvalue_impl<vector<TRInt>>(this);
    test_insert_rvalue_impl<vector<TMInt>>(this);

    test_insert_rvalue_impl<vector<int, fancy_allocator<int>>>(this);
    test_insert_rvalue_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_insert_rvalue_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_insert_rvalue_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, insert_self_rvalue) {
    test_insert_self_rvalue_impl<vector<int>>(this);
    test_insert_self_rvalue_impl<vector<Int>>(this);
    test_insert_self_rvalue_impl<vector<TRInt>>(this);
    test_insert_self_rvalue_impl<vector<TMInt>>(this);

    test_insert_self_rvalue_impl<vector<int, fancy_allocator<int>>>(this);
    test_insert_self_rvalue_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_insert_self_rvalue_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_insert_self_rvalue_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, insert_iterator_range) {
    test_insert_iterator_range_impl<vector<int>, InputIterator<int>>(this);
    test_insert_iterator_range_impl<vector<Int>, InputIterator<Int>>(this);
    test_insert_iterator_range_impl<vector<TRInt>, InputIterator<TRInt>>(this);
    test_insert_iterator_range_impl<vector<TMInt>, InputIterator<TMInt>>(this);

    test_insert_iterator_range_impl<vector<int, fancy_allocator<int>>, InputIterator<int>>(this);
    test_insert_iterator_range_impl<vector<Int, fancy_allocator<Int>>, InputIterator<Int>>(this);
    test_insert_iterator_range_impl<vector<TRInt, fancy_allocator<TRInt>>, InputIterator<TRInt>>(this);
    test_insert_iterator_range_impl<vector<TMInt, fancy_allocator<TMInt>>, InputIterator<TMInt>>(this);

    test_insert_iterator_range_impl<vector<int>, ForwardIterator<int>>(this);
    test_insert_iterator_range_impl<vector<Int>, ForwardIterator<Int>>(this);
    test_insert_iterator_range_impl<vector<TRInt>, ForwardIterator<TRInt>>(this);
    test_insert_iterator_range_impl<vector<TMInt>, ForwardIterator<TMInt>>(this);

    test_insert_iterator_range_impl<vector<int, fancy_allocator<int>>, ForwardIterator<int>>(this);
    test_insert_iterator_range_impl<vector<Int, fancy_allocator<Int>>, ForwardIterator<Int>>(this);
    test_insert_iterator_range_impl<vector<TRInt, fancy_allocator<TRInt>>, ForwardIterator<TRInt>>(this);
    test_insert_iterator_range_impl<vector<TMInt, fancy_allocator<TMInt>>, ForwardIterator<TMInt>>(this);

    test_insert_iterator_range_impl<vector<int>, RandomAccessIterator<int>>(this);
    test_insert_iterator_range_impl<vector<Int>, RandomAccessIterator<Int>>(this);
    test_insert_iterator_range_impl<vector<TRInt>, RandomAccessIterator<TRInt>>(this);
    test_insert_iterator_range_impl<vector<TMInt>, RandomAccessIterator<TMInt>>(this);

    test_insert_iterator_range_impl<vector<int, fancy_allocator<int>>, RandomAccessIterator<int>>(this);
    test_insert_iterator_range_impl<vector<Int, fancy_allocator<Int>>, RandomAccessIterator<Int>>(this);
    test_insert_iterator_range_impl<vector<TRInt, fancy_allocator<TRInt>>, RandomAccessIterator<TRInt>>(this);
    test_insert_iterator_range_impl<vector<TMInt, fancy_allocator<TMInt>>, RandomAccessIterator<TMInt>>(this);

    test_insert_iterator_range_impl<vector<int>, int*>(this);
    test_insert_iterator_range_impl<vector<Int>, Int*>(this);
    test_insert_iterator_range_impl<vector<TRInt>, TRInt*>(this);
    test_insert_iterator_range_impl<vector<TMInt>, TMInt*>(this);

    test_insert_iterator_range_impl<vector<int, fancy_allocator<int>>, int*>(this);
    test_insert_iterator_range_impl<vector<Int, fancy_allocator<Int>>, Int*>(this);
    test_insert_iterator_range_impl<vector<TRInt, fancy_allocator<TRInt>>, TRInt*>(this);
    test_insert_iterator_range_impl<vector<TMInt, fancy_allocator<TMInt>>, TMInt*>(this);
}
