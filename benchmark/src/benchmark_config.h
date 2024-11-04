#ifndef CIELLAB_BENCHMARK_CONFIG_H
#define CIELLAB_BENCHMARK_CONFIG_H

#include <benchmark/benchmark.h>

#include <ciel/config.hpp>
#include <ciel/exchange.hpp>
#include <ciel/is_trivially_relocatable.hpp>

#include <algorithm>
#include <random>

#define define_benchmark(suite_name, function_name, container_name) \
    void suite_name(benchmark::State& state) {                      \
        for (auto _ : state) {                                      \
            function_name<container_name>();                        \
        }                                                           \
    }                                                               \
    BENCHMARK(suite_name)

struct TriviallyRelocatable {
    int* ptr;

    TriviallyRelocatable(int i = 0)
        : ptr(new int{i}) {}

    TriviallyRelocatable(const TriviallyRelocatable& other)
        : ptr(other.ptr ? new int{*other.ptr} : nullptr) {}

    TriviallyRelocatable(TriviallyRelocatable&& other) noexcept
        : ptr(ciel::exchange(other.ptr, nullptr)) {}

    TriviallyRelocatable&
    operator=(TriviallyRelocatable other) noexcept {
        swap(other);
        return *this;
    }

    ~TriviallyRelocatable() {
        delete ptr;
    }

    void
    swap(TriviallyRelocatable& other) noexcept {
        std::swap(ptr, other.ptr);
    }

}; // struct trivially_relocatable_obj

NAMESPACE_CIEL_BEGIN

template<>
struct is_trivially_relocatable<TriviallyRelocatable> : std::true_type {};

NAMESPACE_CIEL_END

template<class Container>
void
emplace_back() noexcept {
    Container c;

    for (int i = 0; i < 100000; ++i) {
        c.emplace_back(i);
    }
}

template<class Container>
void
emplace_back_known_size() noexcept {
    Container c;
    c.reserve(100000);

    for (int i = 0; i < 100000; ++i) {
        c.emplace_back(i);
    }
}

template<class Container>
void
unchecked_emplace_back() noexcept {
    Container c;
    c.reserve(100000);

    for (int i = 0; i < 100000; ++i) {
        c.unchecked_emplace_back(i);
    }
}

template<class Container>
void
emplace_front() noexcept {
    Container c;

    for (int i = 0; i < 100000; ++i) {
        c.emplace_front(i);
    }
}

template<class Container>
void
push_and_pop() noexcept {
    Container c;

    for (int i = 0; i < 100000; ++i) {
        c.emplace_back(i);
        c.emplace_front(i);
        c.pop_back();
        c.pop_front();
    }
}

template<class Container, class iterator = typename Container::iterator>
void
insert() noexcept {
    Container c;

    iterator it = c.begin();

    for (int i = 0; i < 1000; ++i) {
        it = c.insert(it, i);

        // Try to safely increment the iterator three times.
        if (it == c.end()) {
            it = c.begin();
        }
        if (++it == c.end()) {
            it = c.begin();
        }
        if (++it == c.end()) {
            it = c.begin();
        }
    }
}

template<class Container, class iterator = typename Container::iterator>
void
erase() noexcept {
    Container c(1000);

    iterator it = c.begin();

    for (int i = 0; i < 1000; ++i) {
        it = c.erase(it);

        // Try to safely increment the iterator three times.
        if (it == c.end()) {
            it = c.begin();
        }
        if (++it == c.end()) {
            it = c.begin();
        }
        if (++it == c.end()) {
            it = c.begin();
        }
    }
}

template<class Container>
void
few_objects_emplace_back() {
    for (int i = 0; i < 1000; ++i) {
        Container c{50, 123};

        for (int j = 0; j < 50; ++j) {
            c.emplace_back(j);
        }
    }
}

template<class Container>
void
pop_and_shrink() {
    Container c(10000);

    for (int i = 0; i < 100; ++i) {
        c.pop_back();
        c.shrink_to_fit();
    }
}

/*
// We need to make a functor for sort_benchmark since it needs an generated unsorted container
struct sort {
    uint64_t arr[100000];

    sort() noexcept {
        std::random_device rd;
        const std::mt19937_64 g(rd());
        std::generate(std::begin(arr), std::end(arr), g);
    }

    void
    operator()(void (*sort)(uint64_t*, uint64_t*)) noexcept {
        sort(std::begin(arr), std::end(arr));
    }
};

struct sorted_arr_sort {
    uint64_t arr[100000];

    sorted_arr_sort() noexcept {
        std::iota(std::begin(arr), std::end(arr), 0);
    }

    void
    operator()(void (*sort)(uint64_t*, uint64_t*)) noexcept {
        sort(std::begin(arr), std::end(arr));
    }
};

template<class Container, class value_type = typename Container::value_type>
void
set_insert() noexcept {
    Container c;
    std::random_device rd;
    std::mt19937_64 g(rd());

    for (int i = 0; i < 10000; ++i) {
        c.insert(g());
    }
}

// set insert sorted data to test balance performance
template<class Container, class value_type = typename Container::value_type>
void
set_sorted_insert() noexcept {
    Container c;

    for (uint64_t i = 0; i < 10000ULL; ++i) {
        c.insert(i);
    }
}

template<class Set, class value_type = typename Set::value_type>
void
set_find(Set& s) noexcept {
    std::random_device rd;
    std::mt19937_64 g(rd());

    for (int i = 0; i < 100000; ++i) {
        CIEL_UNUSED(s.find(g()));
    }
}

template<class Set, class iterator = typename Set::iterator>
void
set_erase(Set s) noexcept {
    iterator it = s.begin();

    for (int i = 0; i < 1000; ++i) {
        it = s.erase(it);

        // Try to safely increment the iterator three times.
        if (it == s.end()) {
            it = s.begin();
        }
        if (++it == s.end()) {
            it = s.begin();
        }
        if (++it == s.end()) {
            it = s.begin();
        }
    }
}

template<class Set>
void
set_erase_value(Set s) noexcept {
    std::random_device rd;
    std::mt19937_64 g(rd());

    for (int i = 0; i < 1000; ++i) {
        s.erase(g() % 10000);
    }
}
*/

#endif // CIELLAB_BENCHMARK_CONFIG_H
