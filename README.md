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
-----------------------------------------------------------------------
Benchmark                             Time             CPU   Iterations
-----------------------------------------------------------------------
vector_push_back_std              43035 ns        42987 ns        16290
vector_push_back_ciel             42915 ns        42856 ns        16511
small_vector_push_back_ciel       43802 ns        43717 ns        15943

vector_insert_std                 16376 ns        16327 ns        42344
vector_insert_ciel                14449 ns        14412 ns        48577
small_vector_insert_ciel          15408 ns        15286 ns        45609

vector_erase_std                  17510 ns        17459 ns        40000
vector_erase_ciel                 17816 ns        17768 ns        39430
small_vector_erase_ciel           19141 ns        19089 ns        36888

deque_push_back_std               80047 ns        79868 ns         8758
split_buffer_push_back_ciel       42733 ns        42684 ns        16234

deque_push_front_std             211442 ns       210942 ns         3282
split_buffer_push_front_ciel      57376 ns        57247 ns        11971

deque_erase_std                   25160 ns        25105 ns        27744
split_buffer_erase_ciel            9931 ns         9853 ns        71777

list_push_back_std              1985437 ns      1980059 ns          357
list_push_back_ciel             2094203 ns      2088030 ns          330

list_push_front_std             1978094 ns      1972454 ns          355
list_push_front_ciel            2106473 ns      2101332 ns          331

list_push_and_pop_std           3145399 ns      3138435 ns          223
list_push_and_pop_ciel          1106886 ns      1104616 ns          636

list_insert_std                   18291 ns        18242 ns        38779
list_insert_ciel                  20101 ns        20052 ns        34927

list_erase_std                    18709 ns        18661 ns        37149
list_erase_ciel                   23910 ns        23856 ns        29317
```

### gcc 13.2.0 -O3

```
-----------------------------------------------------------------------
Benchmark                             Time             CPU   Iterations
-----------------------------------------------------------------------
vector_push_back_std              42967 ns        42914 ns        16276
vector_push_back_ciel             43139 ns        43063 ns        16136
small_vector_push_back_ciel       44102 ns        43981 ns        15858

vector_insert_std                 12237 ns        12205 ns        56028
vector_insert_ciel                12642 ns        12609 ns        56152
small_vector_insert_ciel          14324 ns        14288 ns        48540

vector_erase_std                  17996 ns        17942 ns        38851
vector_erase_ciel                 17881 ns        17816 ns        35627
small_vector_erase_ciel           19018 ns        18959 ns        37768

deque_push_back_std               80554 ns        80320 ns         8796
split_buffer_push_back_ciel       42962 ns        42890 ns        16326

deque_push_front_std              75310 ns        75082 ns         9325
split_buffer_push_front_ciel      42703 ns        42605 ns        16549

deque_erase_std                   25601 ns        25420 ns        27763
split_buffer_erase_ciel            9155 ns         9132 ns        75627

list_push_back_std              2097107 ns      2091015 ns          336
list_push_back_ciel             2036256 ns      2030075 ns          348

list_push_front_std             2096603 ns      2091806 ns          335
list_push_front_ciel            2049332 ns      2044451 ns          346

list_push_and_pop_std           3597763 ns      3588103 ns          195
list_push_and_pop_ciel           800034 ns       797859 ns          884

list_insert_std                   20694 ns        20458 ns        31861
list_insert_ciel                  17931 ns        17869 ns        37351

list_erase_std                    20932 ns        20856 ns        33090
list_erase_ciel                   23210 ns        23140 ns        30163
```