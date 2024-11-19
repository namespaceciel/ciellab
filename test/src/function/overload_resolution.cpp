#include <gtest/gtest.h>

#include <ciel/function.hpp>

#include <functional>

using namespace ciel;

namespace {

struct T1 {
    int operator()() noexcept {
        return 1;
    }

    int operator()() const noexcept {
        return 2;
    }

    int operator()() volatile noexcept {
        return 3;
    }

    int operator()() const volatile noexcept {
        return 4;
    }
};

struct T2 {
    int operator()() const& {
        return 0;
    }

    int operator()() const&& {
        return 1;
    }
};

struct T3 {
    int operator()() const {
        return 0;
    }

    int operator&() const {
        return 1;
    }
};

} // namespace

TEST(function, overload_resolution) {
    T1 t1;
    const T1 t2;
    volatile T1 t3;
    const volatile T1 t4;

    const function<int()> f1(std::ref(t1));
    ASSERT_EQ(f1(), 1);

    const function<int()> f2(std::ref(t2));
    ASSERT_EQ(f2(), 2);

    const function<int()> f3(std::ref(t3));
    ASSERT_EQ(f3(), 3);

    const function<int()> f4(std::ref(t4));
    ASSERT_EQ(f4(), 4);
}

TEST(function, overload_resolution_2) {
    const T2 t;

    const function<int()> f1(t);
    ASSERT_EQ(f1(), 0);

    const function<int()> f2(T2{});
    ASSERT_EQ(f2(), 0);
}

TEST(function, overload_resolution_3) {
    const T3 t;

    const function<int()> f(t);
    ASSERT_EQ(f(), 0);
}
