#include <gtest/gtest.h>

#include <ciel/inplace_vector.hpp>

#include <type_traits>

using namespace ciel;

namespace {

struct Trivial {};

struct NotTrivial {
    NotTrivial() = default;

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NotTrivial(const NotTrivial&) noexcept {}

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NotTrivial(NotTrivial&&) noexcept {}

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NotTrivial& operator=(const NotTrivial&) noexcept {
        return *this;
    }

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NotTrivial& operator=(NotTrivial&&) noexcept {
        return *this;
    }

    // NOLINTNEXTLINE(modernize-use-equals-default)
    ~NotTrivial() {}
};

struct NotTriviallyAssignable {
    NotTriviallyAssignable()                              = default;
    NotTriviallyAssignable(const NotTriviallyAssignable&) = default;
    NotTriviallyAssignable(NotTriviallyAssignable&&)      = default;

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NotTriviallyAssignable& operator=(const NotTriviallyAssignable&) noexcept {
        return *this;
    }

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NotTriviallyAssignable& operator=(NotTriviallyAssignable&&) noexcept {
        return *this;
    }
};

struct NotNothrow {
    NotNothrow() = default;

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NotNothrow(const NotNothrow&) {}

    // NOLINTNEXTLINE(modernize-use-equals-default, performance-noexcept-move-constructor)
    NotNothrow(NotNothrow&&) {}

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NotNothrow& operator=(const NotNothrow&) {
        return *this;
    }

    // NOLINTNEXTLINE(modernize-use-equals-default, performance-noexcept-move-constructor)
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
