#ifndef CIELLAB_INCLUDE_CIEL_OPERATOR_HIJACKER_HPP_
#define CIELLAB_INCLUDE_CIEL_OPERATOR_HIJACKER_HPP_

#include <ciel/config.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

struct operator_hijacker {
    CIEL_NODISCARD bool
    operator==(const operator_hijacker&) const {
        return true;
    }

    CIEL_NODISCARD bool
    operator<(const operator_hijacker&) const {
        return true;
    }

    // clang-format off
    template<class T>
    friend void operator&(T&&)       = delete;
    template <class T, class U>
    friend void operator,(T&&, U&&)  = delete;
    template<class T, class U>
    friend void operator&&(T&&, U&&) = delete;
    template<class T, class U>
    friend void operator||(T&&, U&&) = delete;
    // clang-format on

}; // struct operator_hijacker

template<class T>
struct operator_hijacker_allocator : std::allocator<T>,
                                     operator_hijacker {
#if CIEL_STD_VER <= 17
    struct rebind {
        typedef operator_hijacker_allocator<T> other;
    };
#endif

}; // struct operator_hijacker_allocator

NAMESPACE_CIEL_END

namespace std {

template<>
struct hash<ciel::operator_hijacker> {
    CIEL_NODISCARD size_t
    operator()(ciel::operator_hijacker) const noexcept {
        return 0;
    }
};

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_OPERATOR_HIJACKER_HPP_
