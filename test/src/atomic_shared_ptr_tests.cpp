#include "tools.h"
#include <gtest/gtest.h>

#include <ciel/atomic_shared_ptr.hpp>

#include <numeric>
#include <thread>
#include <vector>

TEST(atomic_shared_ptr_test_suite, construction_empty) {
    ciel::atomic_shared_ptr<int> p;

    auto s = p.load();
    ASSERT_FALSE(s);
    ASSERT_EQ(s, nullptr);
}

TEST(atomic_shared_ptr_test_suite, construction_value) {
    ciel::shared_ptr<int> s{new int(5)};
    ciel::atomic_shared_ptr<int> p{std::move(s)};

    auto s2 = p.load();
    ASSERT_EQ(s2.use_count(), 2);
    ASSERT_EQ(*s2, 5);
}

TEST(atomic_shared_ptr_test_suite, store_copy) {
    ciel::atomic_shared_ptr<int> p;

    ciel::shared_ptr<int> s{new int(5)};
    ASSERT_EQ(s.use_count(), 1);
    p.store(s);
    ASSERT_EQ(s.use_count(), 2);

    auto s2 = p.load();
    ASSERT_EQ(s2.use_count(), 3);
    ASSERT_EQ(*s2, 5);
}

TEST(atomic_shared_ptr_test_suite, store_move) {
    ciel::atomic_shared_ptr<int> p;

    ciel::shared_ptr<int> s{new int(5)};
    auto s2 = s;
    ASSERT_EQ(s.use_count(), 2);

    p.store(std::move(s2));
    ASSERT_FALSE(s2);
    ASSERT_EQ(s2, nullptr);
    ASSERT_EQ(s.use_count(), 2);
}

TEST(atomic_shared_ptr_test_suite, load) {
    ciel::shared_ptr<int> s{new int(5)};
    ciel::atomic_shared_ptr<int> p{std::move(s)};
    ASSERT_FALSE(s);
    ASSERT_EQ(s, nullptr);

    ciel::shared_ptr<int> l = p.load();
    ASSERT_EQ(*l, 5);
    ASSERT_EQ(l.use_count(), 2);
}

TEST(atomic_shared_ptr_test_suite, exchange) {
    ciel::shared_ptr<int> s{new int(5)};
    ciel::atomic_shared_ptr<int> p{std::move(s)};
    ASSERT_FALSE(s);
    ASSERT_EQ(s, nullptr);

    ciel::shared_ptr<int> s2{new int(42)};
    ciel::shared_ptr<int> s3 = p.exchange(std::move(s2));

    ASSERT_EQ(*s3, 5);
    ASSERT_EQ(s3.use_count(), 1);

    ciel::shared_ptr<int> l = p.load();
    ASSERT_EQ(*l, 42);
    ASSERT_EQ(l.use_count(), 2);
}

TEST(atomic_shared_ptr_test_suite, compare_exchange_weak_true) {
    ciel::shared_ptr<int> s{new int(5)};
    ciel::atomic_shared_ptr<int> p{s};
    ASSERT_TRUE(s);
    ASSERT_EQ(s.use_count(), 2);

    ciel::shared_ptr<int> s2{new int(42)};
    bool result = p.compare_exchange_weak(s, std::move(s2));
    ASSERT_TRUE(result);
    ASSERT_FALSE(s2);
    ASSERT_EQ(s2, nullptr);

    ciel::shared_ptr<int> l = p.load();
    ASSERT_EQ(*l, 42);
    ASSERT_EQ(l.use_count(), 2);
}

TEST(atomic_shared_ptr_test_suite, compare_exchange_weak_false) {
    ciel::shared_ptr<int> s{new int(5)};
    ciel::atomic_shared_ptr<int> p{s};
    ASSERT_TRUE(s);
    ASSERT_EQ(s.use_count(), 2);

    ciel::shared_ptr<int> s2{new int(42)};
    ciel::shared_ptr<int> s3{new int(5)};
    bool result = p.compare_exchange_weak(s3, std::move(s2));
    ASSERT_FALSE(result);

    ciel::shared_ptr<int> l = p.load();
    ASSERT_EQ(*l, 5);
    ASSERT_EQ(l.use_count(), 4);
}

TEST(atomic_shared_ptr_test_suite, compare_exchange_strong_true) {
    ciel::shared_ptr<int> s{new int(5)};
    ciel::atomic_shared_ptr<int> p{s};
    ASSERT_TRUE(s);
    ASSERT_EQ(s.use_count(), 2);

    ciel::shared_ptr<int> s2{new int(42)};
    bool result = p.compare_exchange_strong(s, std::move(s2));
    ASSERT_TRUE(result);
    ASSERT_FALSE(s2);
    ASSERT_EQ(s2, nullptr);

    ciel::shared_ptr<int> l = p.load();
    ASSERT_EQ(*l, 42);
    ASSERT_EQ(l.use_count(), 2);
}

TEST(atomic_shared_ptr_test_suite, compare_exchange_strong_false) {
    ciel::shared_ptr<int> s{new int(5)};
    ciel::atomic_shared_ptr<int> p{s};
    ASSERT_TRUE(s);
    ASSERT_EQ(s.use_count(), 2);

    ciel::shared_ptr<int> s2{new int(42)};
    ciel::shared_ptr<int> s3{new int(5)};
    bool result = p.compare_exchange_strong(s3, std::move(s2));
    ASSERT_FALSE(result);

    ciel::shared_ptr<int> l = p.load();
    ASSERT_EQ(*l, 5);
    ASSERT_EQ(l.use_count(), 4);
}

// FIXME
// TEST(atomic_shared_ptr_test_suite, concurrent_store_and_loads) {
//    constexpr size_t threads_num = 64;
//    constexpr size_t operations_num = 10000;
//
//    ciel::atomic_shared_ptr<size_t> s;
//    std::latch go{threads_num};
//
//    std::vector<std::thread> consumers;
//    consumers.reserve(threads_num / 2);
//
//    for (size_t i = 0; i < threads_num / 2; ++i) {
//        consumers.emplace_back([&s, &go] {
//            go.arrive_and_wait();
//
//            for (size_t j = 0; j < operations_num; ++j) {
//                auto p = s.load();
//
//                if (p) {
//                    ASSERT_EQ(*p, 123);
//                }
//            }
//        });
//    }
//
//    std::vector<std::thread> producers;
//    producers.reserve(threads_num / 2);
//
//    for (size_t i = 0; i < threads_num / 2; ++i) {
//        producers.emplace_back([&s, &go] {
//            go.arrive_and_wait();
//
//            for (size_t j = 0; j < operations_num; ++j) {
//                s.store(ciel::make_shared<size_t>(123));
//            }
//        });
//    }
//
//    for (auto& t : consumers) {
//        t.join();
//    }
//
//    for (auto& t : producers) {
//        t.join();
//    }
//}

TEST(atomic_shared_ptr_test_suite, concurrent_exchange) {
    constexpr size_t threads_num    = 64;
    constexpr size_t operations_num = 200;

    ciel::atomic_shared_ptr<size_t> s(ciel::make_shared<size_t>(0));
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
                    ciel::shared_ptr<size_t> new_sp = ciel::make_shared<size_t>(std::rand());
                    local_sum_produced += *new_sp;

                    ciel::shared_ptr<size_t> old_sp = s.exchange(std::move(new_sp));
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
    const size_t total_consumed
        = std::accumulate(local_sums_consumed.begin(), local_sums_consumed.end(), 0ULL) + *(s.load());

    ASSERT_EQ(total_produced, total_consumed);
}
