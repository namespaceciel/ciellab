#include <gtest/gtest.h>

#include <ciel/core/config.hpp>
#include <ciel/core/rb_tree.hpp>
#include <ciel/vector.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <random>

using namespace ciel;

namespace {

template<class T>
struct RBNode : rb_node_base {
    using value_type = T;

private:
    value_type value_;

public:
    template<class... Args>
    RBNode(Args&&... args) noexcept(std::is_nothrow_constructible<value_type, Args&&...>::value)
        : value_(std::forward<Args>(args)...) {}

    CIEL_NODISCARD value_type& value() noexcept {
        return value_;
    }

    CIEL_NODISCARD const value_type& value() const noexcept {
        return value_;
    }

}; // struct RBNode

} // namespace

TEST(rb_tree, int) {
    using NodeType = RBNode<int>;
    using Compare  = std::less<int>;

    ciel::vector<NodeType> v(reserve_capacity, 10000);
    for (int i = 0; i < 10000; ++i) {
        v.unchecked_emplace_back(i);
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(v.begin(), v.end(), g);

    rb_tree<NodeType, Compare> tree;
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

TEST(rb_tree, void) {
    struct RBNodeInplace : rb_node_base {
        using value_type = uintptr_t;

        CIEL_NODISCARD value_type value() const noexcept {
            return reinterpret_cast<value_type>(this);
        }

    }; // struct RBNode

    using Compare = std::less<uintptr_t>;

    ciel::vector<RBNodeInplace> v(10000);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(v.begin(), v.end(), g);

    rb_tree<RBNodeInplace, Compare> tree;
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
