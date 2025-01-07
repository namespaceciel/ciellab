#include <gtest/gtest.h>

#include <ciel/core/combining_lock.hpp>
#include <ciel/core/spinlock.hpp>
#include <ciel/test/simple_latch.hpp>
#include <ciel/vector.hpp>

#include <cstddef>
#include <thread>

using namespace ciel;

namespace {

template<class Lock>
void test_impl(::testing::Test*) {
    constexpr size_t threads_num    = 64;
    constexpr size_t operations_num = 10000;

    Lock lock;
    size_t count = 0;
    SimpleLatch go{threads_num};

    ciel::vector<std::thread> threads(reserve_capacity, threads_num);

    for (size_t i = 0; i < threads_num; ++i) {
        threads.unchecked_emplace_back([&] {
            go.arrive_and_wait();

            for (size_t j = 0; j < operations_num; ++j) {
                with(lock, [&]() {
                    ++count;
                });
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(count, threads_num * operations_num);
}

} // namespace

TEST(with_lock, lock) {
    test_impl<combining_lock>(this);
    test_impl<spinlock>(this);
}
