#include <atomic>
#include <benchmark/benchmark.h>
#include <ciel/core/config.hpp>
#include <ciel/core/singleton.hpp>
#include <ciel/core/spinlock.hpp>

namespace {

class SingletonOfCiel : public ciel::singleton<SingletonOfCiel> {};

class SingletonOfStatic {
private:
    SingletonOfStatic() = default;

public:
    CIEL_NODISCARD static SingletonOfStatic& get() {
        static SingletonOfStatic res;
        return res;
    }

    SingletonOfStatic(const SingletonOfStatic&)            = delete;
    SingletonOfStatic& operator=(const SingletonOfStatic&) = delete;

}; // class SingletonOfStatic

class SingletonOfDCLP {
private:
    SingletonOfDCLP() = default;

public:
    CIEL_NODISCARD static SingletonOfDCLP& get() {
        ciel::spinlock lock;
        alignas(SingletonOfDCLP) static unsigned char buffer[sizeof(SingletonOfDCLP)];
        static std::atomic<SingletonOfDCLP*> ptr;

        SingletonOfDCLP* tmp = ptr.load(std::memory_order_acquire);

        if CIEL_UNLIKELY (tmp == nullptr) {
            with(lock, [&]() {
                tmp = ptr.load(std::memory_order_relaxed);

                if (tmp == nullptr) {
                    tmp = ::new (&buffer) SingletonOfDCLP;
                    ptr.store(tmp, std::memory_order_release);
                }
            });
        }

        return *tmp;
    }

    SingletonOfDCLP(const SingletonOfDCLP&)            = delete;
    SingletonOfDCLP& operator=(const SingletonOfDCLP&) = delete;

}; // class SingletonOfDCLP

} // namespace

static void singleton_ciel(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(SingletonOfCiel::get());
    }

    benchmark::ClobberMemory();
}

static void singleton_dclp(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(SingletonOfDCLP::get());
    }

    benchmark::ClobberMemory();
}

static void singleton_static(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(SingletonOfStatic::get());
    }

    benchmark::ClobberMemory();
}

BENCHMARK(singleton_ciel)->Threads(std::thread::hardware_concurrency());
BENCHMARK(singleton_dclp)->Threads(std::thread::hardware_concurrency());
BENCHMARK(singleton_static)->Threads(std::thread::hardware_concurrency());
