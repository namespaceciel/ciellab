#include <gtest/gtest.h>

#include <ciel/small_vector.hpp>

namespace {

struct ConstructAndAssignCounter {
    static size_t copy;
    static size_t move;

    ConstructAndAssignCounter() noexcept = default;
    ConstructAndAssignCounter(const ConstructAndAssignCounter&) noexcept { ++copy; }
    ConstructAndAssignCounter(ConstructAndAssignCounter&&) noexcept { ++move; }
    ConstructAndAssignCounter& operator=(const ConstructAndAssignCounter&) noexcept {
        ++copy;
        return *this;
    }
    ConstructAndAssignCounter& operator=(ConstructAndAssignCounter&&) noexcept {
        ++move;
        return *this;
    }
};

size_t ConstructAndAssignCounter::copy = 0;
size_t ConstructAndAssignCounter::move = 0;

}   // namespace

TEST(small_vector_tests, constructors) {
    const ciel::small_vector<int> v1;
    ASSERT_TRUE(v1.empty());
    ASSERT_EQ(v1.size(), 0);
    ASSERT_EQ(v1.capacity(), 8);

    const ciel::small_vector<int> v2(v1);
    ASSERT_TRUE(v2.empty());

    const ciel::small_vector<int> v3(10, 20);
    ASSERT_EQ(v3.size(), 10);

    const ciel::small_vector<int> v4(15);
    ASSERT_EQ(v4.size(), 15);

    ciel::small_vector<int> v5(v4);
    ASSERT_EQ(v5.size(), 15);

    const ciel::small_vector<int> v6(std::move(v5));
    ASSERT_EQ(v5.size(), 0);
    ASSERT_EQ(v6.size(), 15);

    const ciel::small_vector<int> v7({1, 2, 3, 4, 5});
    ASSERT_EQ(v7.size(), 5);

    const ciel::small_vector<int> v8(0, 10);
    ASSERT_TRUE(v8.empty());

    const ciel::small_vector<int> v9(0);
    ASSERT_TRUE(v9.empty());

    const ciel::small_vector<int> v10(v7.begin(), v7.begin());
    ASSERT_TRUE(v10.empty());
}

TEST(small_vector_tests, assignments) {
    ciel::small_vector<int> v1({1, 2, 3, 4, 5});
    ciel::small_vector<int> v2{};

    v2 = std::move(v1);
    ASSERT_TRUE(v1.empty());
    ASSERT_EQ(v2, std::initializer_list<int>({1, 2, 3, 4, 5}));

    ciel::small_vector<int> v3{};
    v3 = v2;
    ASSERT_EQ(v2, v3);

    // expansion
    v3 = {1 ,2, 3, 4, 5, 6, 7, 8, 9, 10};
    ASSERT_EQ(v3, std::initializer_list<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));

    // shrink
    v3.assign(2, 10);
    ASSERT_EQ(v3, std::initializer_list<int>({10, 10}));
}

TEST(small_vector_tests, at) {
    const ciel::small_vector<size_t> v1({0, 1, 2, 3, 4, 5});
    for (size_t i = 0; i < v1.size(); ++i) {
        ASSERT_EQ(v1[i], i);
    }

    ASSERT_EQ(v1.front(), 0);
    ASSERT_EQ(v1.back(), 5);

#ifdef CIEL_HAS_EXCEPTIONS
    ASSERT_THROW(CIEL_UNUSED(v1.at(-1)), std::out_of_range);
#endif
}

TEST(small_vector_tests, push_and_pop) {
    // empty
    ciel::small_vector<int> v1;
    ASSERT_EQ(v1.emplace_back(0), 0);

    v1.push_back(1);
    ASSERT_EQ(v1.emplace_back(2), 2);
    ASSERT_EQ(v1, std::initializer_list<int>({0, 1, 2}));

    ciel::small_vector<int> v2({0, 1, 2, 3, 4});
    ASSERT_EQ(v2.emplace_back(5), 5);

    ASSERT_EQ(v2.emplace_back(6), 6);
    ASSERT_EQ(v2.emplace_back(7), 7);
    ASSERT_EQ(v2.emplace_back(8), 8);
    ASSERT_EQ(v2, std::initializer_list<int>({0, 1, 2, 3, 4, 5, 6, 7, 8}));

    v2.reserve(100);
    ASSERT_EQ(v2.emplace_back(7), 7);
    ASSERT_EQ(v2.back(), 7);

    v2.pop_back();
    v2.pop_back();
    ASSERT_EQ(v2.back(), 7);

    // self assignment when expansion
    // v2.shrink_to_fit();  // TODO
    v2.push_back(v2[2]);
    ASSERT_EQ(v2.back(), 2);
}

TEST(small_vector_tests, resize) {
    ciel::small_vector<int> v1(8, 5);
    ASSERT_EQ(v1.size(), 8);
    for (const int i : v1) {
        ASSERT_EQ(i, 5);
    }

    // shrink
    v1.resize(1);
    ASSERT_EQ(v1.size(), 1);
    ASSERT_EQ(v1.front(), 5);

    // enlarge but not beyond capacity
    v1.reserve(10);
    v1.resize(10, 77);
    ASSERT_EQ(v1, std::initializer_list<int>({5, 77, 77, 77, 77, 77, 77, 77, 77, 77}));

    // enlarge beyond capacity
    v1.resize(12, 44);
    ASSERT_EQ(v1, std::initializer_list<int>({5, 77, 77, 77, 77, 77, 77, 77, 77, 77, 44, 44}));
}

TEST(small_vector_tests, insert_and_emplace) {
    ciel::small_vector<int> v1{0, 1, 2, 3, 4, 5, 6};

    // insert at front
    ASSERT_EQ(*v1.insert(v1.begin(), 21), 21);
    ASSERT_EQ(*v1.emplace(v1.begin(), 22), 22);

    ASSERT_EQ(v1, std::initializer_list<int>({22, 21, 0, 1, 2, 3, 4, 5, 6}));

    // insert at back
    ASSERT_EQ(*v1.insert(v1.end(), 31), 31);
    ASSERT_EQ(*v1.emplace(v1.end(), 32), 32);

    // insert at mid
    ASSERT_EQ(*v1.insert(v1.begin() + 5, 2, 41), 41);
    ASSERT_EQ(*v1.insert(v1.begin() + 8, {42, 43}), 42);

    // insert empty range
    ASSERT_EQ(*v1.insert(v1.begin(), v1.begin(), v1.begin()), 22);

    ASSERT_EQ(v1, std::initializer_list<int>({22, 21, 0, 1, 2, 41, 41, 3, 42, 43, 4, 5, 6, 31, 32}));

    // insert when expansion
    // v1.shrink_to_fit();  // TODO
    ASSERT_EQ(*v1.insert(v1.begin() + 2, 99), 99);
    ASSERT_EQ(v1, std::initializer_list<int>({22, 21, 99, 0, 1, 2, 41, 41, 3, 42, 43, 4, 5, 6, 31, 32}));

    // insert self range when expansion
    // v1.shrink_to_fit();  // TODO
    ASSERT_EQ(*v1.insert(v1.begin() + 2, v1.begin() + 1, v1.begin() + 5), 21);
    ASSERT_EQ(v1, std::initializer_list<int>({22, 21, 21, 99, 0, 1, 99, 0, 1, 2, 41, 41, 3, 42, 43, 4, 5, 6, 31, 32}));
}

TEST(small_vector_tests, erase) {
    ciel::small_vector<int> v1{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    ASSERT_EQ(*v1.erase(v1.begin()), 1);
    ASSERT_EQ(v1, std::initializer_list<int>({1, 2, 3, 4, 5, 6, 7, 8, 9}));

    ASSERT_EQ(*v1.erase(v1.begin() + 2, v1.begin() + 4), 5);
    ASSERT_EQ(v1, std::initializer_list<int>({1, 2, 5, 6, 7, 8, 9}));

    // You can't ensure this eval_order, v1.end() may be calculated before erasing
    // ASSERT_EQ(v1.erase(v1.end() - 1), v1.end());

    auto res = v1.erase(v1.end() - 1);
    ASSERT_EQ(res, v1.end());
    ASSERT_EQ(v1, std::initializer_list<int>({1, 2, 5, 6, 7, 8}));

    res = v1.erase(v1.end() - 2, v1.end());
    ASSERT_EQ(res, v1.end());
    ASSERT_EQ(v1, std::initializer_list<int>({1, 2, 5, 6}));
}

TEST(small_vector_tests, copy_and_move_behavior) {
    ConstructAndAssignCounter::copy = 0;
    ConstructAndAssignCounter::move = 0;

    ciel::small_vector<ConstructAndAssignCounter> v1(5);
    ASSERT_EQ(ConstructAndAssignCounter::copy, 0);

    ciel::small_vector<ConstructAndAssignCounter> v2(6, ConstructAndAssignCounter{});
    ASSERT_EQ(ConstructAndAssignCounter::copy, 6);

    const ciel::small_vector<ConstructAndAssignCounter> v3 = v1;
    const ciel::small_vector<ConstructAndAssignCounter> v4 = std::move(v2);
    ASSERT_EQ(ConstructAndAssignCounter::copy, 17);

    const ciel::small_vector<ConstructAndAssignCounter> v5(v1.begin(), v1.end() - 1);
    ASSERT_EQ(ConstructAndAssignCounter::copy, 21);

    ciel::small_vector<ConstructAndAssignCounter> v6({{}, {}, {}});
    ASSERT_EQ(ConstructAndAssignCounter::copy, 24);

    v6 = {{}, {}, {}, {}};
    ASSERT_EQ(ConstructAndAssignCounter::copy, 28);

    v6.assign(7, {});
    ASSERT_EQ(ConstructAndAssignCounter::copy, 35);

    v6.assign(v1.begin(), v1.end());
    ASSERT_EQ(ConstructAndAssignCounter::copy, 40);

    v6.assign({{}, {}, {}, {}});
    ASSERT_EQ(ConstructAndAssignCounter::copy, 44);

    ASSERT_EQ(ConstructAndAssignCounter::move, 0);
}

TEST(small_vector_tests, copy_and_move_behavior2) {
    ConstructAndAssignCounter::copy = 0;

    ciel::small_vector<ConstructAndAssignCounter> v1;

    for (size_t i = 0; i < 10; ++i) {
        v1.emplace_back();
    }
    ASSERT_EQ(ConstructAndAssignCounter::copy, 0);

    for (size_t i = 0; i < 10; ++i) {
        v1.push_back({});
    }
    ASSERT_EQ(ConstructAndAssignCounter::copy, 0);

    ConstructAndAssignCounter tmp;

    for (size_t i = 0; i < 10; ++i) {
        v1.push_back(std::move(tmp));
    }
    ASSERT_EQ(ConstructAndAssignCounter::copy, 0);

    for (size_t i = 0; i < 10; ++i) {
        v1.push_back(tmp);
    }
    ASSERT_EQ(ConstructAndAssignCounter::copy, 10);

    v1.reserve(100);

    // v1.shrink_to_fit();  // TODO
    ASSERT_EQ(ConstructAndAssignCounter::copy, 10);
}

TEST(small_vector_tests, copy_and_move_behavior3) {
    ConstructAndAssignCounter::copy = 0;

    ciel::small_vector<ConstructAndAssignCounter> v1(10);
    v1.erase(v1.begin());
    ASSERT_EQ(ConstructAndAssignCounter::copy, 0);

    v1.erase(v1.begin() + 5, v1.begin() + 7);
    ASSERT_EQ(ConstructAndAssignCounter::copy, 0);

    v1.insert(v1.begin(), ConstructAndAssignCounter{});
    ASSERT_EQ(ConstructAndAssignCounter::copy, 0);

    constexpr ConstructAndAssignCounter tmp;
    v1.insert(v1.begin(), tmp);
    ASSERT_EQ(ConstructAndAssignCounter::copy, 1);

    v1.insert(v1.begin(), 3, {});
    ASSERT_EQ(ConstructAndAssignCounter::copy, 4);

    v1.insert(v1.begin(), {{}, {}});
    ASSERT_EQ(ConstructAndAssignCounter::copy, 6);

    // v1.shrink_to_fit();    // capacity turns to 14   // TODO
    // ASSERT_EQ(ConstructAndAssignCounter::copy, 6);

    v1.insert(v1.end() - 2, v1.begin(), v1.begin() + 2);
    ASSERT_EQ(ConstructAndAssignCounter::copy, 8);
}