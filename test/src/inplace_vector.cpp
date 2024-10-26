#include <gtest/gtest.h>

#include <ciel/inplace_vector.hpp>

using namespace ciel;

namespace {

struct Trivial {};

struct NotTrivial {
    NotTrivial() noexcept = default;

    NotTrivial(const NotTrivial&) noexcept {}

    NotTrivial(NotTrivial&&) noexcept {}

    NotTrivial&
    operator=(const NotTrivial&) noexcept {
        return *this;
    }

    NotTrivial&
    operator=(NotTrivial&&) noexcept {
        return *this;
    }

    ~NotTrivial() {}
};

} // namespace

TEST(inplace_vector, constructors) {
    const inplace_vector<int, 8> v1;
    ASSERT_TRUE(v1.empty());
    ASSERT_EQ(v1.size(), 0);
    ASSERT_EQ(v1.capacity(), 8);

    const inplace_vector<int, 8> v2(v1);
    ASSERT_TRUE(v2.empty());

    const inplace_vector<int, 10> v3(10, 20);
    ASSERT_EQ(v3.size(), 10);

    const inplace_vector<int, 15> v4(15);
    ASSERT_EQ(v4.size(), 15);

    inplace_vector<int, 15> v5(v4);
    ASSERT_EQ(v5.size(), 15);

    const inplace_vector<int, 15> v6(std::move(v5));
    ASSERT_EQ(v6.size(), 15);

    const inplace_vector<int, 5> v7({1, 2, 3, 4, 5});
    ASSERT_EQ(v7.size(), 5);

    const inplace_vector<int, 8> v8(0, 10);
    ASSERT_TRUE(v8.empty());

    const inplace_vector<int, 8> v9(0);
    ASSERT_TRUE(v9.empty());

    const inplace_vector<int, 8> v10(v7.begin(), v7.begin());
    ASSERT_TRUE(v10.empty());
}

TEST(inplace_vector, assignments) {
    inplace_vector<int, 10> v1({1, 2, 3, 4, 5});
    inplace_vector<int, 10> v2{};

    v2 = std::move(v1);
    {
        std::initializer_list<int> il{1, 2, 3, 4, 5};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v2.begin()));
    }

    inplace_vector<int, 10> v3{};
    v3 = v2;
    ASSERT_EQ(v2, v3);

    // expansion
    v3 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    {
        std::initializer_list<int> il{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v3.begin()));
    }

    // shrink
    v3.assign(2, 10);
    {
        std::initializer_list<int> il{10, 10};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v3.begin()));
    }
}

TEST(inplace_vector, at) {
    const inplace_vector<size_t, 6> v1({0, 1, 2, 3, 4, 5});
    for (size_t i = 0; i < v1.size(); ++i) {
        ASSERT_EQ(v1[i], i);
    }

    ASSERT_EQ(v1.front(), 0);
    ASSERT_EQ(v1.back(), 5);

#ifdef CIEL_HAS_EXCEPTIONS
    ASSERT_THROW(CIEL_UNUSED(v1.at(-1)), std::out_of_range);
#endif
}

TEST(inplace_vector, push_and_pop) {
    // empty
    inplace_vector<int, 8> v1;
    ASSERT_EQ(v1.emplace_back(0), 0);

    v1.push_back(1);
    ASSERT_EQ(v1.emplace_back(2), 2);
    {
        std::initializer_list<int> il{0, 1, 2};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    inplace_vector<int, 16> v2({0, 1, 2, 3, 4});
    ASSERT_EQ(v2.emplace_back(5), 5);

    ASSERT_EQ(v2.emplace_back(6), 6);
    {
        std::initializer_list<int> il{0, 1, 2, 3, 4, 5, 6};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v2.begin()));
    }

    ASSERT_EQ(v2.emplace_back(7), 7);
    ASSERT_EQ(v2.back(), 7);

    v2.pop_back();
    v2.pop_back();
    ASSERT_EQ(v2.back(), 5);

    v2.push_back(v2[2]);
    ASSERT_EQ(v2.back(), 2);
}

TEST(inplace_vector, resize) {
    inplace_vector<int, 16> v1(10, 5);
    ASSERT_EQ(v1.size(), 10);
    for (const int i : v1) {
        ASSERT_EQ(i, 5);
    }

    // shrink
    v1.resize(1);
    ASSERT_EQ(v1.size(), 1);
    ASSERT_EQ(v1.front(), 5);

    v1.resize(10, 77);
    {
        std::initializer_list<int> il{5, 77, 77, 77, 77, 77, 77, 77, 77, 77};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    v1.resize(12, 44);
    {
        std::initializer_list<int> il{5, 77, 77, 77, 77, 77, 77, 77, 77, 77, 44, 44};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }
}

TEST(inplace_vector, emplace_il) {
    inplace_vector<inplace_vector<int, 8>, 8> v;

    v.emplace_back({1, 2});

    v.unchecked_emplace_back({5, 6});

    // v.emplace_back({}); // error: we can't deduce type for this.

    ASSERT_EQ(v.size(), 2);
    using inplace_vector_int_8 = inplace_vector<int, 8>;
    ASSERT_EQ(v[0], inplace_vector_int_8({1, 2}));
    ASSERT_EQ(v[1], inplace_vector_int_8({5, 6}));
}

TEST(inplace_vector, trivial) {
    static_assert(std::is_trivially_copy_constructible<inplace_vector<Trivial, 8>>::value, "");
    static_assert(std::is_trivially_move_constructible<inplace_vector<Trivial, 8>>::value, "");
    static_assert(std::is_trivially_copy_assignable<inplace_vector<Trivial, 8>>::value, "");
    static_assert(std::is_trivially_move_assignable<inplace_vector<Trivial, 8>>::value, "");
    static_assert(std::is_trivially_destructible<inplace_vector<Trivial, 8>>::value, "");

    static_assert(not std::is_trivially_copy_constructible<inplace_vector<NotTrivial, 8>>::value, "");
    static_assert(not std::is_trivially_move_constructible<inplace_vector<NotTrivial, 8>>::value, "");
    static_assert(not std::is_trivially_copy_assignable<inplace_vector<NotTrivial, 8>>::value, "");
    static_assert(not std::is_trivially_move_assignable<inplace_vector<NotTrivial, 8>>::value, "");
    static_assert(not std::is_trivially_destructible<inplace_vector<NotTrivial, 8>>::value, "");
}
