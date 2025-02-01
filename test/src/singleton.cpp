#include <gtest/gtest.h>

#include <ciel/core/singleton.hpp>
#include <ciel/test/simple_latch.hpp>
#include <ciel/vector.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <random>
#include <thread>

using namespace ciel;

namespace {

struct NonThrow : singleton<NonThrow> {
    static std::atomic<size_t> counter;

    NonThrow() noexcept {
        ++counter;
    }
};

std::atomic<size_t> NonThrow::counter{0};

#ifdef CIEL_HAS_EXCEPTIONS
std::random_device rd;
std::mt19937_64 g(rd());

struct CanThrow : singleton<CanThrow> {
    static std::atomic<size_t> counter;

    CanThrow() {
        if (g() % 20 > 0) {
            throw 0;
        }

        ++counter;
    }
};

std::atomic<size_t> CanThrow::counter{0};
#endif

} // namespace

TEST(singleton, non_throw) {
    constexpr size_t threads_num = 64;

    SimpleLatch go{threads_num};
    ciel::vector<std::thread> threads(reserve_capacity, threads_num);

    for (size_t i = 0; i < threads_num; ++i) {
        threads.unchecked_emplace_back([&] {
            go.arrive_and_wait();

            CIEL_UNUSED(NonThrow::get());
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(NonThrow::counter, 1);
}

#ifdef CIEL_HAS_EXCEPTIONS
TEST(singleton, can_throw) {
    constexpr size_t threads_num = 64;

    SimpleLatch go{threads_num};
    ciel::vector<std::thread> threads(reserve_capacity, threads_num);

    std::atomic<size_t> throws{0};

    for (size_t i = 0; i < threads_num; ++i) {
        threads.unchecked_emplace_back([&] {
            go.arrive_and_wait();

            try {
                CIEL_UNUSED(CanThrow::get());

            } catch (...) {
                ++throws;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_TRUE(CanThrow::counter == 1 || throws == threads_num)
        << "counter: " << CanThrow::counter << ", throws: " << throws;
}
#endif
