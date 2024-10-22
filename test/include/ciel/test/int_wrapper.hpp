#ifndef CIELLAB_INCLUDE_CIEL_INT_WRAPPER_HPP_
#define CIELLAB_INCLUDE_CIEL_INT_WRAPPER_HPP_

#include <ciel/config.hpp>
#include <ciel/exchange.hpp>

#include <cstddef>

struct Int {
private:
    int i_;

public:
    Int(const int i = 0) noexcept
        : i_(i) {}

    Int(const Int&) noexcept = default;
    // clang-format off
    Int& operator=(const Int&) noexcept = default;
    // clang-format on

    Int(Int&& other) noexcept
        : i_(ciel::exchange(other.i_, -1)) {}

    Int&
    operator=(Int&& other) noexcept {
        i_ = ciel::exchange(other.i_, -1);
        return *this;
    }

    Int&
    operator++() noexcept {
        ++i_;
        return *this;
    }

    CIEL_NODISCARD Int
    operator++(int) noexcept {
        auto res = *this;
        ++(*this);
        return res;
    }

    Int&
    operator--() noexcept {
        --i_;
        return *this;
    }

    CIEL_NODISCARD Int
    operator--(int) noexcept {
        auto res = *this;
        --(*this);
        return res;
    }

    Int&
    operator+=(const Int other) noexcept {
        i_ += other.i_;
        return *this;
    }

    Int&
    operator-=(const Int other) noexcept {
        return (*this) += (-other);
    }

    CIEL_NODISCARD Int
    operator-() noexcept {
        Int res(-i_);
        return res;
    }

    CIEL_NODISCARD
    operator int() const noexcept {
        return i_;
    }

    CIEL_NODISCARD friend Int
    operator+(Int lhs, const Int rhs) noexcept {
        lhs.i_ += rhs.i_;
        return lhs;
    }

    CIEL_NODISCARD friend Int
    operator-(Int lhs, const Int rhs) noexcept {
        lhs.i_ -= rhs.i_;
        return lhs;
    }

}; // struct Int

#endif // CIELLAB_INCLUDE_CIEL_INT_WRAPPER_HPP_
