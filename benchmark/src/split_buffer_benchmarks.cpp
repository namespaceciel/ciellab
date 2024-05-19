#include "benchmark_config.h"

#include <ciel/split_buffer.hpp>
#include <deque>

// push_back
void
deque_push_back_std(benchmark::State& state) {
    for (auto _ : state) {
        push_back_benchmark<std::deque<int>>();
    }
}

void
split_buffer_push_back_ciel(benchmark::State& state) {
    for (auto _ : state) {
        push_back_benchmark<ciel::split_buffer<int>>();
    }
}

// push_front
void
deque_push_front_std(benchmark::State& state) {
    for (auto _ : state) {
        push_front_benchmark<std::deque<int>>();
    }
}

void
split_buffer_push_front_ciel(benchmark::State& state) {
    for (auto _ : state) {
        push_front_benchmark<ciel::split_buffer<int>>();
    }
}

// erase
void
deque_erase_std(benchmark::State& state) {
    for (auto _ : state) {
        erase_benchmark<std::deque<int>>();
    }
}

void
split_buffer_erase_ciel(benchmark::State& state) {
    for (auto _ : state) {
        erase_benchmark<ciel::split_buffer<int>>();
    }
}
