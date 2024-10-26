#include <gtest/gtest.h>

#include <ciel/split_buffer.hpp>

using namespace ciel;

TEST(split_buffer, constructors) {
    const split_buffer<int> v1;
    ASSERT_TRUE(v1.empty());
    ASSERT_EQ(v1.size(), 0);

    const split_buffer<int> v2(v1);
    ASSERT_TRUE(v2.empty());

    const split_buffer<int> v3(10, 20);
    ASSERT_EQ(v3.size(), 10);

    const split_buffer<int> v4(15);
    ASSERT_EQ(v4.size(), 15);

    split_buffer<int> v5(v4);
    ASSERT_EQ(v5.size(), 15);

    const split_buffer<int> v6(std::move(v5));
    ASSERT_EQ(v5.size(), 0);
    ASSERT_EQ(v6.size(), 15);

    const split_buffer<int> v7({1, 2, 3, 4, 5});
    ASSERT_EQ(v7.size(), 5);

    const split_buffer<int> v8(0, 10);
    ASSERT_TRUE(v8.empty());

    const split_buffer<int> v9(0);
    ASSERT_TRUE(v9.empty());

    const split_buffer<int> v10(v7.begin(), v7.begin());
    ASSERT_TRUE(v10.empty());
}

TEST(split_buffer, assignments) {
    split_buffer<int> v1({1, 2, 3, 4, 5});
    split_buffer<int> v2{};

    v2 = std::move(v1);
    ASSERT_TRUE(v1.empty());
    {
        std::initializer_list<int> il{1, 2, 3, 4, 5};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v2.begin()));
    }

    split_buffer<int> v3{};
    v3 = v2;
    ASSERT_EQ(v2, v3);

    v3.shrink_to_fit();
    ASSERT_EQ(v3.size(), v3.capacity());

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

    // lend space from other side
    v3.shrink_to_fit();
    v3.reserve_front_spare(4);
    v3.assign(4, 10);
    {
        std::initializer_list<int> il{10, 10, 10, 10};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v3.begin()));
    }

    // collect both sides' space
    v3.shrink_to_fit();

    v3.reserve_front_spare(4);
    v3.reserve_back_spare(2); // will lend 2 from front spare

    v3.assign(7, 10);
    {
        std::initializer_list<int> il{10, 10, 10, 10, 10, 10, 10};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v3.begin()));
    }
}

TEST(split_buffer, at) {
    const split_buffer<size_t> v1({0, 1, 2, 3, 4, 5});
    for (size_t i = 0; i < v1.size(); ++i) {
        ASSERT_EQ(v1[i], i);
    }

    ASSERT_EQ(v1.front(), 0);
    ASSERT_EQ(v1.back(), 5);

#ifdef CIEL_HAS_EXCEPTIONS
    ASSERT_THROW(CIEL_UNUSED(v1.at(-1)), std::out_of_range);
#endif
}

TEST(split_buffer, push_and_pop) {
    // empty
    split_buffer<int> v1;
    ASSERT_EQ(v1.emplace_back(0), 0);

    v1.push_back(1);
    ASSERT_EQ(v1.emplace_back(2), 2);
    {
        std::initializer_list<int> il{0, 1, 2};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    ASSERT_EQ(v1.emplace_front(3), 3);
    {
        std::initializer_list<int> il{3, 0, 1, 2};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    v1.push_front(4);
    {
        std::initializer_list<int> il{4, 3, 0, 1, 2};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    split_buffer<int> v2({0, 1, 2, 3, 4});
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

    v2.pop_front();
    ASSERT_EQ(v2.front(), 1);
}

TEST(split_buffer, resize) {
    split_buffer<int> v1(10, 5);
    ASSERT_EQ(v1.size(), 10);
    for (const int i : v1) {
        ASSERT_EQ(i, 5);
    }

    // shrink
    v1.resize(1);
    ASSERT_EQ(v1.size(), 1);
    ASSERT_EQ(v1.front(), 5);

    // enlarge but not beyond capacity
    v1.reserve_back_spare(9);
    v1.resize(10, 77);
    {
        std::initializer_list<int> il{5, 77, 77, 77, 77, 77, 77, 77, 77, 77};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    // enlarge beyond capacity
    v1.shrink_to_fit();
    v1.resize(12, 44);
    {
        std::initializer_list<int> il{5, 77, 77, 77, 77, 77, 77, 77, 77, 77, 44, 44};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    // lend space from other side
    v1.shrink_to_fit();
    v1.reserve_front_spare(4);
    v1.resize(15, 10);
    {
        std::initializer_list<int> il{5, 77, 77, 77, 77, 77, 77, 77, 77, 77, 44, 44, 10, 10, 10};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }

    // collect both sides' space
    v1.shrink_to_fit();

    v1.reserve_front_spare(4);
    v1.reserve_back_spare(2); // will lend 2 from front spare

    v1.resize(18, 19);
    {
        std::initializer_list<int> il{5, 77, 77, 77, 77, 77, 77, 77, 77, 77, 44, 44, 10, 10, 10, 19, 19, 19};
        ASSERT_TRUE(std::equal(il.begin(), il.end(), v1.begin()));
    }
}
