#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"
#include "folly/container/F14Map.h"
#include "hashtable/bytell_hash_map.hpp"
#include "hashtable/flat_hash_map.hpp"
#include "hashtable/unordered_map.hpp"
#include <benchmark/benchmark.h>
#include <unordered_map>

#define BENCH_MACRO(name, range)                                    \
    static void name##_std(benchmark::State& state) {               \
        bench_##name##_impl<std::unordered_map<int, int>>(state);   \
    }                                                               \
    static void name##_ska(benchmark::State& state) {               \
        bench_##name##_impl<ska::unordered_map<int, int>>(state);   \
    }                                                               \
    static void name##_bytell(benchmark::State& state) {            \
        bench_##name##_impl<ska::bytell_hash_map<int, int>>(state); \
    }                                                               \
    static void name##_flat(benchmark::State& state) {              \
        bench_##name##_impl<ska::flat_hash_map<int, int>>(state);   \
    }                                                               \
    static void name##_absl_flat(benchmark::State& state) {         \
        bench_##name##_impl<absl::flat_hash_map<int, int>>(state);  \
    }                                                               \
    static void name##_absl_node(benchmark::State& state) {         \
        bench_##name##_impl<absl::node_hash_map<int, int>>(state);  \
    }                                                               \
    static void name##_folly_flat(benchmark::State& state) {        \
        bench_##name##_impl<folly::F14FastMap<int, int>>(state);    \
    }                                                               \
    static void name##_folly_node(benchmark::State& state) {        \
        bench_##name##_impl<folly::F14NodeMap<int, int>>(state);    \
    }                                                               \
    BENCHMARK(name##_std)->Arg(range);                              \
    BENCHMARK(name##_ska)->Arg(range);                              \
    BENCHMARK(name##_absl_node)->Arg(range);                        \
    BENCHMARK(name##_folly_node)->Arg(range);                       \
    BENCHMARK(name##_bytell)->Arg(range);                           \
    BENCHMARK(name##_flat)->Arg(range);                             \
    BENCHMARK(name##_absl_flat)->Arg(range);                        \
    BENCHMARK(name##_folly_flat)->Arg(range)

template<class Container>
void bench_insert_impl(benchmark::State& state) {
    const auto range = state.range(0);

    for (auto _ : state) {
        Container v;

        for (int i = 0; i < range; ++i) {
            v.insert(std::make_pair(i, i));
        }

        benchmark::DoNotOptimize(v);
    }
}

BENCH_MACRO(insert, 10000);

template<class Container>
void bench_found_impl(benchmark::State& state) {
    const auto range = state.range(0);

    Container v;
    for (int i = 0; i < range; ++i) {
        v.insert(std::make_pair(i, i));
    }
    benchmark::DoNotOptimize(v);

    for (auto _ : state) {
        for (int i = 0; i < range; ++i) {
            auto it = v.find(i);
            benchmark::DoNotOptimize(it);
        }
    }
}

BENCH_MACRO(found, 10000);

template<class Container>
void bench_not_found_impl(benchmark::State& state) {
    const auto range = state.range(0);

    Container v;
    for (int i = 0; i < range; ++i) {
        v.insert(std::make_pair(i, i));
    }
    benchmark::DoNotOptimize(v);

    for (auto _ : state) {
        for (int i = 0; i < range; ++i) {
            auto it = v.find(i + range);
            benchmark::DoNotOptimize(it);
        }
    }
}

BENCH_MACRO(not_found, 10000);
