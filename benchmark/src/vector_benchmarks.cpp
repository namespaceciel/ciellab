#include "benchmark_config.h"

#include <ciel/small_vector.hpp>
#include <ciel/vector.hpp>
#include <vector>

// push_back
void
vector_push_back_std(benchmark::State& state) {
    for (auto _ : state) {
        push_back_benchmark<std::vector<int>>();
    }
}

void
vector_push_back_ciel(benchmark::State& state) {
    for (auto _ : state) {
        push_back_benchmark<ciel::vector<int>>();
    }
}

void
small_vector_push_back_ciel(benchmark::State& state) {
    for (auto _ : state) {
        push_back_benchmark<ciel::small_vector<int, 32>>();
    }
}

// insert
void
vector_insert_std(benchmark::State& state) {
    for (auto _ : state) {
        insert_benchmark<std::vector<int>>();
    }
}

void
vector_insert_ciel(benchmark::State& state) {
    for (auto _ : state) {
        insert_benchmark<ciel::vector<int>>();
    }
}

void
small_vector_insert_ciel(benchmark::State& state) {
    for (auto _ : state) {
        insert_benchmark<ciel::small_vector<int, 32>>();
    }
}

// erase
void
vector_erase_std(benchmark::State& state) {
    for (auto _ : state) {
        erase_benchmark<std::vector<int>>();
    }
}

void
vector_erase_ciel(benchmark::State& state) {
    for (auto _ : state) {
        erase_benchmark<ciel::vector<int>>();
    }
}

void
small_vector_erase_ciel(benchmark::State& state) {
    for (auto _ : state) {
        erase_benchmark<ciel::small_vector<int, 32>>();
    }
}

// few_objects
void
vector_few_objects_std(benchmark::State& state) {
    for (auto _ : state) {
        few_objects_benchmark<std::vector<int>>();
    }
}

void
vector_few_objects_ciel(benchmark::State& state) {
    for (auto _ : state) {
        few_objects_benchmark<ciel::vector<int>>();
    }
}

void
small_vector_few_objects_ciel(benchmark::State& state) {
    for (auto _ : state) {
        few_objects_benchmark<ciel::small_vector<int, 100>>();
    }
}

static_assert(ciel::is_trivially_relocatable<trivially_relocatable_obj>::value, "");

// trivially_relocatable_obj
void
vector_trivially_relocatable_obj_std(benchmark::State& state) {
    for (auto _ : state) {
        trivially_relocatable_obj_benchmark<std::vector<trivially_relocatable_obj>>();
    }
}

void
vector_trivially_relocatable_obj_ciel(benchmark::State& state) {
    for (auto _ : state) {
        trivially_relocatable_obj_benchmark<ciel::vector<trivially_relocatable_obj>>();
    }
}
