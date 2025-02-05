// __cpp_lib_atomic_shared_ptr doesn't work.
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 12

#  include <atomic>
#  include <benchmark/benchmark.h>
#  include <ciel/atomic_shared_ptr.hpp>
#  include <memory>
#  include <thread>

static void atomic_shared_ptr_load_ciel(benchmark::State& state) {
    ciel::atomic_shared_ptr<int> sp = ciel::make_shared<int>(1);
    for (auto _ : state) {
        auto copy = sp.load(std::memory_order_acquire);

        benchmark::DoNotOptimize(copy);
        benchmark::ClobberMemory();
    }
}

static void atomic_shared_ptr_load_std(benchmark::State& state) {
    std::atomic<std::shared_ptr<int>> sp = std::make_shared<int>(1);
    for (auto _ : state) {
        auto copy = sp.load(std::memory_order_acquire);

        benchmark::DoNotOptimize(copy);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(atomic_shared_ptr_load_ciel)->Threads(std::thread::hardware_concurrency());
BENCHMARK(atomic_shared_ptr_load_std)->Threads(std::thread::hardware_concurrency());

static void atomic_shared_ptr_exchange_ciel(benchmark::State& state) {
    ciel::atomic_shared_ptr<int> sp = ciel::make_shared<int>(1);
    for (auto _ : state) {
        auto copy = sp.exchange(ciel::make_shared<int>(1), std::memory_order_acq_rel);

        benchmark::DoNotOptimize(copy);
        benchmark::ClobberMemory();
    }
}

static void atomic_shared_ptr_exchange_std(benchmark::State& state) {
    std::atomic<std::shared_ptr<int>> sp = std::make_shared<int>(1);
    for (auto _ : state) {
        auto copy = sp.exchange(std::make_shared<int>(1), std::memory_order_acq_rel);

        benchmark::DoNotOptimize(copy);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(atomic_shared_ptr_exchange_ciel)->Threads(std::thread::hardware_concurrency());
BENCHMARK(atomic_shared_ptr_exchange_std)->Threads(std::thread::hardware_concurrency());

static void atomic_shared_ptr_cas_ciel(benchmark::State& state) {
    ciel::atomic_shared_ptr<int> sp = ciel::make_shared<int>(1);
    ciel::shared_ptr<int> copy;
    for (auto _ : state) {
        do {
            copy = sp.load(std::memory_order_relaxed);
        } while (!sp.compare_exchange_weak(copy, ciel::make_shared<int>(1), std::memory_order_acq_rel));

        benchmark::DoNotOptimize(copy);
        benchmark::ClobberMemory();
    }
}

static void atomic_shared_ptr_cas_std(benchmark::State& state) {
    std::atomic<std::shared_ptr<int>> sp = std::make_shared<int>(1);
    std::shared_ptr<int> copy;
    for (auto _ : state) {
        do {
            copy = sp.load(std::memory_order_relaxed);
        } while (!sp.compare_exchange_weak(copy, std::make_shared<int>(1), std::memory_order_acq_rel));

        benchmark::DoNotOptimize(copy);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(atomic_shared_ptr_cas_ciel)->Threads(std::thread::hardware_concurrency());
BENCHMARK(atomic_shared_ptr_cas_std)->Threads(std::thread::hardware_concurrency());

#endif
