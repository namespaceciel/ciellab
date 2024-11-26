#include <gtest/gtest.h>

#include <ciel/inplace_vector.hpp>

#include <memory>
#include <mutex>
#include <type_traits>

using namespace ciel;

namespace {

struct Trivial {};

struct NotTrivial {
    NotTrivial() = default;

    NotTrivial(const NotTrivial&) noexcept {}

    NotTrivial(NotTrivial&&) noexcept {}

    NotTrivial& operator=(const NotTrivial&) noexcept {
        return *this;
    }

    NotTrivial& operator=(NotTrivial&&) noexcept {
        return *this;
    }

    ~NotTrivial() {}
};

struct NotTriviallyAssignable {
    NotTriviallyAssignable()                              = default;
    NotTriviallyAssignable(const NotTriviallyAssignable&) = default;
    NotTriviallyAssignable(NotTriviallyAssignable&&)      = default;

    NotTriviallyAssignable& operator=(const NotTriviallyAssignable&) noexcept {
        return *this;
    }

    NotTriviallyAssignable& operator=(NotTriviallyAssignable&&) noexcept {
        return *this;
    }
};

struct NotNothrow {
    NotNothrow() = default;

    NotNothrow(const NotNothrow&) {}

    NotNothrow(NotNothrow&&) {}

    NotNothrow& operator=(const NotNothrow&) {
        return *this;
    }

    NotNothrow& operator=(NotNothrow&&) {
        return *this;
    }
};

} // namespace

TEST(inplace_vector, trivial) {
    static_assert(std::is_trivially_copy_constructible<inplace_vector<Trivial, 8>>::value, "");
    static_assert(std::is_trivially_move_constructible<inplace_vector<Trivial, 8>>::value, "");
    static_assert(std::is_trivially_copy_assignable<inplace_vector<Trivial, 8>>::value, "");
    static_assert(std::is_trivially_move_assignable<inplace_vector<Trivial, 8>>::value, "");
    static_assert(std::is_trivially_destructible<inplace_vector<Trivial, 8>>::value, "");

    static_assert(not std::is_trivially_copy_constructible<inplace_vector<NotTrivial, 8>>::value, "");
    static_assert(not std::is_trivially_move_constructible<inplace_vector<NotTrivial, 8>>::value, "");
    static_assert(not std::is_trivially_copy_assignable<inplace_vector<NotTrivial, 8>>::value, "");
    static_assert(not std::is_trivially_move_assignable<inplace_vector<NotTrivial, 8>>::value, "");
    static_assert(not std::is_trivially_destructible<inplace_vector<NotTrivial, 8>>::value, "");

    static_assert(std::is_trivially_copy_constructible<inplace_vector<NotTriviallyAssignable, 8>>::value, "");
    static_assert(std::is_trivially_move_constructible<inplace_vector<NotTriviallyAssignable, 8>>::value, "");
    static_assert(not std::is_trivially_copy_assignable<inplace_vector<NotTriviallyAssignable, 8>>::value, "");
    static_assert(not std::is_trivially_move_assignable<inplace_vector<NotTriviallyAssignable, 8>>::value, "");
    static_assert(std::is_trivially_destructible<inplace_vector<NotTriviallyAssignable, 8>>::value, "");
}

TEST(inplace_vector, nothrow) {
    static_assert(std::is_nothrow_copy_constructible<inplace_vector<Trivial, 8>>::value, "");
    static_assert(std::is_nothrow_move_constructible<inplace_vector<Trivial, 8>>::value, "");
    static_assert(std::is_nothrow_copy_assignable<inplace_vector<Trivial, 8>>::value, "");
    static_assert(std::is_nothrow_move_assignable<inplace_vector<Trivial, 8>>::value, "");

    static_assert(std::is_nothrow_copy_constructible<inplace_vector<NotTrivial, 8>>::value, "");
    static_assert(std::is_nothrow_move_constructible<inplace_vector<NotTrivial, 8>>::value, "");
    static_assert(std::is_nothrow_copy_assignable<inplace_vector<NotTrivial, 8>>::value, "");
    static_assert(std::is_nothrow_move_assignable<inplace_vector<NotTrivial, 8>>::value, "");

    static_assert(std::is_nothrow_copy_constructible<inplace_vector<NotTriviallyAssignable, 8>>::value, "");
    static_assert(std::is_nothrow_move_constructible<inplace_vector<NotTriviallyAssignable, 8>>::value, "");
    static_assert(std::is_nothrow_copy_assignable<inplace_vector<NotTriviallyAssignable, 8>>::value, "");
    static_assert(std::is_nothrow_move_assignable<inplace_vector<NotTriviallyAssignable, 8>>::value, "");

    static_assert(not std::is_nothrow_copy_constructible<inplace_vector<NotNothrow, 8>>::value, "");
    static_assert(not std::is_nothrow_move_constructible<inplace_vector<NotNothrow, 8>>::value, "");
    static_assert(not std::is_nothrow_copy_assignable<inplace_vector<NotNothrow, 8>>::value, "");
    static_assert(not std::is_nothrow_move_assignable<inplace_vector<NotNothrow, 8>>::value, "");
}

TEST(inplace_vector, copyable) {
    static_assert(not std::is_copy_constructible<inplace_vector<std::mutex, 8>>::value, "");
    static_assert(not std::is_copy_assignable<inplace_vector<std::mutex, 8>>::value, "");

    static_assert(not std::is_copy_constructible<inplace_vector<std::unique_ptr<int>, 8>>::value, "");
    static_assert(not std::is_copy_assignable<inplace_vector<std::unique_ptr<int>, 8>>::value, "");
}

TEST(inplace_vector, moveable) {
    static_assert(not std::is_move_constructible<inplace_vector<std::mutex, 8>>::value, "");
    static_assert(not std::is_move_assignable<inplace_vector<std::mutex, 8>>::value, "");

    static_assert(std::is_move_constructible<inplace_vector<std::unique_ptr<int>, 8>>::value, "");
    static_assert(std::is_move_assignable<inplace_vector<std::unique_ptr<int>, 8>>::value, "");
}
