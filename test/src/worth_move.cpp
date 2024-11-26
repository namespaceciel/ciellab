#include <gtest/gtest.h>

#include <ciel/experimental/worth_move.hpp>

#include <array>
#include <memory>
#include <vector>

using namespace ciel;

namespace {

struct T1 {};

struct T2 {
    int i;
};

struct T3 {
    // user-defined destructor suppress implicitly-declared move constructor and assignments
    virtual ~T3() {}
};

struct T4 {
    T4(const T4&) noexcept {}

    T4& operator=(const T4&) noexcept {
        return *this;
    }
};

struct T5 {
    T5(T5&&) noexcept {}

    T5& operator=(T5&&) noexcept {
        return *this;
    }
};

struct T6 {
    std::vector<int> v;
};

struct T7 {
    std::unique_ptr<int> v;
};

struct T8 {
    virtual void f() noexcept {}
};

struct T9 {
    T9(const T9&) noexcept {}

    T9(T9&&) noexcept {}

    T9& operator=(const T9&) noexcept {
        return *this;
    }

    T9& operator=(T9&&) noexcept {
        return *this;
    }
};

struct T10 {
    T10(T10&&) noexcept {}

    T10& operator=(T10&&) noexcept {
        return *this;
    }

    T10(const T10&)            = delete;
    T10& operator=(const T10&) = delete;
};

struct T11 {
    T11(const T11&)            = delete;
    T11(T11&&)                 = delete;
    T11& operator=(const T11&) = delete;
    T11& operator=(T11&&)      = delete;
};

} // namespace

TEST(worth_move, worth_move_constructing) {
    static_assert(not worth_move_constructing<int>::value, "");
    static_assert(not worth_move_constructing<int&>::value, "");
    static_assert(not worth_move_constructing<int&&>::value, "");
    static_assert(not worth_move_constructing<int*>::value, "");
    static_assert(not worth_move_constructing<int[5]>::value, "");
    static_assert(not worth_move_constructing<std::array<int, 5>>::value, "");
    static_assert(not worth_move_constructing<T1>::value, "");
    static_assert(not worth_move_constructing<T2>::value, "");
    static_assert(not worth_move_constructing<T3>::value, "");
    static_assert(not worth_move_constructing<T4>::value, "");

    static_assert(worth_move_constructing<std::array<std::vector<int>, 5>>::value, "");
    static_assert(not worth_move_constructing<std::vector<int>[5]>::value, "");

    static_assert(worth_move_constructing<T5>::value, "");
    static_assert(worth_move_constructing<T6>::value, "");
    static_assert(worth_move_constructing<T7>::value, "");
    static_assert(worth_move_constructing<T8>::value, "");
    static_assert(worth_move_constructing<T9>::value, "");
    static_assert(worth_move_constructing<T10>::value, "");

    static_assert(not worth_move_constructing<T11>::value, "");
}

TEST(worth_move, worth_move_assigning) {
    static_assert(not worth_move_assigning<int>::value, "");
    static_assert(not worth_move_assigning<int&>::value, "");
    static_assert(not worth_move_assigning<int&&>::value, "");
    static_assert(not worth_move_assigning<int*>::value, "");
    static_assert(not worth_move_assigning<int[5]>::value, "");
    static_assert(not worth_move_assigning<std::array<int, 5>>::value, "");
    static_assert(not worth_move_assigning<T1>::value, "");
    static_assert(not worth_move_assigning<T2>::value, "");
    static_assert(not worth_move_assigning<T3>::value, "");
    static_assert(not worth_move_assigning<T4>::value, "");

    static_assert(worth_move_assigning<std::array<std::vector<int>, 5>>::value, "");
    static_assert(not worth_move_assigning<std::vector<int>[5]>::value, "");

    static_assert(worth_move_assigning<T5>::value, "");
    static_assert(worth_move_assigning<T6>::value, "");
    static_assert(worth_move_assigning<T7>::value, "");
    static_assert(worth_move_assigning<T8>::value, "");
    static_assert(worth_move_assigning<T9>::value, "");
    static_assert(worth_move_assigning<T10>::value, "");

    static_assert(not worth_move_assigning<T11>::value, "");
}
