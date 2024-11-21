#include <gtest/gtest.h>

#include <ciel/atomic_shared_ptr.hpp>
#include <ciel/shared_ptr.hpp>
#include <ciel/test/simple_latch.hpp>

#include <cstddef>
#include <cstdlib>
#include <numeric>
#include <thread>
#include <utility>
#include <vector>

using namespace ciel;

TEST(atomic_shared_ptr, construction_empty) {
    const atomic_shared_ptr<int> p;

    auto s = p.load();
    ASSERT_FALSE(s);
    ASSERT_EQ(s, nullptr);
}

TEST(atomic_shared_ptr, construction_value) {
    shared_ptr<int> s{new int(5)};
    const atomic_shared_ptr<int> p{std::move(s)};

    auto s2 = p.load();
    ASSERT_EQ(s2.use_count(), 2);
    ASSERT_EQ(*s2, 5);
}

TEST(atomic_shared_ptr, store_copy) {
    atomic_shared_ptr<int> p;

    const shared_ptr<int> s{new int(5)};
    ASSERT_EQ(s.use_count(), 1);
    p.store(s);
    ASSERT_EQ(s.use_count(), 2);

    const shared_ptr<int> s2 = p.load();
    ASSERT_EQ(s2.use_count(), 3);
    ASSERT_EQ(*s2, 5);
}

TEST(atomic_shared_ptr, store_move) {
    atomic_shared_ptr<int> p;

    const shared_ptr<int> s{new int(5)};
    shared_ptr<int> s2 = s;
    ASSERT_EQ(s.use_count(), 2);

    p.store(std::move(s2));
    ASSERT_FALSE(s2); // NOLINT(bugprone-use-after-move)
    ASSERT_EQ(s2, nullptr);
    ASSERT_EQ(s.use_count(), 2);
}

TEST(atomic_shared_ptr, load) {
    shared_ptr<int> s{new int(5)};
    const atomic_shared_ptr<int> p{std::move(s)};
    ASSERT_FALSE(s); // NOLINT(bugprone-use-after-move)
    ASSERT_EQ(s, nullptr);

    const shared_ptr<int> l = p.load();
    ASSERT_EQ(*l, 5);
    ASSERT_EQ(l.use_count(), 2);
}

TEST(atomic_shared_ptr, exchange) {
    shared_ptr<int> s{new int(5)};
    atomic_shared_ptr<int> p{std::move(s)};
    ASSERT_FALSE(s); // NOLINT(bugprone-use-after-move)
    ASSERT_EQ(s, nullptr);

    shared_ptr<int> s2{new int(42)};
    const shared_ptr<int> s3 = p.exchange(std::move(s2));

    ASSERT_EQ(*s3, 5);
    ASSERT_EQ(s3.use_count(), 1);

    const shared_ptr<int> l = p.load();
    ASSERT_EQ(*l, 42);
    ASSERT_EQ(l.use_count(), 2);
}

TEST(atomic_shared_ptr, compare_exchange_weak_true) {
    shared_ptr<int> s{new int(5)};
    atomic_shared_ptr<int> p{s};
    ASSERT_TRUE(s);
    ASSERT_EQ(s.use_count(), 2);

    shared_ptr<int> s2{new int(42)};
    const bool result = p.compare_exchange_weak(s, std::move(s2));
    ASSERT_TRUE(result);
    ASSERT_FALSE(s2); // NOLINT(bugprone-use-after-move)
    ASSERT_EQ(s2, nullptr);

    const shared_ptr<int> l = p.load();
    ASSERT_EQ(*l, 42);
    ASSERT_EQ(l.use_count(), 2);
}

TEST(atomic_shared_ptr, compare_exchange_weak_false) {
    const shared_ptr<int> s{new int(5)};
    atomic_shared_ptr<int> p{s};
    ASSERT_TRUE(s);
    ASSERT_EQ(s.use_count(), 2);

    shared_ptr<int> s2{new int(42)};
    shared_ptr<int> s3{new int(5)};
    const bool result = p.compare_exchange_weak(s3, std::move(s2));
    ASSERT_FALSE(result);

    const shared_ptr<int> l = p.load();
    ASSERT_EQ(*l, 5);
    ASSERT_EQ(l.use_count(), 4);
}

TEST(atomic_shared_ptr, compare_exchange_strong_true) {
    shared_ptr<int> s{new int(5)};
    atomic_shared_ptr<int> p{s};
    ASSERT_TRUE(s);
    ASSERT_EQ(s.use_count(), 2);

    shared_ptr<int> s2{new int(42)};
    const bool result = p.compare_exchange_strong(s, std::move(s2));
    ASSERT_TRUE(result);
    ASSERT_FALSE(s2); // NOLINT(bugprone-use-after-move)
    ASSERT_EQ(s2, nullptr);

    const shared_ptr<int> l = p.load();
    ASSERT_EQ(*l, 42);
    ASSERT_EQ(l.use_count(), 2);
}

TEST(atomic_shared_ptr, compare_exchange_strong_false) {
    const shared_ptr<int> s{new int(5)};
    atomic_shared_ptr<int> p{s};
    ASSERT_TRUE(s);
    ASSERT_EQ(s.use_count(), 2);

    shared_ptr<int> s2{new int(42)};
    shared_ptr<int> s3{new int(5)};
    const bool result = p.compare_exchange_strong(s3, std::move(s2));
    ASSERT_FALSE(result);

    const shared_ptr<int> l = p.load();
    ASSERT_EQ(*l, 5);
    ASSERT_EQ(l.use_count(), 4);
}

TEST(atomic_shared_ptr, concurrent_store_and_loads) {
    constexpr size_t threads_num    = 64;
    constexpr size_t operations_num = 10000;

    atomic_shared_ptr<size_t> s;
    ciel::SimpleLatch go{threads_num};

    std::vector<std::thread> consumers;
    consumers.reserve(threads_num / 2);

    for (size_t i = 0; i < threads_num / 2; ++i) {
        consumers.emplace_back([&s, &go] {
            go.arrive_and_wait();

            for (size_t j = 0; j < operations_num; ++j) {
                auto p = s.load();

                if (p) {
                    ASSERT_EQ(*p, 123);
                }
            }
        });
    }

    std::vector<std::thread> producers;
    producers.reserve(threads_num / 2);

    for (size_t i = 0; i < threads_num / 2; ++i) {
        producers.emplace_back([&s, &go] {
            go.arrive_and_wait();

            for (size_t j = 0; j < operations_num; ++j) {
                s.store(make_shared<size_t>(123));
            }
        });
    }

    for (auto& t : consumers) {
        t.join();
    }

    for (auto& t : producers) {
        t.join();
    }
}

TEST(atomic_shared_ptr, concurrent_exchange) {
    constexpr size_t threads_num    = 64;
    constexpr size_t operations_num = 200;

    atomic_shared_ptr<size_t> s(make_shared<size_t>(0));
    SimpleLatch go{threads_num};

    std::vector<size_t> local_sums_produced(threads_num);
    std::vector<size_t> local_sums_consumed(threads_num);
    {
        std::vector<std::thread> threads;
        threads.reserve(threads_num);

        for (size_t i = 0; i < threads_num; ++i) {
            threads.emplace_back([i, &s, &go, &local_sums_produced, &local_sums_consumed] {
                go.arrive_and_wait();

                size_t local_sum_produced = 0;
                size_t local_sum_consumed = 0;

                for (size_t j = 0; j < operations_num; ++j) {
                    shared_ptr<size_t> new_sp = make_shared<size_t>(std::rand());
                    local_sum_produced += *new_sp;

                    const shared_ptr<size_t> old_sp = s.exchange(std::move(new_sp));
                    ASSERT_TRUE(old_sp);
                    local_sum_consumed += *old_sp;
                }

                local_sums_produced[i] = local_sum_produced;
                local_sums_consumed[i] = local_sum_consumed;
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    }

    const size_t total_produced = std::accumulate(local_sums_produced.begin(), local_sums_produced.end(), 0ULL);
    const size_t total_consumed =
        std::accumulate(local_sums_consumed.begin(), local_sums_consumed.end(), 0ULL) + *(s.load());

    ASSERT_EQ(total_produced, total_consumed);
}
