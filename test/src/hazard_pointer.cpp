#include <gtest/gtest.h>

#include <ciel/core/config.hpp>
#include <ciel/hazard_pointer.hpp>
#include <ciel/inplace_vector.hpp>
#include <ciel/test/simple_latch.hpp>
#include <ciel/vector.hpp>

#include <atomic>
#include <memory>

using namespace ciel;

namespace {

struct Garbage {
    Garbage* next_{this}; // Intentionally
    int i{1};

    ~Garbage() {
        i = 0;
    }

    CIEL_NODISCARD Garbage* next() const noexcept {
        return next_;
    }

    void set_next(Garbage* n) noexcept {
        next_ = n;
    }

    void destroy() noexcept {
        delete this;
    }

}; // struct Garbage

} // namespace

TEST(hazard_pointer, singlethread) {
    constexpr size_t garbage_num = 10000;

    ciel::inplace_vector<std::atomic<Garbage*>, garbage_num> v;
    for (size_t i = 0; i < garbage_num; ++i) {
        v.unchecked_emplace_back(new Garbage);
    }

    auto& hp = hazard_pointer<Garbage>::get();
    std::atomic<size_t> count{0};
    for (std::atomic<Garbage*>& p : v) {
        Garbage* res = hp.protect(p);
        hp.retire(res);
        count += res->i;
        hp.release();
    }

    ASSERT_EQ(count, garbage_num);
}

TEST(hazard_pointer, multithread) {
    constexpr size_t threads_num    = 64;
    constexpr size_t operations_num = 10000;

    std::atomic<Garbage*> ptr{nullptr};
    SimpleLatch go{threads_num};

    ciel::vector<std::thread> store_threads;
    store_threads.reserve(threads_num / 2);
    for (size_t i = 0; i < threads_num / 2; ++i) {
        store_threads.unchecked_emplace_back([&] {
            go.arrive_and_wait();

            auto& hp = hazard_pointer<Garbage>::get();

            for (size_t j = 0; j < operations_num; ++j) {
                hp.retire(ptr.exchange(new Garbage));
            }
        });
    }

    ciel::vector<std::thread> load_threads;
    load_threads.reserve(threads_num / 2);
    for (size_t i = 0; i < threads_num / 2; ++i) {
        load_threads.unchecked_emplace_back([&] {
            go.arrive_and_wait();

            auto& hp = hazard_pointer<Garbage>::get();

            for (size_t j = 0; j < operations_num; ++j) {
                Garbage* res = hp.protect(ptr);
                if (res != nullptr) {
                    ASSERT_EQ(res->i, 1);
                }
                hp.release();
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
