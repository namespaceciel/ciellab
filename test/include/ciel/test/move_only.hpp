#ifndef CIELLAB_INCLUDE_CIEL_MOVE_ONLY_HPP_
#define CIELLAB_INCLUDE_CIEL_MOVE_ONLY_HPP_

#include <ciel/config.hpp>
#include <ciel/exchange.hpp>

#include <cstddef>
#include <functional>

NAMESPACE_CIEL_BEGIN

class MoveOnly {
    int data_;

public:
    MoveOnly(int data = 1) noexcept
        : data_(data) {}

    MoveOnly(const MoveOnly&) = delete;
    // clang-format off
    MoveOnly& operator=(const MoveOnly&) = delete;
    // clang-format on

    MoveOnly(MoveOnly&& x) noexcept
        : data_(ciel::exchange(x.data_, 0)) {}

    MoveOnly&
    operator=(MoveOnly&& x) noexcept {
        data_ = ciel::exchange(x.data_, 0);
        return *this;
    }

    CIEL_NODISCARD int
    get() const noexcept {
        return data_;
    }

    CIEL_NODISCARD MoveOnly
    operator+(const MoveOnly& x) const noexcept {
        return MoveOnly(data_ + x.data_);
    }

    CIEL_NODISCARD MoveOnly
    operator*(const MoveOnly& x) const noexcept {
        return MoveOnly(data_ * x.data_);
    }

    CIEL_NODISCARD friend bool
    operator==(const MoveOnly& x, const MoveOnly& y) noexcept {
        return x.data_ == y.data_;
    }

    CIEL_NODISCARD friend bool
    operator<(const MoveOnly& x, const MoveOnly& y) noexcept {
        return x.data_ < y.data_;
    }

}; // class MoveOnly

class TrivialMoveOnly {
    int data_;

public:
    TrivialMoveOnly(int data = 1) noexcept
        : data_(data) {}

    TrivialMoveOnly(const TrivialMoveOnly&) = delete;
    // clang-format off
    TrivialMoveOnly& operator=(const TrivialMoveOnly&) = delete;
    // clang-format on

    TrivialMoveOnly(TrivialMoveOnly&&) noexcept = default;
    // clang-format off
    TrivialMoveOnly& operator=(TrivialMoveOnly&&) noexcept = default;
    // clang-format on

    CIEL_NODISCARD int
    get() const noexcept {
        return data_;
    }

    CIEL_NODISCARD TrivialMoveOnly
    operator+(const TrivialMoveOnly& x) const noexcept {
        return TrivialMoveOnly(data_ + x.data_);
    }

    CIEL_NODISCARD TrivialMoveOnly
    operator*(const TrivialMoveOnly& x) const noexcept {
        return TrivialMoveOnly(data_ * x.data_);
    }

    CIEL_NODISCARD friend bool
    operator==(const TrivialMoveOnly& x, const TrivialMoveOnly& y) noexcept {
        return x.data_ == y.data_;
    }

    CIEL_NODISCARD friend bool
    operator<(const TrivialMoveOnly& x, const TrivialMoveOnly& y) noexcept {
        return x.data_ < y.data_;
    }

}; // class TrivialMoveOnly

NAMESPACE_CIEL_END

namespace std {

template<>
struct hash<ciel::MoveOnly> {
    CIEL_NODISCARD size_t
    operator()(const ciel::MoveOnly& x) const noexcept {
        return x.get();
    }
};

template<>
struct hash<ciel::TrivialMoveOnly> {
    CIEL_NODISCARD size_t
    operator()(const ciel::TrivialMoveOnly& x) const noexcept {
        return x.get();
    }
};

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_MOVE_ONLY_HPP_
