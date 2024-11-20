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
    const std::unique_ptr<int> up{new int{0}};
    const spinlock_ptr<int> ptr{up.get()};

    constexpr size_t threads_num = 1000;

    SimpleLatch go{threads_num};

    ciel::vector<std::thread> threads;
    threads.reserve(threads_num);

    for (size_t i = 0; i < threads_num; ++i) {
        threads.unchecked_emplace_back([&] {
            go.arrive_and_wait();

            int* p = ptr.lock();
            CIEL_DEFER({ ptr.unlock(); });

            ++(*p);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(*up, threads_num);
}
