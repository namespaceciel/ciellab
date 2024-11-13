#include <gtest/gtest.h>

#include <ciel/inplace_vector.hpp>
#include <ciel/test/int_wrapper.hpp>

#include <initializer_list>
#include <type_traits>

using namespace ciel;

namespace {

template<class C>
void test_emplace_lvalue_impl(::testing::Test*) {
    using T = typename C::value_type;

    const T value(5);

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

TEST(inplace_vector, emplace_lvalue) {
    test_emplace_lvalue_impl<inplace_vector<int, 16>>(this);
    test_emplace_lvalue_impl<inplace_vector<Int, 16>>(this);
    test_emplace_lvalue_impl<inplace_vector<TRInt, 16>>(this);
    test_emplace_lvalue_impl<inplace_vector<TMInt, 16>>(this);
}

TEST(inplace_vector, emplace_self_lvalue) {
    test_emplace_self_lvalue_impl<inplace_vector<int, 16>>(this);
    test_emplace_self_lvalue_impl<inplace_vector<Int, 16>>(this);
    test_emplace_self_lvalue_impl<inplace_vector<TRInt, 16>>(this);
    test_emplace_self_lvalue_impl<inplace_vector<TMInt, 16>>(this);
}

TEST(inplace_vector, emplace_rvalue) {
    test_emplace_rvalue_impl<inplace_vector<int, 16>>(this);
    test_emplace_rvalue_impl<inplace_vector<Int, 16>>(this);
    test_emplace_rvalue_impl<inplace_vector<TRInt, 16>>(this);
    test_emplace_rvalue_impl<inplace_vector<TMInt, 16>>(this);
}

TEST(inplace_vector, emplace_self_rvalue) {
    test_emplace_self_rvalue_impl<inplace_vector<int, 16>>(this);
    test_emplace_self_rvalue_impl<inplace_vector<Int, 16>>(this);
    test_emplace_self_rvalue_impl<inplace_vector<TRInt, 16>>(this);
    test_emplace_self_rvalue_impl<inplace_vector<TMInt, 16>>(this);
}

TEST(inplace_vector, emplace_parameter) {
    inplace_vector<inplace_vector<int, 16>, 16> v;
    v.emplace(v.end(), 4, 23);
    ASSERT_EQ(v.size(), 1);
    ASSERT_EQ(v[0], std::initializer_list<int>({23, 23, 23, 23}));
}

TEST(inplace_vector, emplace_il) {
    inplace_vector<inplace_vector<int, 16>, 16> v;
    v.emplace(v.end(), {23, 23, 23, 23});
    ASSERT_EQ(v.size(), 1);
    ASSERT_EQ(v[0], std::initializer_list<int>({23, 23, 23, 23}));
}
