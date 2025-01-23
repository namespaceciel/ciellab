#include <gtest/gtest.h>

#include <ciel/core/message.hpp>
#include <ciel/core/mpsc_queue.hpp>
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

TEST(mpsc_queue, singlethread) {
    std::array<Node, 6> arr;
    mpsc_queue<Node> queue;
    std::atomic<size_t> count{0};
    queue.process([&](Node*) {
        ciel::unreachable();
        return true;
    });

    queue.push(arr.data());
    queue.process([&](Node*) {
        ciel::unreachable();
        return true;
    });

    (arr.data() + 1)->next.store(arr.data() + 2);
    (arr.data() + 2)->next.store(arr.data() + 3);
    (arr.data() + 3)->next.store(arr.data() + 4);
    (arr.data() + 4)->next.store(arr.data() + 5);
    queue.push(arr.data() + 1, arr.data() + 5);
    queue.process([&](Node* node) {
        count += node->value;
        return true;
    });
    ASSERT_EQ(count, 5);

    queue.destructive_process([&](Node* node) {
        count += node->value;
    });
    ASSERT_EQ(count, 6);
}

TEST(mpsc_queue, multithread) {
    constexpr size_t producer_threads_num = 64;
    constexpr size_t operations_num       = 10000;

    using ArrayType = std::array<std::array<Node, operations_num>, producer_threads_num>;
    std::unique_ptr<ArrayType> arr{new ArrayType{}};
    SimpleLatch go{producer_threads_num + 1};
    mpsc_queue<Node> queue;
    std::atomic<size_t> count{0};

    ciel::vector<std::thread> producer_threads(reserve_capacity, producer_threads_num);
    for (size_t i = 0; i < producer_threads_num; ++i) {
        producer_threads.unchecked_emplace_back([&, i] {
            go.arrive_and_wait();

            for (size_t j = 0; j < operations_num; ++j) {
                queue.push(std::addressof((*arr)[i][j]));
            }
        });
    }

    std::atomic<bool> flag{false};
    std::thread consumer([&] {
        go.arrive_and_wait();

        while (!flag) {
            queue.process([&](Node* node) {
                count += node->value;
                return true;
            });

            // std::this_thread::yield();
        }
    });

    for (auto& t : producer_threads) {
        t.join();
    }
    flag = true;
    consumer.join();

    queue.destructive_process([&](Node* node) {
        count += node->value;
    });

    ASSERT_EQ(count, producer_threads_num * operations_num);
}
