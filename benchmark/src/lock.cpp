#include <atomic>
#include <benchmark/benchmark.h>
#include <ciel/core/combining_lock.hpp>
#include <ciel/core/spinlock.hpp>
#include <cstddef>
#include <mutex>

static void lock_atomic(benchmark::State& state) {
    std::atomic<size_t> counter{0};
    for (auto _ : state) {
        ++counter;
        benchmark::DoNotOptimize(counter);
    }
}

static void lock_mutex(benchmark::State& state) {
    std::mutex mutex;
    size_t counter = 0;
    for (auto _ : state) {
        {
            std::lock_guard<std::mutex> lg(mutex);
            ++counter;
        }
        benchmark::DoNotOptimize(counter);
    }
}

static void lock_spinlock(benchmark::State& state) {
    ciel::spinlock lock;
    size_t counter = 0;
    for (auto _ : state) {
        with(lock, [&]() {
            ++counter;
        });
        benchmark::DoNotOptimize(counter);
    }
}

static void lock_combininglock(benchmark::State& state) {
    ciel::combining_lock lock;
    size_t counter = 0;
    for (auto _ : state) {
        with(lock, [&]() {
            ++counter;
        });
        benchmark::DoNotOptimize(counter);
    }
}

BENCHMARK(lock_atomic)->Threads(64);
BENCHMARK(lock_mutex)->Threads(64);
BENCHMARK(lock_spinlock)->Threads(64);
BENCHMARK(lock_combininglock)->Threads(64);
