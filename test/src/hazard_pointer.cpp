#include <gtest/gtest.h>

#include <ciel/core/config.hpp>
#include <ciel/hazard_pointer.hpp>
#include <ciel/inplace_vector.hpp>
#include <ciel/test/simple_latch.hpp>
#include <ciel/vector.hpp>

#include <atomic>
#include <cstddef>
#include <thread>

using namespace ciel;

namespace {

struct Garbage final : hazard_pointer_obj_base<Garbage> {
    int i{1};

    ~Garbage() {
        i = 0;
    }

}; // struct Garbage

} // namespace

TEST(hazard_pointer, singlethread) {
    constexpr size_t garbage_num = 10000;

    ciel::inplace_vector<std::atomic<Garbage*>, garbage_num> v;
    for (size_t i = 0; i < garbage_num; ++i) {
        v.unchecked_emplace_back(new Garbage);
    }

    hazard_pointer hp = make_hazard_pointer();
    std::atomic<size_t> count{0};
    for (std::atomic<Garbage*>& p : v) {
        Garbage* res = hp.protect(p);
        res->retire();
        count += res->i;
        hp.reset_protection();
    }

    ASSERT_EQ(count, garbage_num);
}

TEST(hazard_pointer, multithread) {
    constexpr size_t threads_num    = 64;
    constexpr size_t operations_num = 10000;

    std::atomic<Garbage*> ptr{nullptr};
    SimpleLatch go{threads_num};

    ciel::vector<std::thread> store_threads(reserve_capacity, threads_num / 2);
    for (size_t i = 0; i < threads_num / 2; ++i) {
        store_threads.unchecked_emplace_back([&] {
            go.arrive_and_wait();

            hazard_pointer hp = make_hazard_pointer();

            for (size_t j = 0; j < operations_num; ++j) {
                auto g = ptr.exchange(new Garbage);
                if (g) {
                    g->retire();
                }
            }
        });
    }

    ciel::vector<std::thread> load_threads(reserve_capacity, threads_num / 2);
    for (size_t i = 0; i < threads_num / 2; ++i) {
        load_threads.unchecked_emplace_back([&] {
            go.arrive_and_wait();

            hazard_pointer hp = make_hazard_pointer();

            for (size_t j = 0; j < operations_num; ++j) {
                Garbage* res = hp.protect(ptr);
                if (res != nullptr) {
                    ASSERT_EQ(res->i, 1);
                }
                hp.reset_protection();
            }
        });
    }

    for (auto& t : store_threads) {
        t.join();
    }

    for (auto& t : load_threads) {
        t.join();
    }
}
