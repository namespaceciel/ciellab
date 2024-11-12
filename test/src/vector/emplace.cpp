#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

namespace {

template<class C>
void test_emplace_lvalue_impl(::testing::Test*) {
    using T = typename C::value_type;

    const T value(5);

    // expansion
    {
        C v{0, 1, 2, 3, 4};
        v.resize(v.capacity());

        v.emplace(v.begin() + 2, value);
        v.resize(6);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 5, 2, 3, 4}));
    }
    // emplace not at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.emplace(v.begin() + 1, value);
        ASSERT_EQ(v, std::initializer_list<T>({0, 5, 1, 2, 3, 4}));
    }
    // emplace at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.emplace(v.end(), value);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 5}));
    }
}

template<class C>
void test_emplace_self_lvalue_impl(::testing::Test*) {
    using T = typename C::value_type;

    // expansion
    {
        C v{0, 1, 2, 3, 4};
        v.resize(v.capacity());

        v.emplace(v.begin() + 2, v[1]);
        v.resize(6);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 1, 2, 3, 4}));
    }
    // emplace not at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.emplace(v.begin() + 1, v.front());
        ASSERT_EQ(v, std::initializer_list<T>({0, 0, 1, 2, 3, 4}));
    }
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.emplace(v.begin() + 1, v.back());
        ASSERT_EQ(v, std::initializer_list<T>({0, 4, 1, 2, 3, 4}));
    }
    // emplace at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.emplace(v.end(), v[1]);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 1}));
    }
}

template<class C>
void test_emplace_rvalue_impl(::testing::Test*) {
    using T = typename C::value_type;

    // expansion
    {
        C v{0, 1, 2, 3, 4};
        v.resize(v.capacity());

        v.emplace(v.begin() + 2, T{5});
        v.resize(6);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 5, 2, 3, 4}));
    }
    // emplace not at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.emplace(v.begin() + 1, T{5});
        ASSERT_EQ(v, std::initializer_list<T>({0, 5, 1, 2, 3, 4}));
    }
    // emplace at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.emplace(v.end(), T{5});
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 5}));
    }
}

template<class C>
void test_emplace_self_rvalue_impl(::testing::Test*) {
    using T = typename C::value_type;

    // expansion
    {
        C v{0, 1, 2, 3, 4};
        v.resize(v.capacity());

        v.emplace(v.begin() + 2, std::move(v[1]));
        v.resize(6);
        if (!std::is_trivial<T>::value) {
            ASSERT_EQ(v, std::initializer_list<T>({0, -1, 1, 2, 3, 4}));
        }
    }
    // emplace not at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.emplace(v.begin() + 1, std::move(v.front()));
        if (!std::is_trivial<T>::value) {
            ASSERT_EQ(v, std::initializer_list<T>({-1, 0, 1, 2, 3, 4}));
        }
    }
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.emplace(v.begin() + 1, std::move(v.back()));
        if (!std::is_trivial<T>::value) {
            ASSERT_EQ(v, std::initializer_list<T>({0, 4, 1, 2, 3, -1}));
        }
    }
    // emplace at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.emplace(v.end(), std::move(v[1]));
        if (!std::is_trivial<T>::value) {
            ASSERT_EQ(v, std::initializer_list<T>({0, -1, 2, 3, 4, 1}));
        }
    }
}

} // namespace

TEST(vector, emplace_lvalue) {
    test_emplace_lvalue_impl<vector<int>>(this);
    test_emplace_lvalue_impl<vector<Int>>(this);
    test_emplace_lvalue_impl<vector<TRInt>>(this);
    test_emplace_lvalue_impl<vector<TMInt>>(this);

    test_emplace_lvalue_impl<vector<int, fancy_allocator<int>>>(this);
    test_emplace_lvalue_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_emplace_lvalue_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_emplace_lvalue_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, emplace_self_lvalue) {
    test_emplace_self_lvalue_impl<vector<int>>(this);
    test_emplace_self_lvalue_impl<vector<Int>>(this);
    test_emplace_self_lvalue_impl<vector<TRInt>>(this);
    test_emplace_self_lvalue_impl<vector<TMInt>>(this);

    test_emplace_self_lvalue_impl<vector<int, fancy_allocator<int>>>(this);
    test_emplace_self_lvalue_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_emplace_self_lvalue_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_emplace_self_lvalue_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, emplace_rvalue) {
    test_emplace_rvalue_impl<vector<int>>(this);
    test_emplace_rvalue_impl<vector<Int>>(this);
    test_emplace_rvalue_impl<vector<TRInt>>(this);
    test_emplace_rvalue_impl<vector<TMInt>>(this);

    test_emplace_rvalue_impl<vector<int, fancy_allocator<int>>>(this);
    test_emplace_rvalue_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_emplace_rvalue_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_emplace_rvalue_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, emplace_self_rvalue) {
    test_emplace_self_rvalue_impl<vector<int>>(this);
    test_emplace_self_rvalue_impl<vector<Int>>(this);
    test_emplace_self_rvalue_impl<vector<TRInt>>(this);
    test_emplace_self_rvalue_impl<vector<TMInt>>(this);

    test_emplace_self_rvalue_impl<vector<int, fancy_allocator<int>>>(this);
    test_emplace_self_rvalue_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_emplace_self_rvalue_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_emplace_self_rvalue_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, emplace_parameter) {
    vector<vector<int>> v;
    v.emplace(v.end(), 4, 23);
    ASSERT_EQ(v.size(), 1);
    ASSERT_EQ(v[0], std::initializer_list<int>({23, 23, 23, 23}));
}

TEST(vector, emplace_il) {
    vector<vector<int>> v;
    v.emplace(v.end(), {23, 23, 23, 23});
    ASSERT_EQ(v.size(), 1);
    ASSERT_EQ(v[0], std::initializer_list<int>({23, 23, 23, 23}));
}
