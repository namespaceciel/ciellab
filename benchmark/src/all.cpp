#include "benchmark_config.h"

void vector_push_back_std(benchmark::State&);
void vector_push_back_ciel(benchmark::State&);
void small_vector_push_back_ciel(benchmark::State&);

void vector_insert_std(benchmark::State&);
void vector_insert_ciel(benchmark::State&);
void small_vector_insert_ciel(benchmark::State&);

void vector_erase_std(benchmark::State&);
void vector_erase_ciel(benchmark::State&);
void small_vector_erase_ciel(benchmark::State&);

void deque_push_back_std(benchmark::State&);
void split_buffer_push_back_ciel(benchmark::State&);

void deque_push_front_std(benchmark::State&);
void split_buffer_push_front_ciel(benchmark::State&);

void deque_erase_std(benchmark::State&);
void split_buffer_erase_ciel(benchmark::State&);

void list_push_back_std(benchmark::State&);
void list_push_back_ciel(benchmark::State&);

void list_push_front_std(benchmark::State&);
void list_push_front_ciel(benchmark::State&);

void list_push_and_pop_std(benchmark::State&);
void list_push_and_pop_ciel(benchmark::State&);

void list_insert_std(benchmark::State&);
void list_insert_ciel(benchmark::State&);

void list_erase_std(benchmark::State&);
void list_erase_ciel(benchmark::State&);

BENCHMARK(vector_push_back_std);
BENCHMARK(vector_push_back_ciel);
BENCHMARK(small_vector_push_back_ciel);

BENCHMARK(vector_insert_std);
BENCHMARK(vector_insert_ciel);
BENCHMARK(small_vector_insert_ciel);

BENCHMARK(vector_erase_std);
BENCHMARK(vector_erase_ciel);
BENCHMARK(small_vector_erase_ciel);

BENCHMARK(deque_push_back_std);
BENCHMARK(split_buffer_push_back_ciel);

BENCHMARK(deque_push_front_std);
BENCHMARK(split_buffer_push_front_ciel);

BENCHMARK(deque_erase_std);
BENCHMARK(split_buffer_erase_ciel);

BENCHMARK(list_push_back_std);
BENCHMARK(list_push_back_ciel);

BENCHMARK(list_push_front_std);
BENCHMARK(list_push_front_ciel);

BENCHMARK(list_push_and_pop_std);
BENCHMARK(list_push_and_pop_ciel);

BENCHMARK(list_insert_std);
BENCHMARK(list_insert_ciel);

BENCHMARK(list_erase_std);
BENCHMARK(list_erase_ciel);

BENCHMARK_MAIN();