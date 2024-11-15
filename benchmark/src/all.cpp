#include "benchmark_config.h"

#include <ciel/vector.hpp>
#include <vector>

#include <ciel/experimental/list.hpp>
#include <list>

define_benchmark(emplace_back_std_vector_int, emplace_back, std::vector<int>);
define_benchmark(emplace_back_ciel_vector_int, emplace_back, ciel::vector<int>);

define_benchmark(emplace_back_known_size_std_vector_int, emplace_back_known_size, std::vector<int>);
define_benchmark(emplace_back_known_size_ciel_vector_int, emplace_back_known_size, ciel::vector<int>);
define_benchmark(unchecked_emplace_back_ciel_vector_int, unchecked_emplace_back, ciel::vector<int>);

define_benchmark(emplace_back_std_vector_trivially_relocatable, emplace_back, std::vector<TriviallyRelocatable>);
define_benchmark(emplace_back_ciel_vector_trivially_relocatable, emplace_back, ciel::vector<TriviallyRelocatable>);

define_benchmark(insert_std_vector_int, insert, std::vector<int>);
define_benchmark(insert_ciel_vector_int, insert, ciel::vector<int>);

define_benchmark(insert_std_vector_trivially_relocatable, insert, std::vector<TriviallyRelocatable>);
define_benchmark(insert_ciel_vector_trivially_relocatable, insert, ciel::vector<TriviallyRelocatable>);

define_benchmark(erase_std_vector_int, erase, std::vector<int>);
define_benchmark(erase_ciel_vector_int, erase, ciel::vector<int>);

define_benchmark(erase_std_vector_trivially_relocatable, erase, std::vector<TriviallyRelocatable>);
define_benchmark(erase_ciel_vector_trivially_relocatable, erase, ciel::vector<TriviallyRelocatable>);

define_benchmark(pop_and_shrink_emplace_back_std_vector_int, pop_and_shrink, std::vector<int>);
define_benchmark(pop_and_shrink_emplace_back_ciel_vector_int, pop_and_shrink, ciel::vector<int>);

define_benchmark(pop_and_shrink_emplace_back_std_vector_trivially_relocatable, pop_and_shrink,
                 std::vector<TriviallyRelocatable>);
define_benchmark(pop_and_shrink_emplace_back_ciel_vector_trivially_relocatable, pop_and_shrink,
                 ciel::vector<TriviallyRelocatable>);

define_benchmark(emplace_back_std_list_int, emplace_back, std::list<int>);
define_benchmark(emplace_back_ciel_list_int, emplace_back, ciel::list<int>);

define_benchmark(emplace_front_std_list_int, emplace_front, std::list<int>);
define_benchmark(emplace_front_ciel_list_int, emplace_front, ciel::list<int>);

define_benchmark(push_and_pop_std_list_int, push_and_pop, std::list<int>);
define_benchmark(push_and_pop_ciel_list_int, push_and_pop, ciel::list<int>);

define_benchmark(insert_std_list_int, insert, std::list<int>);
define_benchmark(insert_ciel_list_int, insert, ciel::list<int>);

define_benchmark(erase_std_list_int, erase, std::list<int>);
define_benchmark(erase_ciel_list_int, erase, ciel::list<int>);

BENCHMARK_MAIN();
