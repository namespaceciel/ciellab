#include "tools.h"
#include <gtest/gtest.h>

#include <ciel/vector.hpp>

TEST(vector_tests, constructors) {
    const ciel::vector<Int> v1;
    ASSERT_TRUE(v1.empty());
    ASSERT_EQ(v1.size(), 0);
    ASSERT_EQ(v1.capacity(), 0);

    const ciel::vector<Int> v2(v1);
    ASSERT_TRUE(v2.empty());

    const ciel::vector<Int> v3(10, 20);
    ASSERT_EQ(v3.size(), 10);

    const ciel::vector<Int> v4(15);
    ASSERT_EQ(v4.size(), 15);

    ciel::vector<Int> v5(v4);
    ASSERT_EQ(v5.size(), 15);

    const ciel::vector<Int> v6(std::move(v5));
    ASSERT_EQ(v5.size(), 0);
    ASSERT_EQ(v6.size(), 15);

    const ciel::vector<Int> v7({1, 2, 3, 4, 5});
    ASSERT_EQ(v7.size(), 5);

    const ciel::vector<Int> v8(0, 10);
    ASSERT_TRUE(v8.empty());

    const ciel::vector<Int> v9(0);
    ASSERT_TRUE(v9.empty());

    const ciel::vector<Int> v10(v7.begin(), v7.begin());
    ASSERT_TRUE(v10.empty());
}

TEST(vector_tests, assignments) {
    ciel::vector<Int> v1({1, 2, 3, 4, 5});
    ciel::vector<Int> v2{};

    v2 = std::move(v1);
    ASSERT_TRUE(v1.empty());
    {
        std::initializer_list<Int> il{1, 2, 3, 4, 5};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v2.begin()));
    }

    ciel::vector<Int> v3{};
    v3 = v2;
    ASSERT_EQ(v2, v3);

    v3.shrink_to_fit();
    ASSERT_EQ(v3.size(), v3.capacity());

    // expansion
    v3 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    {
        std::initializer_list<Int> il{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v3.begin()));
    }

    // shrink
    v3.assign(2, 10);
    {
        std::initializer_list<Int> il{10, 10};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v3.begin()));
    }
}

TEST(vector_tests, at) {
    const ciel::vector<size_t> v1({0, 1, 2, 3, 4, 5});
    for (size_t i = 0; i < v1.size(); ++i) {
        ASSERT_EQ(v1[i], i);
    }

    ASSERT_EQ(v1.front(), 0);
    ASSERT_EQ(v1.back(), 5);

#ifdef CIEL_HAS_EXCEPTIONS
    ASSERT_THROW(CIEL_UNUSED(v1.at(-1)), std::out_of_range);
#endif
}

TEST(vector_tests, push_and_pop) {
    // empty
    ciel::vector<Int> v1;
    ASSERT_EQ(v1.emplace_back(0), 0);

    v1.push_back(1);
    ASSERT_EQ(v1.emplace_back(2), 2);
    {
        std::initializer_list<Int> il{0, 1, 2};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    ciel::vector<Int> v2({0, 1, 2, 3, 4});
    ASSERT_EQ(v2.emplace_back(5), 5);

    v2.shrink_to_fit();
    ASSERT_EQ(v2.emplace_back(6), 6);
    {
        std::initializer_list<Int> il{0, 1, 2, 3, 4, 5, 6};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v2.begin()));
    }

    v2.shrink_to_fit();
    v2.reserve(100);
    ASSERT_EQ(v2.emplace_back(7), 7);
    ASSERT_EQ(v2.back(), 7);

    v2.pop_back();
    v2.pop_back();
    ASSERT_EQ(v2.back(), 5);

    // self assignment when expansion
    v2.shrink_to_fit();
    v2.push_back(v2[2]);
    ASSERT_EQ(v2.back(), 2);
}

TEST(vector_tests, resize) {
    ciel::vector<Int> v1(10, 5);
    ASSERT_EQ(v1.size(), 10);
    for (const Int& i : v1) {
        ASSERT_EQ(i, 5);
    }

    // shrink
    v1.resize(1);
    ASSERT_EQ(v1.size(), 1);
    ASSERT_EQ(v1.front(), 5);

    // enlarge but not beyond capacity
    v1.reserve(100);
    v1.resize(10, 77);
    {
        std::initializer_list<Int> il{5, 77, 77, 77, 77, 77, 77, 77, 77, 77};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    // enlarge beyond capacity
    v1.shrink_to_fit();
    v1.resize(12, 44);
    {
        std::initializer_list<Int> il{5, 77, 77, 77, 77, 77, 77, 77, 77, 77, 44, 44};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }
}

TEST(vector_tests, insert_and_emplace) {
    ciel::vector<Int> v1{0, 1, 2, 3, 4, 5, 6};

    // insert at front
    ASSERT_EQ(*v1.insert(v1.begin(), 21), 21);
    ASSERT_EQ(*v1.emplace(v1.begin(), 22), 22);

    {
        std::initializer_list<Int> il{22, 21, 0, 1, 2, 3, 4, 5, 6};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    // insert at back
    ASSERT_EQ(*v1.insert(v1.end(), 31), 31);
    ASSERT_EQ(*v1.emplace(v1.end(), 32), 32);

    // insert at mid
    ASSERT_EQ(*v1.insert(v1.begin() + 5, 2, 41), 41);

    {
        std::initializer_list<Int> il{22, 21, 0, 1, 2, 41, 41, 3, 4, 5, 6, 31, 32};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    ASSERT_EQ(*v1.insert(v1.begin() + 8, {42, 43}), 42);

    {
        std::initializer_list<Int> il{22, 21, 0, 1, 2, 41, 41, 3, 42, 43, 4, 5, 6, 31, 32};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    // insert empty range
    ASSERT_EQ(*v1.insert(v1.begin(), v1.begin(), v1.begin()), 22);

    {
        std::initializer_list<Int> il{22, 21, 0, 1, 2, 41, 41, 3, 42, 43, 4, 5, 6, 31, 32};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    // insert when expansion
    v1.shrink_to_fit();
    ASSERT_EQ(*v1.insert(v1.begin() + 2, 99), 99);
    {
        std::initializer_list<Int> il{22, 21, 99, 0, 1, 2, 41, 41, 3, 42, 43, 4, 5, 6, 31, 32};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    // insert self range when expansion
    v1.shrink_to_fit();
    ASSERT_EQ(*v1.insert(v1.begin() + 2, v1.begin() + 1, v1.begin() + 5), 21);
    {
        std::initializer_list<Int> il{22, 21, 21, 99, 0, 1, 99, 0, 1, 2, 41, 41, 3, 42, 43, 4, 5, 6, 31, 32};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }
}

TEST(vector_tests, erase) {
    ciel::vector<Int> v1{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    ASSERT_EQ(*v1.erase(v1.begin()), 1);
    {
        std::initializer_list<Int> il{1, 2, 3, 4, 5, 6, 7, 8, 9};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    ASSERT_EQ(*v1.erase(v1.begin() + 2, v1.begin() + 4), 5);
    {
        std::initializer_list<Int> il{1, 2, 5, 6, 7, 8, 9};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    // You can't ensure this eval_order, v1.end() may be calculated before erasing
    // ASSERT_EQ(v1.erase(v1.end() - 1), v1.end());

    auto res = v1.erase(v1.end() - 1);
    ASSERT_EQ(res, v1.end());
    {
        std::initializer_list<Int> il{1, 2, 5, 6, 7, 8};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    res = v1.erase(v1.end() - 2, v1.end());
    ASSERT_EQ(res, v1.end());
    {
        std::initializer_list<Int> il{1, 2, 5, 6};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }
}

TEST(vector_tests, copy_and_move_behavior) {
    ConstructAndAssignCounter::reset();

    const ciel::vector<ConstructAndAssignCounter> v1(5);
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 0);
    ASSERT_EQ(ConstructAndAssignCounter::move(), 0);

    ciel::vector<ConstructAndAssignCounter> v2(6, ConstructAndAssignCounter{});
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 6);
    ASSERT_EQ(ConstructAndAssignCounter::move(), 0);

    const ciel::vector<ConstructAndAssignCounter> v3 = v1;
    const ciel::vector<ConstructAndAssignCounter> v4 = std::move(v2);
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 5);
    ASSERT_EQ(ConstructAndAssignCounter::move(), 0);

    const ciel::vector<ConstructAndAssignCounter> v5(v1.begin(), v1.end() - 1);
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 4);
    ASSERT_EQ(ConstructAndAssignCounter::move(), 0);

    ciel::vector<ConstructAndAssignCounter> v6({{}, {}, {}});
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 3);

    v6.reserve(100);
    ConstructAndAssignCounter::reset();

    v6 = {{}, {}, {}, {}};
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 4);

    v6.assign(7, {});
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 7);

    v6.assign(v1.begin(), v1.end());
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 5);

    v6.assign({{}, {}, {}, {}});
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 4);
}

TEST(vector_tests, copy_and_move_behavior2) {
    ciel::vector<ConstructAndAssignCounter> v1;
    v1.reserve(50);
    ConstructAndAssignCounter::reset();

    for (size_t i = 0; i < 10; ++i) {
        v1.emplace_back();
    }
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 0);
    ASSERT_EQ(ConstructAndAssignCounter::move(), 0);

    for (size_t i = 0; i < 10; ++i) {
        v1.push_back({});
    }
    ASSERT_EQ(ConstructAndAssignCounter::move(), 10);

    ConstructAndAssignCounter tmp;

    for (size_t i = 0; i < 10; ++i) {
        v1.push_back(std::move(tmp));
    }
    ASSERT_EQ(ConstructAndAssignCounter::move(), 10);

    for (size_t i = 0; i < 10; ++i) {
        v1.push_back(tmp);
    }
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 10);

    ASSERT_EQ(v1.size(), 40);

    v1.reserve(100);
    ASSERT_EQ(ConstructAndAssignCounter::move(), 40);

    v1.shrink_to_fit();
    ASSERT_EQ(ConstructAndAssignCounter::move(), 40);
}

TEST(vector_tests, copy_and_move_behavior3) {
    ConstructAndAssignCounter::reset();

    ciel::vector<ConstructAndAssignCounter> v1(10);
    v1.erase(v1.begin());
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 0);

    v1.erase(v1.begin() + 5, v1.begin() + 7);
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 0);

    v1.insert(v1.begin(), ConstructAndAssignCounter{});
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 0);

    constexpr ConstructAndAssignCounter tmp;
    v1.insert(v1.begin(), tmp);
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 1);

    v1.insert(v1.begin(), 3, {});
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 3);

    v1.insert(v1.begin(), {{}, {}});
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 2);

    v1.shrink_to_fit(); // capacity turns to 14
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 0);

    v1.insert(v1.end() - 2, v1.begin(), v1.begin() + 2);
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 2);
}

TEST(vector_tests, vector_bool) {
    std::initializer_list<bool> il{true, false, false, true, true};
    ciel::vector<bool> v{true, false, false, true, true};

    ASSERT_TRUE(std::equal(il.begin(), il.end(), v.begin()));
}

TEST(vector_tests, emplace_il) {
    ciel::vector<ciel::vector<int>> v;

    v.emplace_back({1, 2});

    v.emplace(v.end(), {3, 4});

    v.reserve(3);
    v.unchecked_emplace_back({5, 6});

    // v.emplace_back({}); // error: we can't deduce type for this.

    ASSERT_EQ(v.size(), 3);
    ASSERT_EQ(v[0], ciel::vector<int>({1, 2}));
    ASSERT_EQ(v[1], ciel::vector<int>({3, 4}));
    ASSERT_EQ(v[2], ciel::vector<int>({5, 6}));
}

/* FIXME
TEST(vector_tests, insert_self_reference) {
    {
        ciel::vector<Int> v{0, 1, 2, 3, 4};
        ASSERT_EQ(v.size(), v.capacity());

        v.insert(v.begin() + 1, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({0, 2, 1, 2, 3, 4}));
    }
    {
        ciel::vector<Int> v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({0, 2, 1, 2, 3, 4}));
    }
    {
        ciel::vector<Int> v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, 5, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({0, 2, 2, 2, 2, 2, 1, 2, 3, 4}));
    }
}
*/
