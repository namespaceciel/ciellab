#ifndef CIELUTILS_BENCHMARK_CONFIG_H
#define CIELUTILS_BENCHMARK_CONFIG_H

#include <benchmark/benchmark.h>

#include <algorithm>
#include <random>

template<class Container, class value_type = typename Container::value_type>
void push_back_benchmark() noexcept {
    Container c;

    for (int i = 0; i < 100000; ++i) {
        c.push_back(i);
    }
}

template<class Container, class value_type = typename Container::value_type>
void push_front_benchmark() noexcept {
    Container c;

    for (int i = 0; i < 100000; ++i) {
        c.push_front(i);
    }
}

template<class Container, class value_type = typename Container::value_type, class iterator = typename Container::iterator>
void insert_benchmark() noexcept {
    Container c;

    iterator it = c.begin();

    for (int i = 0; i < 1000; ++i) {
        it = c.insert(it, i);

        // Try to safely increment the iterator three times.
        if(it == c.end()) {
            it = c.begin();
        }
        if(++it == c.end()) {
            it = c.begin();
        }
        if(++it == c.end()) {
            it = c.begin();
        }
    }
}

template<class Container, class iterator = typename Container::iterator>
void erase_benchmark() noexcept {
    Container c(1000);

    iterator it = c.begin();

    for (int i = 0;  i < 1000; ++i) {
        it = c.erase(it);

        // Try to safely increment the iterator three times.
        if(it == c.end()) {
            it = c.begin();
        }
        if(++it == c.end()) {
            it = c.begin();
        }
        if(++it == c.end()) {
            it = c.begin();
        }
    }
}

// We need to make a functor for sort_benchmark since it needs an generated unsorted container
struct sort_benchmark {
    uint64_t arr[100000];

    sort_benchmark() noexcept {
        std::random_device rd;
        const std::mt19937_64 g(rd());
        std::generate(std::begin(arr), std::end(arr), g);
    }

    auto operator()(void(*sort)(uint64_t*, uint64_t*)) noexcept -> void {
        sort(std::begin(arr), std::end(arr));
    }
};

struct sorted_arr_sort_benchmark {
    uint64_t arr[100000];

    sorted_arr_sort_benchmark() noexcept {
        std::iota(std::begin(arr), std::end(arr), 0);
    }

    auto operator()(void(*sort)(uint64_t*, uint64_t*)) noexcept -> void {
        sort(std::begin(arr), std::end(arr));
    }
};

template<class Container, class value_type = typename Container::value_type>
void set_insert_benchmark() noexcept {
    Container c;
    std::random_device rd;
    std::mt19937_64 g(rd());

    for (int i = 0; i < 10000; ++i) {
        c.insert(g());
    }
}

// set insert sorted data to test balance performance
template<class Container, class value_type = typename Container::value_type>
void set_sorted_insert_benchmark() noexcept {
    Container c;

    for (uint64_t i = 0; i < 10000ULL; ++i) {
        c.insert(i);
    }
}

template<class Set, class value_type = typename Set::value_type>
void set_find_benchmark(Set& s) noexcept {
    std::random_device rd;
    std::mt19937_64 g(rd());

    for (int i = 0; i < 100000; ++i) {
        (void)s.find(g());
    }
}

template<class Set, class iterator = typename Set::iterator>
void set_erase_benchmark(Set s) noexcept {
    iterator it = s.begin();

    for (int i = 0;  i < 1000; ++i) {
        it = s.erase(it);

        // Try to safely increment the iterator three times.
        if(it == s.end()) {
            it = s.begin();
        }
        if(++it == s.end()) {
            it = s.begin();
        }
        if(++it == s.end()) {
            it = s.begin();
        }
    }
}

template<class Set>
void set_erase_value_benchmark(Set s) noexcept {
    std::random_device rd;
    std::mt19937_64 g(rd());

    for (int i = 0;  i < 1000; ++i) {
        s.erase(g() % 10000);
    }
}

#endif // CIELUTILS_BENCHMARK_CONFIG_H