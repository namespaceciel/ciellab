#include <gtest/gtest.h>

#include <ciel/core/finally.hpp>
#include <ciel/core/spinlock_ptr.hpp>
#include <ciel/test/simple_latch.hpp>
#include <ciel/vector.hpp>

#include <cstddef>
#include <memory>
#include <thread>

using namespace ciel;

TEST(spinlock_ptr, lock) {
    constexpr size_t threads_num    = 64;
    constexpr size_t operations_num = 10000;

    const std::unique_ptr<int> up{new int{0}};
    const spinlock_ptr<int> ptr{up.get()};

    SimpleLatch go{threads_num};

    ciel::vector<std::thread> threads(reserve_capacity, threads_num);

    for (size_t i = 0; i < threads_num; ++i) {
        threads.unchecked_emplace_back([&] {
            go.arrive_and_wait();

            for (size_t j = 0; j < operations_num; ++j) {
                int* p = ptr.lock();
                CIEL_DEFER({ ptr.unlock(); });

                ++(*p);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(*up, threads_num * operations_num);
}
