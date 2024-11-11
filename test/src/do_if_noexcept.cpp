#include <gtest/gtest.h>

#include <ciel/do_if_noexcept.hpp>

#include <type_traits>

using namespace ciel;

namespace {

struct NoexceptMove {
    NoexceptMove() = default;

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NoexceptMove(const NoexceptMove&) noexcept {}

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NoexceptMove(NoexceptMove&&) noexcept {}
};

struct NonNothrowMove {
    NonNothrowMove() = default;

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NonNothrowMove(const NonNothrowMove&) {}

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NonNothrowMove(NonNothrowMove&&) {}
};

struct NonCopy {
    NonCopy()               = default;
    NonCopy(const NonCopy&) = delete;

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NonCopy(NonCopy&&) {}
};

} // namespace

TEST(do_if_noexcept, move) {
    NoexceptMove a;   // NOLINT(misc-const-correctness)
    NonNothrowMove b; // NOLINT(misc-const-correctness)
    NonCopy c;        // NOLINT(misc-const-correctness)

#ifdef CIEL_HAS_EXCEPTIONS
    static_assert(std::is_same<decltype(ciel::move_if_noexcept(a)), NoexceptMove&&>::value, "");
    static_assert(std::is_same<decltype(ciel::move_if_noexcept(b)), const NonNothrowMove&>::value, "");
    static_assert(std::is_same<decltype(ciel::move_if_noexcept(c)), NonCopy&&>::value, "");
#else
    static_assert(std::is_same<decltype(ciel::move_if_noexcept(a)), NoexceptMove&&>::value, "");
    static_assert(std::is_same<decltype(ciel::move_if_noexcept(b)), NonNothrowMove&&>::value, "");
    static_assert(std::is_same<decltype(ciel::move_if_noexcept(c)), NonCopy&&>::value, "");
#endif
}

TEST(do_if_noexcept, forward) {
    NoexceptMove a;   // NOLINT(misc-const-correctness)
    NonNothrowMove b; // NOLINT(misc-const-correctness)
    NonCopy c;        // NOLINT(misc-const-correctness)

#ifdef CIEL_HAS_EXCEPTIONS
    static_assert(std::is_same<decltype(ciel::forward_if_noexcept(a)), NoexceptMove&>::value, "");
    static_assert(std::is_same<decltype(ciel::forward_if_noexcept(std::move(a))), NoexceptMove&&>::value, "");
    static_assert(std::is_same<decltype(ciel::forward_if_noexcept(b)), const NonNothrowMove&>::value, "");
    static_assert(std::is_same<decltype(ciel::forward_if_noexcept(std::move(b))), const NonNothrowMove&>::value, "");
    static_assert(std::is_same<decltype(ciel::forward_if_noexcept(c)), NonCopy&>::value, "");
    static_assert(std::is_same<decltype(ciel::forward_if_noexcept(std::move(c))), NonCopy&&>::value, "");
#else
    static_assert(std::is_same<decltype(ciel::forward_if_noexcept(a)), NoexceptMove&>::value, "");
    static_assert(std::is_same<decltype(ciel::forward_if_noexcept(std::move(a))), NoexceptMove&&>::value, "");
    static_assert(std::is_same<decltype(ciel::forward_if_noexcept(b)), NonNothrowMove&>::value, "");
    static_assert(std::is_same<decltype(ciel::forward_if_noexcept(std::move(b))), NonNothrowMove&&>::value, "");
    static_assert(std::is_same<decltype(ciel::forward_if_noexcept(c)), NonCopy&>::value, "");
    static_assert(std::is_same<decltype(ciel::forward_if_noexcept(std::move(c))), NonCopy&&>::value, "");
#endif
}
