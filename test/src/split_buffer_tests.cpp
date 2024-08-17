#include "tools.h"
#include <gtest/gtest.h>

#include <ciel/split_buffer.hpp>

TEST(split_buffer_tests, constructors) {
    const ciel::split_buffer<int> v1;
    ASSERT_TRUE(v1.empty());
    ASSERT_EQ(v1.size(), 0);

    const ciel::split_buffer<int> v2(v1);
    ASSERT_TRUE(v2.empty());

    const ciel::split_buffer<int> v3(10, 20);
    ASSERT_EQ(v3.size(), 10);

    const ciel::split_buffer<int> v4(15);
    ASSERT_EQ(v4.size(), 15);

    ciel::split_buffer<int> v5(v4);
    ASSERT_EQ(v5.size(), 15);

    const ciel::split_buffer<int> v6(std::move(v5));
    ASSERT_EQ(v5.size(), 0);
    ASSERT_EQ(v6.size(), 15);

    const ciel::split_buffer<int> v7({1, 2, 3, 4, 5});
    ASSERT_EQ(v7.size(), 5);

    const ciel::split_buffer<int> v8(0, 10);
    ASSERT_TRUE(v8.empty());

    const ciel::split_buffer<int> v9(0);
    ASSERT_TRUE(v9.empty());

    const ciel::split_buffer<int> v10(v7.begin(), v7.begin());
    ASSERT_TRUE(v10.empty());
}

TEST(split_buffer_tests, assignments) {
    ciel::split_buffer<int> v1({1, 2, 3, 4, 5});
    ciel::split_buffer<int> v2{};

    v2 = std::move(v1);
    ASSERT_TRUE(v1.empty());
    ASSERT_EQ(v2, std::initializer_list<int>({1, 2, 3, 4, 5}));

    ciel::split_buffer<int> v3{};
    v3 = v2;
    ASSERT_EQ(v2, v3);

    v3.shrink_to_fit();
    ASSERT_EQ(v3.size(), v3.capacity());

    // expansion
    v3 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    ASSERT_EQ(v3, std::initializer_list<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));

    // shrink
    v3.assign(2, 10);
    ASSERT_EQ(v3, std::initializer_list<int>({10, 10}));

    // lend space from other side
    v3.shrink_to_fit();
    v3.reserve_front_spare(4);
    v3.assign(4, 10);
    ASSERT_EQ(v3, std::initializer_list<int>({10, 10, 10, 10}));

    // collect both sides' space
    v3.shrink_to_fit();

    v3.reserve_front_spare(4);
    v3.reserve_back_spare(2); // will lend 2 from front spare

    v3.assign(7, 10);
    ASSERT_EQ(v3, std::initializer_list<int>({10, 10, 10, 10, 10, 10, 10}));
}

TEST(split_buffer_tests, at) {
    const ciel::split_buffer<size_t> v1({0, 1, 2, 3, 4, 5});
    for (size_t i = 0; i < v1.size(); ++i) {
        ASSERT_EQ(v1[i], i);
    }

    ASSERT_EQ(v1.front(), 0);
    ASSERT_EQ(v1.back(), 5);

#ifdef CIEL_HAS_EXCEPTIONS
    ASSERT_THROW(CIEL_UNUSED(v1.at(-1)), std::out_of_range);
#endif
}

TEST(split_buffer_tests, push_and_pop) {
    // empty
    ciel::split_buffer<int> v1;
    ASSERT_EQ(v1.emplace_back(0), 0);

    v1.push_back(1);
    ASSERT_EQ(v1.emplace_back(2), 2);
    ASSERT_EQ(v1, std::initializer_list<int>({0, 1, 2}));

    ASSERT_EQ(v1.emplace_front(3), 3);
    ASSERT_EQ(v1, std::initializer_list<int>({3, 0, 1, 2}));

    v1.push_front(4);
    ASSERT_EQ(v1, std::initializer_list<int>({4, 3, 0, 1, 2}));

    ciel::split_buffer<int> v2({0, 1, 2, 3, 4});
    ASSERT_EQ(v2.emplace_back(5), 5);

    ASSERT_EQ(v2.emplace_back(6), 6);
    ASSERT_EQ(v2, std::initializer_list<int>({0, 1, 2, 3, 4, 5, 6}));

    ASSERT_EQ(v2.emplace_back(7), 7);
    ASSERT_EQ(v2.back(), 7);

    v2.pop_back();
    v2.pop_back();
    ASSERT_EQ(v2.back(), 5);

    v2.pop_front();
    ASSERT_EQ(v2.front(), 1);
}

TEST(split_buffer_tests, resize) {
    ciel::split_buffer<int> v1(10, 5);
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
    ASSERT_EQ(v1, std::initializer_list<int>({5, 77, 77, 77, 77, 77, 77, 77, 77, 77}));

    // enlarge beyond capacity
    v1.shrink_to_fit();
    v1.resize(12, 44);
    ASSERT_EQ(v1, std::initializer_list<int>({5, 77, 77, 77, 77, 77, 77, 77, 77, 77, 44, 44}));

    // lend space from other side
    v1.shrink_to_fit();
    v1.reserve_front_spare(4);
    v1.resize(15, 10);
    ASSERT_EQ(v1, std::initializer_list<int>({5, 77, 77, 77, 77, 77, 77, 77, 77, 77, 44, 44, 10, 10, 10}));

    // collect both sides' space
    v1.shrink_to_fit();

    v1.reserve_front_spare(4);
    v1.reserve_back_spare(2); // will lend 2 from front spare

    v1.resize(18, 19);
    ASSERT_EQ(v1, std::initializer_list<int>({5, 77, 77, 77, 77, 77, 77, 77, 77, 77, 44, 44, 10, 10, 10, 19, 19, 19}));
}

TEST(split_buffer_tests, copy_and_move_behavior) {
    ConstructAndAssignCounter::reset();

    const ciel::split_buffer<ConstructAndAssignCounter> v1(5);
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 0);

    ciel::split_buffer<ConstructAndAssignCounter> v2(6, ConstructAndAssignCounter{});
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 6);

    const ciel::split_buffer<ConstructAndAssignCounter> v3 = v1;
    const ciel::split_buffer<ConstructAndAssignCounter> v4 = std::move(v2);
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 5);

    const ciel::split_buffer<ConstructAndAssignCounter> v5(v1.begin(), v1.end() - 1);
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 4);

    ciel::split_buffer<ConstructAndAssignCounter> v6({{}, {}, {}});
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 0);

    v6 = {{}, {}, {}, {}};
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 0);

    v6.assign(7, {});
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 7);

    v6.assign(v1.begin(), v1.end());
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 5);

    v6.assign({{}, {}, {}, {}});
    ASSERT_EQ(ConstructAndAssignCounter::copy(), 0);
}

TEST(split_buffer_tests, copy_and_move_behavior2) {
    ciel::split_buffer<ConstructAndAssignCounter> v1;
    v1.reserve_back_spare(50);
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

    // v1.reserve(100); // TODO
    // ASSERT_EQ(ConstructAndAssignCounter::move(), 40);

    ASSERT_GT(v1.capacity(), v1.size());
    v1.shrink_to_fit();
    ASSERT_EQ(ConstructAndAssignCounter::move(), 40);
}
