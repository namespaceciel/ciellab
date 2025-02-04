#include <benchmark/benchmark.h>
#include <ciel/shared_ptr.hpp>
#include <memory>
#include <thread>

static void shared_ptr_inc_dec_ciel(benchmark::State& state) {
    ciel::shared_ptr<int> sp = ciel::make_shared<int>(1);
    for (auto _ : state) {
        auto copy = sp;

        benchmark::DoNotOptimize(copy);
        benchmark::ClobberMemory();
    }
}

static void shared_ptr_inc_dec_std(benchmark::State& state) {
    std::shared_ptr<int> sp = std::make_shared<int>(1);
    for (auto _ : state) {
        auto copy = sp;

        benchmark::DoNotOptimize(copy);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(shared_ptr_inc_dec_ciel)->Threads(std::thread::hardware_concurrency());
BENCHMARK(shared_ptr_inc_dec_std)->Threads(std::thread::hardware_concurrency());

static void shared_ptr_lock_ciel(benchmark::State& state) {
    ciel::shared_ptr<int> sp = ciel::make_shared<int>(1);
    ciel::weak_ptr<int> wp(sp);
    for (auto _ : state) {
        auto copy = wp.lock();

        benchmark::DoNotOptimize(copy);
        benchmark::ClobberMemory();
    }
}

static void shared_ptr_lock_std(benchmark::State& state) {
    std::shared_ptr<int> sp = std::make_shared<int>(1);
    std::weak_ptr<int> wp(sp);
    for (auto _ : state) {
        auto copy = wp.lock();

        benchmark::DoNotOptimize(copy);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(shared_ptr_lock_ciel)->Threads(std::thread::hardware_concurrency());
BENCHMARK(shared_ptr_lock_std)->Threads(std::thread::hardware_concurrency());
