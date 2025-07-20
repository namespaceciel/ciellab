#include <gtest/gtest.h>

#include <ciel/core/config.hpp>
#include <ciel/core/rb_tree.hpp>
#include <ciel/vector.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <random>

using namespace ciel;

TEST(rb_tree, int) {
    ciel::vector<rb_node<int>> v(reserve_capacity, 10000);
    for (int i = 0; i < 10000; ++i) {
        v.unchecked_emplace_back(i);
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(v.begin(), v.end(), g);

    rb_tree<int, std::less<int>> tree;
    for (auto& node : v) {
        ASSERT_TRUE(tree.insert(&node));
    }
    ASSERT_TRUE(std::is_sorted(tree.begin(), tree.end(), std::less<int>{}));
    ASSERT_EQ(tree.size(), v.size());

    for (auto& node : v) {
        auto p = tree.find(node.value());
        ASSERT_NE(p, nullptr);

        tree.remove(p);
    }

    ASSERT_TRUE(tree.empty());
}

TEST(rb_tree, void) {
    struct Compare {
        CIEL_NODISCARD bool operator()(uintptr_t lhs, uintptr_t rhs) const noexcept {
            return lhs < rhs;
        }

    }; // struct Compare

    ciel::vector<rb_node<void>> v(10000);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(v.begin(), v.end(), g);

    rb_tree<void, Compare> tree;
    for (auto& node : v) {
        ASSERT_TRUE(tree.insert(&node));
    }
    ASSERT_TRUE(std::is_sorted(tree.begin(), tree.end(), Compare{}));
    ASSERT_EQ(tree.size(), v.size());

    for (auto& node : v) {
        auto p = tree.find(node.value());
        ASSERT_NE(p, nullptr);

        tree.remove(p);
    }

    ASSERT_TRUE(tree.empty());
}
