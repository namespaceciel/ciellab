#include "benchmark_config.h"

#include <ciel/list.hpp>
#include <list>

// push_back
void
list_push_back_std(benchmark::State& state) {
    for (auto _ : state) {
        push_back_benchmark<std::list<int>>();
    }
}

void
list_push_back_ciel(benchmark::State& state) {
    for (auto _ : state) {
        push_back_benchmark<ciel::list<int>>();
    }
}

// push_front
void
list_push_front_std(benchmark::State& state) {
    for (auto _ : state) {
        push_front_benchmark<std::list<int>>();
    }
}

void
list_push_front_ciel(benchmark::State& state) {
    for (auto _ : state) {
        push_front_benchmark<ciel::list<int>>();
    }
}

// push_and_pop
void
list_push_and_pop_std(benchmark::State& state) {
    for (auto _ : state) {
        push_and_pop_benchmark<std::list<int>>();
    }
}

void
list_push_and_pop_ciel(benchmark::State& state) {
    for (auto _ : state) {
        push_and_pop_benchmark<ciel::list<int>>();
    }
}

// insert
void
list_insert_std(benchmark::State& state) {
    for (auto _ : state) {
        insert_benchmark<std::list<int>>();
    }
}

void
list_insert_ciel(benchmark::State& state) {
    for (auto _ : state) {
        insert_benchmark<ciel::list<int>>();
    }
}

// erase
void
list_erase_std(benchmark::State& state) {
    for (auto _ : state) {
        erase_benchmark<std::list<int>>();
    }
}

void
list_erase_ciel(benchmark::State& state) {
    for (auto _ : state) {
        erase_benchmark<ciel::list<int>>();
    }
}
