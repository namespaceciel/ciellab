#include <gtest/gtest.h>

#include <ciel/core/avl_raw.hpp>
#include <ciel/vector.hpp>

#include <algorithm>
#include <functional>
#include <random>

using namespace ciel;

TEST(avl_raw, all) {
    ciel::vector<avl_node<int>> v;
    v.reserve(10000);
    for (int i = 0; i < 10000; ++i) {
        v.unchecked_emplace_back(i);
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(v.begin(), v.end(), g);

    avl_raw<int, std::less<int>> avl;
    for (auto& node : v) {
        ASSERT_TRUE(avl.insert_node_unique(&node).second);
    }

    avl_node<int> node1{-1};
    avl_node<int> node2{-2};
    avl_node<int> node3{-3};
    ASSERT_TRUE(avl.insert_node_unique(&node1).second);
    ASSERT_TRUE(avl.insert_node_unique(&node2).second);
    ASSERT_TRUE(avl.insert_node_unique(&node3).second);

    {
        ASSERT_EQ(avl.size(), 10003);
        auto it = avl.begin();
        for (int i = -3; i < 10000; ++i, ++it) {
            ASSERT_EQ(*it, i);
        }
        ASSERT_EQ(it, avl.end());
    }

    ASSERT_EQ(avl.pop_node(avl.begin()), &node3);
    ASSERT_EQ(avl.pop_node(avl.begin()), &node2);
    ASSERT_EQ(avl.pop_node(avl.begin()), &node1);
    for (auto& node : v) {
        auto it = avl.find(node.value);
        ASSERT_NE(it, avl.end());

        ASSERT_EQ(avl.pop_node(it), &node);
    }
    ASSERT_TRUE(avl.empty());
}
