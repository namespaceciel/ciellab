# CielLab

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

This library is header only, requiring at least C++11.

## Third Party Notices

We use GoogleTest for unit testing, Google Benchmark for benchmarking and comparisons.

Check out ThirdPartyNotices.txt for information.

## Benchmark

Note that benchmark results may vary between different compilers and compile options. Compiler optimizations and resulting code generation may coincidencally favor one kind of implementation over another, often when they are visually virtually identical.

The following temporary results run on:

```
Macbook Air M1 macOS Sonoma 14.2.1
8 X 24 MHz CPU s
CPU Caches:
L1 Data 64 KiB
L1 Instruction 128 KiB
L2 Unified 4096 KiB (x8)
```

### clang 17.0.6 -O3

```
--------------------------------------------------------------------------------
Benchmark                                      Time             CPU   Iterations
--------------------------------------------------------------------------------
vector_push_back_std                       42769 ns        42699 ns        16359
vector_push_back_ciel                      43092 ns        43030 ns        16321
small_vector_push_back_ciel                43744 ns        43651 ns        15847

vector_insert_std                          16136 ns        16085 ns        42913
vector_insert_ciel                         14226 ns        14187 ns        49474
small_vector_insert_ciel                   15140 ns        15106 ns        46578

vector_erase_std                           17484 ns        17439 ns        39838
vector_erase_ciel                          17736 ns        17680 ns        39904
small_vector_erase_ciel                    19124 ns        19063 ns        37146

vector_few_objects_std                    133778 ns       133371 ns         5275
vector_few_objects_ciel                   128253 ns       127859 ns         5203
small_vector_few_objects_ciel              17387 ns        17339 ns        40544

vector_trivially_relocatable_obj_std     8549512 ns      8530036 ns           83
vector_trivially_relocatable_obj_ciel    3026774 ns      3022392 ns          227

deque_push_back_std                        79794 ns        79577 ns         8772
split_buffer_push_back_ciel                58080 ns        57942 ns        11864

deque_push_front_std                      211574 ns       210932 ns         3320
split_buffer_push_front_ciel               73042 ns        72851 ns         9534

deque_erase_std                            25133 ns        25058 ns        28094
split_buffer_erase_ciel                    10183 ns        10159 ns        65757

list_push_back_std                       1962017 ns      1957221 ns          353
list_push_back_ciel                      2086146 ns      2082452 ns          334

list_push_front_std                      1965522 ns      1960747 ns          360
list_push_front_ciel                     2083311 ns      2077101 ns          335

list_push_and_pop_std                    3138169 ns      3131321 ns          224
list_push_and_pop_ciel                   1107015 ns      1103760 ns          633

list_insert_std                            17902 ns        17850 ns        40272
list_insert_ciel                           20189 ns        20130 ns        35523

list_erase_std                             18073 ns        18022 ns        37504
list_erase_ciel                            24218 ns        24151 ns        29067
```

### gcc 13.2.0 -O3

```
--------------------------------------------------------------------------------
Benchmark                                      Time             CPU   Iterations
--------------------------------------------------------------------------------
vector_push_back_std                       42949 ns        42878 ns        16334
vector_push_back_ciel                      42854 ns        42783 ns        16272
small_vector_push_back_ciel                43827 ns        43731 ns        15959

vector_insert_std                          12343 ns        12311 ns        56616
vector_insert_ciel                         12650 ns        12604 ns        54285
small_vector_insert_ciel                   14396 ns        14355 ns        49287

vector_erase_std                           17258 ns        17198 ns        40730
vector_erase_ciel                          17703 ns        17638 ns        39669
small_vector_erase_ciel                    18478 ns        18421 ns        37928

vector_few_objects_std                    128878 ns       128498 ns         5403
vector_few_objects_ciel                   125803 ns       125465 ns         5603
small_vector_few_objects_ciel              17607 ns        17560 ns        39872

vector_trivially_relocatable_obj_std    17492002 ns     17450025 ns           40
vector_trivially_relocatable_obj_ciel    3177720 ns      3172706 ns          218

deque_push_back_std                        79012 ns        78762 ns         8870
split_buffer_push_back_ciel                57344 ns        57218 ns        11954

deque_push_front_std                       75021 ns        74840 ns         9327
split_buffer_push_front_ciel               56711 ns        56591 ns        12304

deque_erase_std                            24958 ns        24906 ns        27908
split_buffer_erase_ciel                     8961 ns         8934 ns        78768

list_push_back_std                       2088266 ns      2083168 ns          334
list_push_back_ciel                      2007246 ns      1999934 ns          346

list_push_front_std                      2105095 ns      2098157 ns          338
list_push_front_ciel                     2045755 ns      2038270 ns          344

list_push_and_pop_std                    3568076 ns      3556340 ns          197
list_push_and_pop_ciel                    793893 ns       791424 ns          884

list_insert_std                            19305 ns        19247 ns        36992
list_insert_ciel                           17606 ns        17552 ns        39257

list_erase_std                             20500 ns        20443 ns        33998
list_erase_ciel                            23160 ns        23097 ns        30379
```