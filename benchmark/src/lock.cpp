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
        benchmark::ClobberMemory();
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
        benchmark::ClobberMemory();
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
        benchmark::ClobberMemory();
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
        benchmark::ClobberMemory();
    }
}

BENCHMARK(lock_atomic)->Threads(std::thread::hardware_concurrency());
BENCHMARK(lock_mutex)->Threads(std::thread::hardware_concurrency());
BENCHMARK(lock_spinlock)->Threads(std::thread::hardware_concurrency());
BENCHMARK(lock_combininglock)->Threads(std::thread::hardware_concurrency());
