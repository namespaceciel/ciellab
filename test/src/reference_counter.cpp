#include <gtest/gtest.h>

#include <ciel/core/reference_counter.hpp>
#include <ciel/test/simple_latch.hpp>
#include <ciel/vector.hpp>

#include <atomic>
#include <cstddef>
#include <thread>

using namespace ciel;

TEST(reference_counter, singlethread) {
    reference_counter counter;
    ASSERT_EQ(counter.load(), 1);

    ASSERT_TRUE(counter.increment_if_not_zero(1));
    ASSERT_EQ(counter.load(), 2);
    ASSERT_FALSE(counter.decrement(1));
    ASSERT_EQ(counter.load(), 1);
    ASSERT_TRUE(counter.increment_if_not_zero(2));
    ASSERT_EQ(counter.load(), 3);

    ASSERT_TRUE(counter.decrement(3));
    ASSERT_EQ(counter.load(), 0);

    ASSERT_FALSE(counter.increment_if_not_zero(1));
    ASSERT_EQ(counter.load(), 0);
}

TEST(reference_counter, multithread) {
    constexpr size_t threads_num    = 64;
    constexpr size_t operations_num = 10000;

    SimpleLatch go{threads_num + 1};

    reference_counter counter;
    std::atomic<size_t> cleanup_count{0};
    std::atomic<bool> hits_zero{false};

    ciel::vector<std::thread> write_threads(reserve_capacity, threads_num / 2);
    for (size_t i = 0; i < threads_num / 2; ++i) {
        write_threads.unchecked_emplace_back([&] {
            go.arrive_and_wait();

            for (size_t j = 0; j < operations_num; ++j) {
                if (counter.increment_if_not_zero(1)) {
                    if (counter.decrement(1)) {
                        ++cleanup_count;
                    }
                }
            }
        });
    }

    ciel::vector<std::thread> read_threads(reserve_capacity, threads_num / 2);
    for (size_t i = 0; i < threads_num / 2; ++i) {
        read_threads.unchecked_emplace_back([&] {
            go.arrive_and_wait();

            for (size_t j = 0; j < operations_num; ++j) {
                if (hits_zero.load()) {
                    ASSERT_EQ(counter.load(), 0);

                } else if (counter.load() == 0) {
                    hits_zero = true;
                }
            }
        });
    }

    go.arrive_and_wait();
    if (counter.decrement(1)) {
        ++cleanup_count;
    }

    for (auto& t : write_threads) {
        t.join();
    }

    for (auto& t : read_threads) {
        t.join();
    }

    ASSERT_EQ(cleanup_count.load(), 1);
}
