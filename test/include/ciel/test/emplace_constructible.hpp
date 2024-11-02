#ifndef CIELLAB_INCLUDE_CIEL_EMPLACE_CONSTRUCTIBLE_HPP_
#define CIELLAB_INCLUDE_CIEL_EMPLACE_CONSTRUCTIBLE_HPP_

#include <ciel/config.hpp>

#include <utility>

NAMESPACE_CIEL_BEGIN

template<class T>
struct EmplaceConstructible {
    T value;

    EmplaceConstructible(T xvalue) noexcept
        : value(xvalue) {}

    EmplaceConstructible(const EmplaceConstructible&) = delete;

}; // struct EmplaceConstructible

template<class T>
struct EmplaceConstructibleAndMoveInsertable {
    int copied = 0;
    T value;

    EmplaceConstructibleAndMoveInsertable(T xvalue) noexcept
        : value(xvalue) {}

    EmplaceConstructibleAndMoveInsertable(EmplaceConstructibleAndMoveInsertable&& Other) noexcept
        : copied(Other.copied + 1), value(std::move(Other.value)) {}

}; // struct EmplaceConstructibleAndMoveInsertable

template<class T>
struct EmplaceConstructibleAndMoveable {
    int copied   = 0;
    int assigned = 0;
    T value;

    EmplaceConstructibleAndMoveable(T xvalue) noexcept
        : value(xvalue) {}

    EmplaceConstructibleAndMoveable(EmplaceConstructibleAndMoveable&& Other) noexcept
        : copied(Other.copied + 1), value(std::move(Other.value)) {}

    EmplaceConstructibleAndMoveable&
    operator=(EmplaceConstructibleAndMoveable&& Other) noexcept {
        copied   = Other.copied;
        assigned = Other.assigned + 1;
        value    = std::move(Other.value);
        return *this;
    }

}; // struct EmplaceConstructibleAndMoveable

template<class T>
struct EmplaceConstructibleMoveableAndAssignable {
    int copied   = 0;
    int assigned = 0;
    T value;

    EmplaceConstructibleMoveableAndAssignable(T xvalue) noexcept
        : value(xvalue) {}

    EmplaceConstructibleMoveableAndAssignable(EmplaceConstructibleMoveableAndAssignable&& Other) noexcept
        : copied(Other.copied + 1), value(std::move(Other.value)) {}

    EmplaceConstructibleMoveableAndAssignable&
    operator=(EmplaceConstructibleMoveableAndAssignable&& Other) noexcept {
        copied   = Other.copied;
        assigned = Other.assigned + 1;
        value    = std::move(Other.value);
        return *this;
    }

    EmplaceConstructibleMoveableAndAssignable&
    operator=(T xvalue) noexcept {
        value = std::move(xvalue);
        ++assigned;
        return *this;
    }

}; // struct EmplaceConstructibleMoveableAndAssignable

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_EMPLACE_CONSTRUCTIBLE_HPP_
