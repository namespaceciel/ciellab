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
vector_push_back_std              42887 ns        42732 ns        16335
vector_push_back_ciel             43595 ns        43438 ns        16064
small_vector_push_back_ciel       44254 ns        44117 ns        15780

vector_insert_std                 16255 ns        16186 ns        43256
vector_insert_ciel                14366 ns        14314 ns        48924
small_vector_insert_ciel          15231 ns        15164 ns        46272

vector_erase_std                  17488 ns        17424 ns        40206
vector_erase_ciel                 17748 ns        17660 ns        39583
small_vector_erase_ciel           19148 ns        19033 ns        36936

deque_push_back_std               79451 ns        79097 ns         8812
split_buffer_push_back_ciel       43684 ns        43478 ns        16083

deque_push_front_std             213435 ns       212511 ns         3291
split_buffer_push_front_ciel      58802 ns        58541 ns        12068

deque_erase_std                   25215 ns        25115 ns        27902
split_buffer_erase_ciel            9843 ns         9803 ns        71736
```

### gcc 13.2.0 -O3

```
-----------------------------------------------------------------------
Benchmark                             Time             CPU   Iterations
-----------------------------------------------------------------------
vector_push_back_std              43774 ns        43579 ns        16050
vector_push_back_ciel             43732 ns        43565 ns        16008
small_vector_push_back_ciel       44519 ns        44370 ns        15765

vector_insert_std                 12225 ns        12162 ns        57452
vector_insert_ciel                12559 ns        12500 ns        56069
small_vector_insert_ciel          14219 ns        14152 ns        49452

vector_erase_std                  17430 ns        17339 ns        40400
vector_erase_ciel                 17383 ns        17303 ns        40396
small_vector_erase_ciel           18703 ns        18580 ns        37763

deque_push_back_std               80206 ns        79870 ns         8757
split_buffer_push_back_ciel       43907 ns        43745 ns        16021

deque_push_front_std              75780 ns        75495 ns         9230
split_buffer_push_front_ciel      42600 ns        42478 ns        16375

deque_erase_std                   25143 ns        25045 ns        27682
split_buffer_erase_ciel            8961 ns         8931 ns        78582
```