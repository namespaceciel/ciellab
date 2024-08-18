#include "benchmark_config.h"

#include <ciel/small_vector.hpp>
#include <ciel/vector.hpp>
#include <vector>

#include <ciel/split_buffer.hpp>
#include <deque>

#include <ciel/list.hpp>
#include <list>

define_benchmark(push_back_std_vector_int, push_back, std::vector<int>);
define_benchmark(push_back_ciel_vector_int, push_back, ciel::vector<int>);
define_benchmark(push_back_ciel_small_vector_int, push_back, ciel::small_vector<int>);

define_benchmark(push_back_std_vector_trivially_relocatable, push_back, std::vector<TriviallyRelocatable>);
define_benchmark(push_back_ciel_vector_trivially_relocatable, push_back, ciel::vector<TriviallyRelocatable>);
define_benchmark(push_back_ciel_small_vector_trivially_relocatable, push_back,
                 ciel::small_vector<TriviallyRelocatable>);

define_benchmark(insert_std_vector_int, insert, std::vector<int>);
define_benchmark(insert_ciel_vector_int, insert, ciel::vector<int>);
define_benchmark(insert_ciel_small_vector_int, insert, ciel::small_vector<int>);

define_benchmark(insert_std_vector_trivially_relocatable, insert, std::vector<TriviallyRelocatable>);
define_benchmark(insert_ciel_vector_trivially_relocatable, insert, ciel::vector<TriviallyRelocatable>);
define_benchmark(insert_ciel_small_vector_trivially_relocatable, insert, ciel::small_vector<TriviallyRelocatable>);

define_benchmark(erase_std_vector_int, erase, std::vector<int>);
define_benchmark(erase_ciel_vector_int, erase, ciel::vector<int>);
define_benchmark(erase_ciel_small_vector_int, erase, ciel::small_vector<int>);

define_benchmark(erase_std_vector_trivially_relocatable, erase, std::vector<TriviallyRelocatable>);
define_benchmark(erase_ciel_vector_trivially_relocatable, erase, ciel::vector<TriviallyRelocatable>);
define_benchmark(erase_ciel_small_vector_trivially_relocatable, erase, ciel::small_vector<TriviallyRelocatable>);

define_benchmark(few_objects_push_back_std_vector_int, few_objects_push_back, std::vector<int>);
define_benchmark(few_objects_push_back_ciel_vector_int, few_objects_push_back, ciel::vector<int>);
using ciel_small_vector_int_100 = ciel::small_vector<int, 100>;
define_benchmark(few_objects_push_back_ciel_small_vector_int, few_objects_push_back, ciel_small_vector_int_100);

define_benchmark(few_objects_push_back_std_vector_trivially_relocatable, few_objects_push_back,
                 std::vector<TriviallyRelocatable>);
define_benchmark(few_objects_push_back_ciel_vector_trivially_relocatable, few_objects_push_back,
                 ciel::vector<TriviallyRelocatable>);
define_benchmark(few_objects_push_back_ciel_small_vector_trivially_relocatable, few_objects_push_back,
                 ciel::small_vector<TriviallyRelocatable>);

define_benchmark(pop_and_shrink_push_back_std_vector_int, pop_and_shrink, std::vector<int>);
define_benchmark(pop_and_shrink_push_back_ciel_vector_int, pop_and_shrink, ciel::vector<int>);

define_benchmark(pop_and_shrink_push_back_std_vector_trivially_relocatable, pop_and_shrink,
                 std::vector<TriviallyRelocatable>);
define_benchmark(pop_and_shrink_push_back_ciel_vector_trivially_relocatable, pop_and_shrink,
                 ciel::vector<TriviallyRelocatable>);

define_benchmark(push_back_std_deque_int, push_back, std::deque<int>);
define_benchmark(push_back_ciel_split_buffer_int, push_back, ciel::split_buffer<int>);

define_benchmark(push_back_std_deque_trivially_relocatable, push_back, std::deque<TriviallyRelocatable>);
define_benchmark(push_back_ciel_split_buffer_trivially_relocatable, push_back,
                 ciel::split_buffer<TriviallyRelocatable>);

define_benchmark(push_front_std_deque_int, push_front, std::deque<int>);
define_benchmark(push_front_ciel_split_buffer_int, push_front, ciel::split_buffer<int>);

define_benchmark(push_front_std_deque_trivially_relocatable, push_front, std::deque<TriviallyRelocatable>);
define_benchmark(push_front_ciel_split_buffer_trivially_relocatable, push_front,
                 ciel::split_buffer<TriviallyRelocatable>);

// TODO: insert and erase

define_benchmark(push_back_std_list_int, push_back, std::list<int>);
define_benchmark(push_back_ciel_list_int, push_back, ciel::list<int>);

define_benchmark(push_front_std_list_int, push_front, std::list<int>);
define_benchmark(push_front_ciel_list_int, push_front, ciel::list<int>);

define_benchmark(push_and_pop_std_list_int, push_and_pop, std::list<int>);
define_benchmark(push_and_pop_ciel_list_int, push_and_pop, ciel::list<int>);

define_benchmark(insert_std_list_int, insert, std::list<int>);
define_benchmark(insert_ciel_list_int, insert, ciel::list<int>);

define_benchmark(erase_std_list_int, erase, std::list<int>);
define_benchmark(erase_ciel_list_int, erase, ciel::list<int>);

BENCHMARK_MAIN();
