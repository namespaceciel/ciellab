#include <benchmark/benchmark.h>
#include <ciel/core/exchange.hpp>
#include <ciel/core/is_trivially_relocatable.hpp>
#include <ciel/vector.hpp>
#include <cstddef>
#include <vector>

namespace {

class tr {
private:
    int* ptr;

public:
    tr(int i = 0)
        : ptr(new int{i}) {}

    tr(const tr&)            = delete;
    tr& operator=(const tr&) = delete;

    tr(tr&& other) noexcept
        : ptr(ciel::exchange(other.ptr, nullptr)) {}

    tr& operator=(tr&& other) noexcept {
        delete ptr;
        ptr = ciel::exchange(other.ptr, nullptr);
        return *this;
    }

    ~tr() {
        delete ptr;
    }

}; // class tr

} // namespace

template<>
struct ciel::is_trivially_relocatable<tr> : std::true_type {};

// emplace_back

template<class Container>
static void bench_emplace_back_impl(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        Container v;
        state.ResumeTiming();

        for (int i = 0; i < state.range(0); ++i) {
            v.emplace_back(i);
        }

        benchmark::DoNotOptimize(v);
        benchmark::ClobberMemory();
    }
}

static void vector_int_emplace_back_ciel(benchmark::State& state) {
    bench_emplace_back_impl<ciel::vector<int>>(state);
}

static void vector_int_emplace_back_std(benchmark::State& state) {
    bench_emplace_back_impl<std::vector<int>>(state);
}

static void vector_tr_emplace_back_ciel(benchmark::State& state) {
    bench_emplace_back_impl<ciel::vector<tr>>(state);
}

static void vector_tr_emplace_back_std(benchmark::State& state) {
    bench_emplace_back_impl<std::vector<tr>>(state);
}

BENCHMARK(vector_int_emplace_back_ciel)->Arg(10000);
BENCHMARK(vector_int_emplace_back_std)->Arg(10000);
BENCHMARK(vector_tr_emplace_back_ciel)->Arg(10000);
BENCHMARK(vector_tr_emplace_back_std)->Arg(10000);

// insert

template<class Container>
static void bench_insert_impl(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        Container v;
        auto it = v.begin();
        state.ResumeTiming();

        for (int i = 0; i < state.range(0); ++i) {
            it = v.insert(it, i);

            // Try to safely increment the iterator three times.
            if (it == v.end()) {
                it = v.begin();
            }
            if (++it == v.end()) {
                it = v.begin();
            }
            if (++it == v.end()) {
                it = v.begin();
            }
        }

        benchmark::DoNotOptimize(v);
        benchmark::ClobberMemory();
    }
}

static void vector_int_insert_ciel(benchmark::State& state) {
    bench_insert_impl<ciel::vector<int>>(state);
}

static void vector_int_insert_std(benchmark::State& state) {
    bench_insert_impl<std::vector<int>>(state);
}

static void vector_tr_insert_ciel(benchmark::State& state) {
    bench_insert_impl<ciel::vector<tr>>(state);
}

static void vector_tr_insert_std(benchmark::State& state) {
    bench_insert_impl<std::vector<tr>>(state);
}

BENCHMARK(vector_int_insert_ciel)->Arg(1000);
BENCHMARK(vector_int_insert_std)->Arg(1000);
BENCHMARK(vector_tr_insert_ciel)->Arg(1000);
BENCHMARK(vector_tr_insert_std)->Arg(1000);

// erase

template<class Container>
static void bench_erase_impl(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        Container v(state.range(0));
        auto it = v.begin();
        state.ResumeTiming();

        for (int i = 0; i < state.range(0); ++i) {
            it = v.erase(it);

            // Try to safely increment the iterator three times.
            if (it == v.end()) {
                it = v.begin();
            }
            if (++it == v.end()) {
                it = v.begin();
            }
            if (++it == v.end()) {
                it = v.begin();
            }
        }

        benchmark::DoNotOptimize(v);
        benchmark::ClobberMemory();
    }
}

static void vector_int_erase_ciel(benchmark::State& state) {
    bench_erase_impl<ciel::vector<int>>(state);
}

static void vector_int_erase_std(benchmark::State& state) {
    bench_erase_impl<std::vector<int>>(state);
}

static void vector_tr_erase_ciel(benchmark::State& state) {
    bench_erase_impl<ciel::vector<tr>>(state);
}

static void vector_tr_erase_std(benchmark::State& state) {
    bench_erase_impl<std::vector<tr>>(state);
}

BENCHMARK(vector_int_erase_ciel)->Arg(1000);
BENCHMARK(vector_int_erase_std)->Arg(1000);
BENCHMARK(vector_tr_erase_ciel)->Arg(1000);
BENCHMARK(vector_tr_erase_std)->Arg(1000);
