#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <vector>

#include <ciel/type_traits.hpp>

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

    T4&
    operator=(const T4&) noexcept {
        return *this;
    }
};

struct T5 {
    T5(T5&&) noexcept {}

    T5&
    operator=(T5&&) noexcept {
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
    virtual void
    f() noexcept {}
};

struct T9 {
    T9(const T9&) noexcept {}

    T9(T9&&) noexcept {}

    T9&
    operator=(const T9&) noexcept {
        return *this;
    }

    T9&
    operator=(T9&&) noexcept {
        return *this;
    }
};

struct T10 {
    T10(const T10&) = delete;

    T10(T10&&) noexcept {}

    T10&
    operator=(const T10&)
        = delete;

    T10&
    operator=(T10&&) noexcept {
        return *this;
    }
};

struct T11 {
    T11(const T11&) = delete;
    T11(T11&&)      = delete;
    T11&
    operator=(const T11&)
        = delete;
    T11&
    operator=(T11&&)
        = delete;
};

} // namespace

TEST(type_traits_tests, worth_move_constructing) {
    static_assert(not ciel::worth_move_constructing<int>, "");
    static_assert(not ciel::worth_move_constructing<int&>, "");
    static_assert(not ciel::worth_move_constructing<int&&>, "");
    static_assert(not ciel::worth_move_constructing<int*>, "");
    static_assert(not ciel::worth_move_constructing<int[5]>, "");
    static_assert(not ciel::worth_move_constructing<std::array<int, 5>>, "");
    static_assert(not ciel::worth_move_constructing<T1>, "");
    static_assert(not ciel::worth_move_constructing<T2>, "");
    static_assert(not ciel::worth_move_constructing<T3>, "");
    static_assert(not ciel::worth_move_constructing<T4>, "");

    static_assert(ciel::worth_move_constructing<std::array<std::vector<int>, 5>>, "");
    static_assert(not ciel::worth_move_constructing<std::vector<int>[5]>, "");

    static_assert(ciel::worth_move_constructing<T5>, "");
    static_assert(ciel::worth_move_constructing<T6>, "");
    static_assert(ciel::worth_move_constructing<T7>, "");
    static_assert(ciel::worth_move_constructing<T8>, "");
    static_assert(ciel::worth_move_constructing<T9>, "");
    static_assert(ciel::worth_move_constructing<T10>, "");

    static_assert(not ciel::worth_move_constructing<T11>, "");
}

TEST(type_traits_tests, worth_move_assigning) {
    static_assert(not ciel::worth_move_assigning<int>, "");
    static_assert(not ciel::worth_move_assigning<int&>, "");
    static_assert(not ciel::worth_move_assigning<int&&>, "");
    static_assert(not ciel::worth_move_assigning<int*>, "");
    static_assert(not ciel::worth_move_assigning<int[5]>, "");
    static_assert(not ciel::worth_move_assigning<std::array<int, 5>>, "");
    static_assert(not ciel::worth_move_assigning<T1>, "");
    static_assert(not ciel::worth_move_assigning<T2>, "");
    static_assert(not ciel::worth_move_assigning<T3>, "");
    static_assert(not ciel::worth_move_assigning<T4>, "");

    static_assert(ciel::worth_move_assigning<std::array<std::vector<int>, 5>>, "");
    static_assert(not ciel::worth_move_assigning<std::vector<int>[5]>, "");

    static_assert(ciel::worth_move_assigning<T5>, "");
    static_assert(ciel::worth_move_assigning<T6>, "");
    static_assert(ciel::worth_move_assigning<T7>, "");
    static_assert(ciel::worth_move_assigning<T8>, "");
    static_assert(ciel::worth_move_assigning<T9>, "");
    static_assert(ciel::worth_move_assigning<T10>, "");

    static_assert(not ciel::worth_move_assigning<T11>, "");
}
