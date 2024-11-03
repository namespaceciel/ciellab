#include <gtest/gtest.h>

#include <ciel/test/int_wrapper.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, assignments) {
    vector<Int> v1({1, 2, 3, 4, 5});
    vector<Int> v2{};

    v2 = std::move(v1);
    ASSERT_TRUE(v1.empty());
    {
        std::initializer_list<Int> il{1, 2, 3, 4, 5};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v2.begin()));
    }

    vector<Int> v3{};
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

TEST(vector, at) {
    const vector<size_t> v1({0, 1, 2, 3, 4, 5});
    for (size_t i = 0; i < v1.size(); ++i) {
        ASSERT_EQ(v1[i], i);
    }

    ASSERT_EQ(v1.front(), 0);
    ASSERT_EQ(v1.back(), 5);

#ifdef CIEL_HAS_EXCEPTIONS
    ASSERT_THROW(CIEL_UNUSED(v1.at(-1)), std::out_of_range);
#endif
}

TEST(vector, insert_and_emplace) {
    vector<Int> v1{0, 1, 2, 3, 4, 5, 6};

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

TEST(vector, erase) {
    vector<Int> v1{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

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

TEST(vector, vector_bool) {
    std::initializer_list<bool> il{true, false, false, true, true};
    vector<bool> v{true, false, false, true, true};

    ASSERT_TRUE(std::equal(il.begin(), il.end(), v.begin()));
}

TEST(vector, insert_self_reference) {
    {
        vector<Int> v{0, 1, 2, 3, 4};
        ASSERT_EQ(v.size(), v.capacity());

        v.insert(v.begin() + 1, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({0, 2, 1, 2, 3, 4}));
    }
    {
        vector<Int> v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({0, 2, 1, 2, 3, 4}));
    }
    {
        vector<Int> v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, std::move(v[2]));
        ASSERT_EQ(v, std::initializer_list<Int>({0, 2, 1, -1, 3, 4}));
    }
    {
        vector<Int> v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin() + 1, 5, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({0, 2, 2, 2, 2, 2, 1, 2, 3, 4}));
    }
}

TEST(vector, assign_self_reference) {
    {
        vector<Int> v{0, 1, 2, 3, 4};
        ASSERT_EQ(v.size(), v.capacity());

        v.assign(2, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({2, 2}));
    }
    {
        vector<Int> v{0, 1, 2, 3, 4};
        ASSERT_EQ(v.size(), v.capacity());

        v.assign(6, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({2, 2, 2, 2, 2, 2}));
    }
    {
        vector<Int> v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.assign(6, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({2, 2, 2, 2, 2, 2}));
    }
}

TEST(vector, resize_self_reference) {
    {
        vector<Int> v{0, 1, 2, 3, 4};
        ASSERT_EQ(v.size(), v.capacity());

        v.resize(6, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4, 2}));
    }
}
