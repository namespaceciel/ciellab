#include <gtest/gtest.h>

#include <ciel/core/treiber_stack.hpp>
#include <ciel/test/simple_latch.hpp>
#include <ciel/vector.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <thread>

using namespace ciel;

namespace {

struct Node {
    size_t value{1};
    std::atomic<Node*> next{this}; // Intentionally

}; // struct Node

} // namespace

TEST(treiber_stack, concurrent_push_and_pop) {
    constexpr size_t threads_num    = 64;
    constexpr size_t operations_num = 1000;

    std::array<std::array<Node, operations_num>, threads_num / 2> arr;
    SimpleLatch go{threads_num};
    treiber_stack<Node> stack;
    std::atomic<size_t> count{0};

    ciel::vector<std::thread> push_threads;
    push_threads.reserve(threads_num / 2);
    for (size_t i = 0; i < threads_num / 2; ++i) {
        push_threads.unchecked_emplace_back([&, i] {
            go.arrive_and_wait();

            for (size_t j = 0; j < operations_num; ++j) {
                stack.push(std::addressof(arr[i][j]));
            }
        });
    }

    ciel::vector<std::thread> pop_threads;
    pop_threads.reserve(threads_num / 2);
    for (size_t i = 0; i < threads_num / 2; ++i) {
        pop_threads.unchecked_emplace_back([&] {
            go.arrive_and_wait();

            for (size_t j = 0; j < operations_num; ++j) {
                Node* node = stack.pop();
                if (node != nullptr) {
                    count += node->value;
                }
            }
        });
    }

    for (auto& t : push_threads) {
        t.join();
    }

    for (auto& t : pop_threads) {
        t.join();
    }

    Node* top = stack.pop_all();
    while (top != nullptr) {
        count += top->value;
        top = top->next;
    }

    ASSERT_EQ(count, threads_num / 2 * operations_num);
}
