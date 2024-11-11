#ifndef CIELLAB_INCLUDE_CIEL_INT_WRAPPER_HPP_
#define CIELLAB_INCLUDE_CIEL_INT_WRAPPER_HPP_

#include <ciel/config.hpp>
#include <ciel/exchange.hpp>
#include <ciel/is_trivially_relocatable.hpp>

#include <cstddef>

NAMESPACE_CIEL_BEGIN

template<bool IsTriviallyRelocatable, bool IsNothrowMovable>
struct int_wrapper {
private:
    int i_;

public:
    int_wrapper(const int i = 0) noexcept
        : i_(i) {}

    int_wrapper(const int_wrapper&) = default;
    // clang-format off
    int_wrapper& operator=(const int_wrapper&) = default;
    // clang-format on

    int_wrapper(int_wrapper&& other) noexcept(IsNothrowMovable)
        : i_(ciel::exchange(other.i_, -1)) {}

    int_wrapper&
    operator=(int_wrapper&& other) noexcept(IsNothrowMovable) {
        i_ = ciel::exchange(other.i_, -1);
        return *this;
    }

    ~int_wrapper() {
        i_ = -2;
    }

    int_wrapper&
    operator++() noexcept {
        ++i_;
        return *this;
    }

    CIEL_NODISCARD int_wrapper
    operator++(int) noexcept {
        auto res = *this;
        ++(*this);
        return res;
    }

    int_wrapper&
    operator--() noexcept {
        --i_;
        return *this;
    }

    CIEL_NODISCARD int_wrapper
    operator--(int) noexcept {
        auto res = *this;
        --(*this);
        return res;
    }

    int_wrapper&
    operator+=(const int_wrapper other) noexcept {
        i_ += other.i_;
        return *this;
    }

    int_wrapper&
    operator-=(const int_wrapper other) noexcept {
        return (*this) += (-other);
    }

    CIEL_NODISCARD int_wrapper
    operator-() noexcept {
        int_wrapper res(-i_);
        return res;
    }

    CIEL_NODISCARD
    operator int() const noexcept {
        return i_;
    }

    CIEL_NODISCARD friend int_wrapper
    operator+(int_wrapper lhs, const int_wrapper rhs) noexcept {
        lhs.i_ += rhs.i_;
        return lhs;
    }

    CIEL_NODISCARD friend int_wrapper
    operator-(int_wrapper lhs, const int_wrapper rhs) noexcept {
        lhs.i_ -= rhs.i_;
        return lhs;
    }

}; // struct int_wrapper

using Int   = int_wrapper<false, true>;
using TRInt = int_wrapper<true, true>;
using TMInt = int_wrapper<false, false>;

template<>
struct is_trivially_relocatable<Int> : std::false_type {};

template<>
struct is_trivially_relocatable<TMInt> : std::false_type {};

template<>
struct is_trivially_relocatable<TRInt> : std::true_type {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_INT_WRAPPER_HPP_
